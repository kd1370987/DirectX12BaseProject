#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ScaleHelpers.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystem.h>

#include "PhysicsEngine.h"
#include "Internal/JoltMath.h"
#include "Core/PhysicsLayer.h"
#include "Core/PhysicsBroadPhaseLayer/PhysicsBroadPhaseLayer.h"
#include "Core/PhysicsObjectVsBroadPhaseLayerFilter/PhysicsObjectVsBroadPhaseLayerFilter.h"
#include "Core/PysicsObjectLayerPairFilter/PysicsObjectLayerPairFilter.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Model/Model.h"
#include "Engine/Resource/Data/Mesh/Mesh.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"

namespace Engine::Physics
{
	//======================================================================================
	// Jolt の型を持つ中身
	//======================================================================================
	struct PhysicsWorld::Detail
	{
		// モデルごとの形状(モデル空間)。ボディはこれに ScaledShape を被せて使う。
		// ワールドと一緒に捨てる(シーンの切れ目で作り直す)
		std::unordered_map<uint32_t, JPH::RefConst<JPH::Shape>> modelShapeCache;

		// 作ったがまだ空間へ入れていないボディ。Update でまとめて入れる
		std::vector<JPH::BodyID> pendingAdd;

		// まだ一度もボディを入れていないか(最初の追加 = シーン読み込みの分)
		bool isFirstFlush = true;
	};

	static_assert(kQueryAllLayers == Layer::kAllGroups, "クエリの全レイヤーとレイヤーの詰め方がずれている");

	namespace
	{
		//----------------------------------------------------------------------------------
		// 持ち主が a_ignore のボディを外す(自分自身に当たらないように)。
		// ボディの UserData に持ち主のエンティティを入れてある
		//----------------------------------------------------------------------------------
		class IgnoreOwnerBodyFilter final : public JPH::BodyFilter
		{
		public:
			explicit IgnoreOwnerBodyFilter(ECS::Entity a_ignore) noexcept : m_ignore(a_ignore) {}

			bool ShouldCollideLocked(const JPH::Body& a_body) const override
			{
				return a_body.GetUserData() != static_cast<JPH::uint64>(m_ignore);
			}

		private:
			ECS::Entity m_ignore = ECS::Limits::INVALID_ENTITY;
		};

		bool IsFinite(const Math::Vector3& a_value)
		{
			return std::isfinite(a_value.x) && std::isfinite(a_value.y) && std::isfinite(a_value.z);
		}

		//----------------------------------------------------------------------------------
		// モデルの判定メッシュ(COL ノード)の三角形を集める。
		// 旧 CollisionWorld と同じデータ(Mesh::GetCollisionMesh の三角形)を、
		// ノードの行列 × a_extra で変換して積む
		//----------------------------------------------------------------------------------
		void CollectModelTriangles(
			const Resource::ResourceManager& a_resourceManager,
			const Resource::Model& a_model,
			const Math::Matrix& a_extra,
			JPH::TriangleList& a_outTriangles)
		{
			const auto& _nodeVec = a_model.GetOriginalNodeVec();
			const auto& _meshHandles = a_model.GetMeshHandles();

			for (int _nodeIdx : a_model.GetCollisionMeshNodeVec())
			{
				if (_nodeIdx < 0 || _nodeIdx >= static_cast<int>(_nodeVec.size())) continue;
				const Resource::Node& _node = _nodeVec[_nodeIdx];

				const Math::Matrix _mat = _node.worldTransform * a_extra;

				for (int _meshIdx : _node.meshIndices)
				{
					if (_meshIdx < 0 || _meshIdx >= static_cast<int>(_meshHandles.size())) continue;

					const Resource::Mesh* _pMesh = a_resourceManager.Get(_meshHandles[_meshIdx]);
					if (!_pMesh || !_pMesh->HasCollisionMesh()) continue;

					for (const auto& _tri : _pMesh->GetCollisionMesh().triangleVec)
					{
						const Math::Vector3 _v0 = Math::Vector3::TransformCoord(_tri.v[0], _mat);
						const Math::Vector3 _v1 = Math::Vector3::TransformCoord(_tri.v[1], _mat);
						const Math::Vector3 _v2 = Math::Vector3::TransformCoord(_tri.v[2], _mat);
						a_outTriangles.push_back(JPH::Triangle(
							JPH::Float3(_v0.x, _v0.y, _v0.z),
							JPH::Float3(_v1.x, _v1.y, _v1.z),
							JPH::Float3(_v2.x, _v2.y, _v2.z)));
					}
				}
			}
		}

#ifdef _DEBUG
		//----------------------------------------------------------------------------------
		// 面の向きの確認(Debug のみ・形状を作ったときに1回)
		//
		// エンジンは左手系で、モデルは読み込み時に Z ミラーと面の巻き直しを通っている。
		// Jolt は (v1-v0)x(v2-v0) を表の法線とし、既定のレイは裏面を無視するので、
		// 巻きが逆だと上から撃った接地レイが地形を素通りする。
		// 上から真下へ 5x5 本撃ち、最初に当たった面が表か裏かを数えて残す
		//----------------------------------------------------------------------------------
		void LogFaceOrientation(const JPH::Shape& a_shape, uint32_t a_modelId, size_t a_triangleCount)
		{
			const JPH::AABox _bounds = a_shape.GetLocalBounds();
			const JPH::Vec3 _min = _bounds.mMin;
			const JPH::Vec3 _size = _bounds.GetSize();
			const float _startY = _bounds.mMax.GetY() + 1.0f;
			const float _length = _size.GetY() + 2.0f;

			JPH::RayCastSettings _settings;
			_settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;

			int _front = 0;
			int _back = 0;
			constexpr int _div = 5;
			for (int _z = 0; _z < _div; ++_z)
			{
				for (int _x = 0; _x < _div; ++_x)
				{
					const float _fx = (static_cast<float>(_x) + 0.5f) / _div;
					const float _fz = (static_cast<float>(_z) + 0.5f) / _div;
					const JPH::Vec3 _origin(
						_min.GetX() + _size.GetX() * _fx,
						_startY,
						_min.GetZ() + _size.GetZ() * _fz);
					const JPH::RayCast _ray(_origin, JPH::Vec3(0.0f, -_length, 0.0f));

					JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> _collector;
					a_shape.CastRay(_ray, _settings, JPH::SubShapeIDCreator(), _collector);
					if (!_collector.HadHit()) continue;

					const JPH::Vec3 _normal = a_shape.GetSurfaceNormal(
						_collector.mHit.mSubShapeID2, _ray.GetPointOnRay(_collector.mHit.mFraction));
					(_normal.Dot(_ray.mDirection) < 0.0f) ? ++_front : ++_back;
				}
			}

			ENGINE_LOG("[Physics] mesh shape model=0x%08x tris=%zu : down rays hit front=%d back=%d",
				a_modelId, a_triangleCount, _front, _back);
		}
#endif

		//----------------------------------------------------------------------------------
		// 三角形からメッシュ形状を作る。作れなければ nullptr
		//----------------------------------------------------------------------------------
		JPH::RefConst<JPH::Shape> CreateMeshShape(const JPH::TriangleList& a_triangles, uint32_t a_modelId)
		{
			if (a_triangles.empty())
			{
				ENGINE_WARNING("[Physics] 判定メッシュに三角形がありません(model=0x%08x)", a_modelId);
				return nullptr;
			}

			JPH::MeshShapeSettings _settings(a_triangles);
			_settings.SetEmbedded();

			const JPH::ShapeSettings::ShapeResult _result = _settings.Create();
			if (_result.HasError())
			{
				ENGINE_WARNING("[Physics] メッシュ形状を作れませんでした(model=0x%08x) : %s",
					a_modelId, _result.GetError().c_str());
				return nullptr;
			}

#ifdef _DEBUG
			LogFaceOrientation(*_result.Get(), a_modelId, a_triangles.size());
#endif
			return _result.Get();
		}

		//----------------------------------------------------------------------------------
		// 行列を 拡大縮小・回転・平行移動 に分けられるか。
		// 親の非等方スケールの下で回っているとせん断が混ざり、分けると元に戻らない
		//----------------------------------------------------------------------------------
		bool IsDecomposable(const Math::Matrix& a_mat, const Math::TRS& a_trs)
		{
			const Math::Matrix _recomposed =
				Math::Matrix::CreateScale(a_trs.scale) *
				Math::Matrix::CreateFromQuaternion(a_trs.rotation) *
				Math::Matrix::CreateTranslation(a_trs.pos);

			float _maxAbs = 1.0f;
			for (int _r = 0; _r < 4; ++_r)
				for (int _c = 0; _c < 4; ++_c)
					_maxAbs = (std::max)(_maxAbs, std::fabs(a_mat.m[_r][_c]));

			const float _tolerance = 1e-4f * _maxAbs;
			for (int _r = 0; _r < 4; ++_r)
				for (int _c = 0; _c < 4; ++_c)
					if (std::fabs(_recomposed.m[_r][_c] - a_mat.m[_r][_c]) > _tolerance) return false;
			return true;
		}
	}

	PhysicsWorld::PhysicsWorld(PhysicsEngine* a_pEngine, const PhysicsWorldDesc& a_desc)
		: m_pEngine(a_pEngine)
		, m_upDetail(std::make_unique<Detail>())
	{
		if (!m_pEngine)
		{
			ENGINE_ERROR("[Physics] PhysicsEngine が無いので PhysicsWorld を作れません");
			return;
		}

		// レイヤー定義
		m_upBroadPhaseLayerInterface = std::make_unique<PhysicsBroadPhaseLayer>();
		m_upObjectVsBroadPhaseLayerFilter = std::make_unique<PhysicsObjectVsBroadPhaseLayerFilter>();
		m_upObjectLayerPairFilter = std::make_unique<PysicsObjectLayerPairFilter>();

		// フィジックスシステム
		m_upPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();

		// ボディの排他はしない(ECS はシングルスレッドで、ボディを触るのはメインだけ)。
		// 0 を渡すと Jolt が既定の数を選ぶ
		constexpr JPH::uint _numBodyMutexes = 0;

		m_upPhysicsSystem->Init(
			a_desc.maxBodies,
			_numBodyMutexes,
			a_desc.maxBodyPairs,
			a_desc.maxContactConstraints,
			*m_upBroadPhaseLayerInterface,
			*m_upObjectVsBroadPhaseLayerFilter,
			*m_upObjectLayerPairFilter);

		m_upPhysicsSystem->SetGravity(Internal::ToJolt(a_desc.gravity));

		m_pEngine->OnWorldCreated();
	}

	PhysicsWorld::~PhysicsWorld()
	{
		if (!m_upPhysicsSystem) return;

		// PhysicsSystem を先に壊す(レイヤー定義を参照で持っているため)。
		// 残っているボディは PhysicsSystem が一緒に消す
		m_upPhysicsSystem.reset();
		m_upDetail.reset();
		m_upObjectLayerPairFilter.reset();
		m_upObjectVsBroadPhaseLayerFilter.reset();
		m_upBroadPhaseLayerInterface.reset();

		m_pEngine->OnWorldDestroyed();
	}

	void PhysicsWorld::Update(float a_dt)
	{
		if (!m_upPhysicsSystem) return;

		FlushPendingBodies();

		// ボディが1つも無ければ何もしない(プレビューや、まだ登録が無いシーン)
		if (m_upPhysicsSystem->GetNumBodies() == 0) return;

		constexpr int _collisionSteps = 1;
		const JPH::EPhysicsUpdateError _error = m_upPhysicsSystem->Update(
			a_dt,
			_collisionSteps,
			m_pEngine->RefTempAllocator(),
			m_pEngine->RefJobSystem());

		if (_error != JPH::EPhysicsUpdateError::None)
		{
			ENGINE_WARNING("[Physics] PhysicsSystem::Update で上限を超えました(0x%x)", static_cast<uint32_t>(_error));
		}
	}

	void PhysicsWorld::FlushPendingBodies()
	{
		auto& _pending = m_upDetail->pendingAdd;
		if (_pending.empty()) return;

		// まとめて入れる(1体ずつ入れるとブロードフェーズの木を毎回組み替える)
		JPH::BodyInterface& _bodyInterface = m_upPhysicsSystem->GetBodyInterfaceNoLock();
		const int _count = static_cast<int>(_pending.size());
		const JPH::BodyInterface::AddState _state = _bodyInterface.AddBodiesPrepare(_pending.data(), _count);
		_bodyInterface.AddBodiesFinalize(_pending.data(), _count, _state, JPH::EActivation::DontActivate);
		_pending.clear();

		// 一度にたくさん入れた後は木を組み直しておく(シーン読み込み直後の地形など)
		m_upPhysicsSystem->OptimizeBroadPhase();

		// シーン読み込み(最初の追加・まとまった数)のときだけ残す。
		// 弾のように毎フレーム少しずつ入るものは出さない
		constexpr int _logThreshold = 32;
		const bool _isFirst = m_upDetail->isFirstFlush;
		m_upDetail->isFirstFlush = false;
		if (_isFirst || _count >= _logThreshold)
		{
			ENGINE_LOG("[Physics] %d bodies added (total %u)", _count, m_upPhysicsSystem->GetNumBodies());
		}
	}

	BodyHandle PhysicsWorld::CreateStaticModelBody(const Resource::ResourceManager& a_resourceManager, const StaticModelBodyDesc& a_desc)
	{
		if (!m_upPhysicsSystem) return {};

		const Resource::Model* _pModel = a_resourceManager.Get(a_desc.modelHandle);
		if (!_pModel) return {};

		if (!Layer::IsInRange(a_desc.group) || !Layer::IsInRange(a_desc.mask))
		{
			ENGINE_WARNING("[Physics] レイヤーのビットが範囲外です(group=0x%x mask=0x%x)。はみ出した分は無視します",
				a_desc.group, a_desc.mask);
		}

		const uint32_t _modelId = a_desc.modelHandle.id;
		const Math::TRS _trs = Math::Decompose(a_desc.worldMat);

		JPH::RefConst<JPH::Shape> _shape;
		JPH::RVec3 _position = JPH::RVec3::sZero();
		JPH::Quat _rotation = JPH::Quat::sIdentity();

		if (IsDecomposable(a_desc.worldMat, _trs))
		{
			// モデル空間の形状を使い回し、インスタンスの拡大縮小だけ被せる
			auto& _cache = m_upDetail->modelShapeCache;
			auto _it = _cache.find(_modelId);
			if (_it == _cache.end())
			{
				JPH::TriangleList _triangles;
				CollectModelTriangles(a_resourceManager, *_pModel, Math::Matrix::Identity(), _triangles);
				_it = _cache.emplace(_modelId, CreateMeshShape(_triangles, _modelId)).first;
			}
			if (_it->second.GetPtr() == nullptr) return {};

			const JPH::Vec3 _scale = Internal::ToJolt(_trs.scale);
			if (JPH::ScaleHelpers::IsZeroScale(_scale))
			{
				ENGINE_WARNING("[Physics] 拡大率が 0 のインスタンスは登録しません(model=0x%08x)", _modelId);
				return {};
			}

			_shape = _scale.IsClose(JPH::Vec3::sOne())
				? _it->second
				: JPH::RefConst<JPH::Shape>(new JPH::ScaledShape(_it->second, _scale));
			_position = Internal::ToJoltR(_trs.pos);
			_rotation = Internal::ToJolt(_trs.rotation).Normalized();
		}
		else
		{
			// せん断が混ざっていて分けられない。行列ごと頂点に焼き込んだ専用の形状にする
			ENGINE_WARNING("[Physics] 行列にせん断が混ざっているので形状を焼き込みます(model=0x%08x)", _modelId);

			JPH::TriangleList _triangles;
			CollectModelTriangles(a_resourceManager, *_pModel, a_desc.worldMat, _triangles);
			_shape = CreateMeshShape(_triangles, _modelId);
			if (_shape.GetPtr() == nullptr) return {};
		}

		JPH::BodyCreationSettings _settings(
			_shape,
			_position,
			_rotation,
			JPH::EMotionType::Static,
			Layer::Make(a_desc.group, a_desc.mask, false));
		_settings.mUserData = static_cast<JPH::uint64>(a_desc.owner);

		JPH::BodyInterface& _bodyInterface = m_upPhysicsSystem->GetBodyInterfaceNoLock();
		JPH::Body* _pBody = _bodyInterface.CreateBody(_settings);
		if (!_pBody)
		{
			ENGINE_ERROR("[Physics] ボディの上限に達したので作れませんでした(%u 体)", m_upPhysicsSystem->GetNumBodies());
			return {};
		}

		m_upDetail->pendingAdd.push_back(_pBody->GetID());

		BodyHandle _handle;
		_handle.id = _pBody->GetID().GetIndexAndSequenceNumber();
		return _handle;
	}

	void PhysicsWorld::DestroyBody(BodyHandle a_handle, ECS::Entity a_owner)
	{
		if (!m_upPhysicsSystem || !a_handle.IsValid()) return;

		const JPH::BodyID _id(a_handle.id);

		// 持ち主の照合。消えた札・他人の札なら何もしない
		{
			JPH::BodyLockRead _lock(m_upPhysicsSystem->GetBodyLockInterfaceNoLock(), _id);
			if (!_lock.Succeeded()) return;
			if (_lock.GetBody().GetUserData() != static_cast<JPH::uint64>(a_owner)) return;
		}

		JPH::BodyInterface& _bodyInterface = m_upPhysicsSystem->GetBodyInterfaceNoLock();
		if (_bodyInterface.IsAdded(_id))
		{
			_bodyInterface.RemoveBody(_id);
		}
		else
		{
			// まだ空間へ入れていない(作ったフレームのうちに消えた)
			auto& _pending = m_upDetail->pendingAdd;
			_pending.erase(std::remove(_pending.begin(), _pending.end(), _id), _pending.end());
		}
		_bodyInterface.DestroyBody(_id);
	}

	bool PhysicsWorld::CastRay(const Math::Ray& a_ray, uint32_t a_queryMask, ECS::Entity a_ignore, RayHit& a_outHit) const
	{
		if (!m_upPhysicsSystem) return false;

		// NaN や長さ0の方向で Jolt の中まで行かせない(旧 CollisionWorld::Raycast と同じ弾き方)
		if (!IsFinite(a_ray.origin) || !IsFinite(a_ray.direction)) return false;
		if (!(a_ray.maxDistance > 0.0f)) return false;

		const float _lenSq = a_ray.direction.LengthSquared();
		if (_lenSq < 1e-12f) return false;
		const Math::Vector3 _dir = a_ray.direction / std::sqrt(_lenSq);

		// Jolt のレイは「始点 + 方向×長さ」で、当たりは長さに対する割合で返る
		const JPH::RRayCast _ray(Internal::ToJoltR(a_ray.origin), Internal::ToJolt(_dir * a_ray.maxDistance));

		JPH::RayCastSettings _settings;
		_settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;

		JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> _collector;
		const LayerMaskQueryFilter _layerFilter(a_queryMask);
		const IgnoreOwnerBodyFilter _bodyFilter(a_ignore);

		m_upPhysicsSystem->GetNarrowPhaseQueryNoLock().CastRay(
			_ray, _settings, _collector, {}, _layerFilter, _bodyFilter);

		if (!_collector.HadHit()) return false;

		const JPH::RayCastResult& _hit = _collector.mHit;
		const JPH::RVec3 _position = _ray.GetPointOnRay(_hit.mFraction);

		// 法線と持ち主はボディから引く
		JPH::BodyLockRead _lock(m_upPhysicsSystem->GetBodyLockInterfaceNoLock(), _hit.mBodyID);
		if (!_lock.Succeeded()) return false;
		const JPH::Body& _body = _lock.GetBody();

		a_outHit.entity = static_cast<ECS::Entity>(_body.GetUserData());
		a_outHit.position = Internal::ToMath(JPH::Vec3(_position));
		a_outHit.normal = Internal::ToMath(_body.GetWorldSpaceSurfaceNormal(_hit.mSubShapeID2, _position));
		a_outHit.distance = _hit.mFraction * a_ray.maxDistance;
		return true;
	}

	bool PhysicsWorld::ResolveCapsule(Math::Vector3& a_pointA, Math::Vector3& a_pointB, float a_radius,
		uint32_t a_queryMask, ECS::Entity a_ignore, Math::Vector3& a_outCorrection, int a_iterations) const
	{
		a_outCorrection = {};
		if (!m_upPhysicsSystem) return false;
		if (!(a_radius > 0.0f) || !IsFinite(a_pointA) || !IsFinite(a_pointB)) return false;

		// 旧 CollisionWorld::ResolveCapsule と同じ値
		constexpr float _minDepth = 1e-4f;	// これ以下のめり込みは無視
		constexpr float _bias = 1e-3f;		// 完全に離すための微小バイアス

		// 三角形は表裏どちらにも当たる(旧実装の押し出しは面の向きを見ていなかった)
		JPH::CollideShapeSettings _settings;
		_settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

		const JPH::NarrowPhaseQuery& _query = m_upPhysicsSystem->GetNarrowPhaseQueryNoLock();
		const LayerMaskQueryFilter _layerFilter(a_queryMask);
		const IgnoreOwnerBodyFilter _bodyFilter(a_ignore);

		Math::Vector3 _total = {};
		bool _anyPush = false;

		// 反復して複数面(床+壁など)を解決する。1回ごとにいちばん深い接触を1つだけ解く
		for (int _it = 0; _it < a_iterations; ++_it)
		{
			const Math::Vector3 _axis = a_pointB - a_pointA;
			const float _length = _axis.Length();
			const Math::Vector3 _center = (a_pointA + a_pointB) * 0.5f;

			// 形状はその場で組む(ヒープに置かないので SetEmbedded)。
			// 長さが無ければ球、あればカプセル(Jolt のカプセルは Y 軸向きなので線分の向きへ回す)
			JPH::SphereShape _sphere(a_radius);
			_sphere.SetEmbedded();
			std::optional<JPH::CapsuleShape> _capsule;

			const JPH::Shape* _pShape = &_sphere;
			JPH::Quat _rotation = JPH::Quat::sIdentity();
			constexpr float _minLength = 1e-5f;
			if (_length > _minLength)
			{
				_capsule.emplace(_length * 0.5f, a_radius);
				_capsule->SetEmbedded();
				_pShape = &*_capsule;
				_rotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), Internal::ToJolt(_axis / _length));
			}

			const JPH::RMat44 _transform = JPH::RMat44::sRotationTranslation(_rotation, Internal::ToJoltR(_center));

			// いちばん深い接触だけ拾う(ClosestHit は めり込みが深いほど「近い」扱い)
			JPH::ClosestHitCollisionCollector<JPH::CollideShapeCollector> _collector;
			_query.CollideShape(_pShape, JPH::Vec3::sOne(), _transform, _settings, JPH::RVec3::sZero(),
				_collector, {}, _layerFilter, _bodyFilter);

			if (!_collector.HadHit()) break;

			const JPH::CollideShapeResult& _hit = _collector.mHit;
			if (_hit.mPenetrationDepth < _minDepth) break;

			// mPenetrationAxis は「相手(形状2)を押し出す向き」。こちらはその逆へ動く
			const JPH::Vec3 _penetrationAxis = _hit.mPenetrationAxis;
			if (_penetrationAxis.LengthSq() < 1e-12f) break;
			const Math::Vector3 _normal = Internal::ToMath(-_penetrationAxis.Normalized());

			const Math::Vector3 _push = _normal * (_hit.mPenetrationDepth + _bias);
			a_pointA += _push;
			a_pointB += _push;
			_total += _push;
			_anyPush = true;
		}

		a_outCorrection = _total;
		return _anyPush;
	}

	bool PhysicsWorld::ResolveSphere(Math::Vector3& a_center, float a_radius,
		uint32_t a_queryMask, ECS::Entity a_ignore, Math::Vector3& a_outCorrection, int a_iterations) const
	{
		// 球は長さ0のカプセルとして押し出しを流用する(旧実装と同じ)
		Math::Vector3 _a = a_center;
		Math::Vector3 _b = a_center;
		const bool _pushed = ResolveCapsule(_a, _b, a_radius, a_queryMask, a_ignore, a_outCorrection, a_iterations);
		a_center += a_outCorrection;
		return _pushed;
	}

	uint32_t PhysicsWorld::GetBodyCount() const
	{
		return m_upPhysicsSystem ? m_upPhysicsSystem->GetNumBodies() : 0u;
	}

	void PhysicsWorld::DrawDebug(Graphics::DebugDraw* a_pDebugDraw) const
	{
		if (!a_pDebugDraw || !m_upPhysicsSystem) return;

		// 旧 CollisionWorld(白)と重ねて見比べられるよう、別の色で描く
		constexpr Math::Color _color = { 0.0f, 1.0f, 1.0f, 1.0f };

		JPH::BodyIDVector _ids;
		m_upPhysicsSystem->GetBodies(_ids);

		const JPH::BodyLockInterfaceNoLock& _lockInterface = m_upPhysicsSystem->GetBodyLockInterfaceNoLock();
		for (const JPH::BodyID& _id : _ids)
		{
			JPH::BodyLockRead _lock(_lockInterface, _id);
			if (!_lock.Succeeded()) continue;

			const JPH::AABox _bounds = _lock.GetBody().GetWorldSpaceBounds();
			const JPH::Vec3 _center = _bounds.GetCenter();
			const JPH::Vec3 _extent = _bounds.GetExtent();

			DirectX::BoundingBox _box;
			_box.Center = { _center.GetX(), _center.GetY(), _center.GetZ() };
			_box.Extents = { _extent.GetX(), _extent.GetY(), _extent.GetZ() };
			a_pDebugDraw->DrawBox(_box, _color);
		}
	}
}
