#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/Helper/EditorField.h"	// DrawEnumCombo

// パーティクルの発生源(位置・方向)をどこから取るか
// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
enum class EEmitSpace : uint32_t
{
	WorldMatrix,		// 付いているオブジェクトの worldMat をそのまま使う(追従)
	LocalOffset,		// worldMat を基準に posOffset / emitDir を合成する(ノズル位置調整など)
	FixedWorld,			// コンポーネントの絶対 worldPos / emitDir を使う(行列を使わない単発など)
	ReverseVelocity,	// 進行方向(VelocityComponent)の逆へ吹く。位置は worldMat 基準 + posOffset。
						// 弾やミサイルのように「見た目の姿勢が進行方向と一致しない」ものの
						// 噴射・排気向け。速度が無いときは行列の +Z の逆を使う
};

struct ParticlesComponent
{
	// ---- 参照データ ----
	Engine::GUID particleGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> particlesAssetHandle;

	// ---- 発生源 ----
	EEmitSpace			emitSpace = EEmitSpace::WorldMatrix;
	Math::Vector3	posOffset = { 0,0,0 };		// LocalOffset時: worldMat基準の追加移動(ローカル座標)
	Math::Vector3	emitDir   = { 0,0,1 };		// Local/Fixed時の発生方向
	Math::Vector3	worldPos  = { 0,0,0 };		// FixedWorld時の絶対ワールド座標

	// ---- 発生量 ----
	int   emitCount = 8;		// 1回の発生数
	float emitRate  = 0.0f;		// >0: 毎秒 emitRate 回の連続発生 / 0: isPlay立ち上がりで1回だけバースト

	// 生成された時点から発生させるか(ParticleFixupSystem が isPlay に反映する)。
	// ミサイルの噴煙のように「出っぱなし」のものはこれを立てる。
	// ブースターのように状況で入り切りするものは false のままにして、
	// 制御側のシステム(ThrusterEffectSystem 等)に isPlay を任せる。
	bool  playOnStart = false;

	// ---- 形状(スケール/拡散) : アセットに持たせていないのでインスタンス側で持つ ----
	float baseScale      = 1.0f;	// 全体スケール
	float minScale       = 0.1f;	// 個々のスケール下限
	float maxScale       = 1.0f;	// 個々のスケール上限
	float positionRadius = 0.5f;	// 発生位置の半径(ばらつき)
	float directionAngle = 10.0f;	// 方向のばらつき(度)。Cone のときだけ効く

	// 発生方向の決め方。噴射は Cone、爆発は Sphere。
	// 火花(サブパーティクル)も同じ形状で出す
	Engine::Particle::EParticleEmitShape emitShape = Engine::Particle::EParticleEmitShape::Cone;

	// ---- 火花(サブパーティクル) ----
	// isPlay の立ち上がり / 立ち下がりで一度だけ出す別アセット。
	// ブースターの点火・消火の「バチッ」とした表現用。
	// 発生源(emitSpace / posOffset / emitDir)は本体と共有し、
	// 同じフレームに本体と2つ同時に emit される。
	Engine::GUID sparkGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> sparkAssetHandle;

	bool  emitSparkOnStart = false;	// 発動時(isPlay false→true)に出す
	bool  emitSparkOnEnd   = false;	// 終了時(isPlay true→false)に出す
	int   sparkEmitCount   = 16;	// 1回の発生数

	// 火花は本体より大きく・広く散らしたいことが多いので、形状は別に持つ
	float sparkBaseScale      = 1.0f;
	float sparkMinScale       = 0.1f;
	float sparkMaxScale       = 1.0f;
	float sparkPositionRadius = 0.5f;
	float sparkDirectionAngle = 45.0f;

	// ---- ランタイム(保存しない) ----
	bool  isPlay = false;			// 発生させたいか(AttachmentDispatchSystem 等が設定)
	float time = 0.0f;				// レート発生の小数繰り越し用アキュムレータ
	int   pendingEmitCount = 0;		// このフレームの発生数(ParticleEmitSystemが計算 / EmitParticleSystemが消費)
	int   pendingSparkEmitCount = 0;// このフレームの火花発生数(同上)
	bool  wasPlaying = false;		// バーストの立ち上がり / 立ち下がり検出用
};


template<>
struct Engine::ECS::ComponentTraits<ParticlesComponent>
{
	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事。
	// ECS がエンティティを消すとき・コンポーネントを外すとき・
	// PostDeserialize へ入り直すとき(fixup が取り直す)に必ず呼ぶ。
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_pData);
		auto& _resourceManager = *a_services.pResourceManager;

		_resourceManager.ReleaseHandle(_comp.particlesAssetHandle);
		_resourceManager.ReleaseHandle(_comp.sparkAssetHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_pData);
		a_ar.Field("particleGUID",   _comp.particleGUID);

		a_ar.Field("emitSpace",      _comp.emitSpace);
		a_ar.Field("posOffset",      _comp.posOffset);
		a_ar.Field("emitDir",        _comp.emitDir);
		a_ar.Field("worldPos",       _comp.worldPos);

		a_ar.Field("emitCount",      _comp.emitCount);
		a_ar.Field("emitRate",       _comp.emitRate);
		a_ar.Field("playOnStart",    _comp.playOnStart);

		a_ar.Field("baseScale",      _comp.baseScale);
		a_ar.Field("minScale",       _comp.minScale);
		a_ar.Field("maxScale",       _comp.maxScale);
		a_ar.Field("positionRadius", _comp.positionRadius);
		a_ar.Field("directionAngle", _comp.directionAngle);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		a_ar.Field("sparkGUID",            _comp.sparkGUID);
		a_ar.Field("emitSparkOnStart",     _comp.emitSparkOnStart);
		a_ar.Field("emitSparkOnEnd",       _comp.emitSparkOnEnd);
		a_ar.Field("sparkEmitCount",       _comp.sparkEmitCount);
		a_ar.Field("sparkBaseScale",       _comp.sparkBaseScale);
		a_ar.Field("sparkMinScale",        _comp.sparkMinScale);
		a_ar.Field("sparkMaxScale",        _comp.sparkMaxScale);
		a_ar.Field("sparkPositionRadius",  _comp.sparkPositionRadius);
		a_ar.Field("sparkDirectionAngle",  _comp.sparkDirectionAngle);

		a_ar.Field("emitShape",            _comp.emitShape);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_context.pData);

		// ---- 発生源 ----
		Engine::Editor::Text("Emit Source");
		Engine::Editor::Field("EmitSpace", _comp.emitSpace);
		if (_comp.emitSpace == EEmitSpace::LocalOffset)
		{
			Engine::Editor::Field("PosOffset", _comp.posOffset, 0.05f);
			Engine::Editor::Field("EmitDir (local)", _comp.emitDir, 0.05f);
		}
		else if (_comp.emitSpace == EEmitSpace::FixedWorld)
		{
			Engine::Editor::Field("WorldPos", _comp.worldPos, 0.05f);
			Engine::Editor::Field("EmitDir", _comp.emitDir, 0.05f);
		}
		else if (_comp.emitSpace == EEmitSpace::ReverseVelocity)
		{
			// 向きは速度から決まるので EmitDir は使わない
			Engine::Editor::Field("PosOffset", _comp.posOffset, 0.05f);
			Engine::Editor::HelpText("Dir : -Velocity (fallback : -Forward)");
		}

		Engine::Editor::Separator();

		// ---- 発生量 ----
		Engine::Editor::Text("Emission");
		Engine::Editor::Field("EmitCount", _comp.emitCount, 1, 0);
		Engine::Editor::Field("EmitRate (/s, 0=Burst)", _comp.emitRate, 0.5f, 0.0f);

		// 出っぱなしにするか。切り替えは即座に反映して、エディタで確認できるようにする
		// (生成時の反映は ParticleFixupSystem が行う)
		if (Engine::Editor::Field("PlayOnStart", _comp.playOnStart))
		{
			_comp.isPlay = _comp.playOnStart;
		}

		Engine::Editor::Separator();

		// ---- 形状 ----
		Engine::Editor::Text("Shape");
		Engine::Editor::Field("BaseScale", _comp.baseScale, 0.05f, 0.0f);
		Engine::Editor::Field("MinScale", _comp.minScale, 0.01f, 0.0f);
		Engine::Editor::Field("MaxScale", _comp.maxScale, 0.01f, 0.0f);
		Engine::Editor::Field("PositionRadius", _comp.positionRadius, 0.05f, 0.0f);

		// どっちへ出すか。Cone の角度を 360 にしても全方向にはならないので、
		// 爆発のように四方八方へ飛ばしたいときは Sphere を選ぶ
		Engine::Editor::Field("EmitShape", _comp.emitShape);

		{
			Engine::Editor::DisabledScope _disabled(_comp.emitShape != Engine::Particle::EParticleEmitShape::Cone);
			Engine::Editor::Field("DirectionAngle (deg)", _comp.directionAngle, 0.5f, 0.0f, 180.0f);
		}

		Engine::Editor::Separator();

		// ---- アセット選択(既存踏襲) ----
		// ロードではなくキャッシュ参照で解決したいので、選択だけを共通ヘルパーに任せる
		Engine::Editor::HandleInfo(_comp.particlesAssetHandle);
		GUID _selectedGUID = {};
		if (Engine::Editor::AssetPicker(
			*a_context.pWorld->RefEngineServices(),
			"Change Particle",
			"ParticlesAsset",
			_comp.particleGUID,
			_selectedGUID))
		{
			_comp.particlesAssetHandle = a_context.pWorld->RefEngineServices()->pResourceManager->GetCache<Resource::ParticlesAsset>(_selectedGUID);
			_comp.particleGUID = _selectedGUID;
		}

		Engine::Editor::Separator();

		// ---- 火花(発動時 / 終了時のワンショット) ----
		// 本体と同じ発生源から、同じフレームに同時に出る
		if (Engine::Editor::CollapsingHeader("Spark (Start / End)"))
		{
			Engine::Editor::Field("EmitSparkOnStart", _comp.emitSparkOnStart);
			Engine::Editor::Field("EmitSparkOnEnd", _comp.emitSparkOnEnd);

			Engine::Editor::Field("SparkEmitCount", _comp.sparkEmitCount, 1, 0);

			Engine::Editor::Field("SparkBaseScale", _comp.sparkBaseScale, 0.05f, 0.0f);
			Engine::Editor::Field("SparkMinScale", _comp.sparkMinScale, 0.01f, 0.0f);
			Engine::Editor::Field("SparkMaxScale", _comp.sparkMaxScale, 0.01f, 0.0f);
			Engine::Editor::Field("SparkPositionRadius", _comp.sparkPositionRadius, 0.05f, 0.0f);
			Engine::Editor::Field("SparkDirectionAngle (deg)", _comp.sparkDirectionAngle, 0.5f, 0.0f);

			Engine::Editor::HandleInfo(_comp.sparkAssetHandle);
			GUID _selectedSparkGUID = {};
			if (Engine::Editor::AssetPicker(
				*a_context.pWorld->RefEngineServices(),
				"Change Spark Particle",
				"ParticlesAsset",
				_comp.sparkGUID,
				_selectedSparkGUID))
			{
				_comp.sparkAssetHandle = a_context.pWorld->RefEngineServices()->pResourceManager->GetCache<Resource::ParticlesAsset>(_selectedSparkGUID);
				_comp.sparkGUID = _selectedSparkGUID;
			}
		}

		// ---- ランタイム状態(参考) ----
		Engine::Editor::Separator();
		Engine::Editor::HelpText("isPlay:%d  pending:%d  spark:%d  time:%.2f", _comp.isPlay ? 1 : 0, _comp.pendingEmitCount, _comp.pendingSparkEmitCount, _comp.time);
	}
};
