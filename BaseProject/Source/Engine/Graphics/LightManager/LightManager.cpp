#include "Engine/Graphics/LightManager/LightManager.h"

namespace Engine::Graphics
{
	//---------------------------------------------------------------------------------------
	// FrameLightData
	//---------------------------------------------------------------------------------------

	bool FrameLightData::Create(D3D12::Device* a_pDevice, D3D12::DescriptorHeapManager* a_pHeapManager)
	{
		// 要素数は上限固定で確保する
		// ライトが増えるたびにバッファを作り直すと、GPU が読んでいる最中のリソースを
		// 開放することになるため、最初から最大数ぶん取っておく
		if (!dlBuffer.Create(a_pDevice, a_pHeapManager, MAX_DIRECTIONAL_LIGHTS)) return false;
		if (!plBuffer.Create(a_pDevice, a_pHeapManager, MAX_POINT_LIGHTS)) return false;

		return true;
	}

	void FrameLightData::Release()
	{
		dlBuffer.Release();
		plBuffer.Release();

		dlCount = 0;
		plCount = 0;
	}

	//---------------------------------------------------------------------------------------
	// LightManager
	//---------------------------------------------------------------------------------------

	void LightManager::Init()
	{
		// 上限ぶんを先に取っておき、ライト追加のたびに再確保が走らないようにする
		m_directionalLightPool.Reserve(MAX_DIRECTIONAL_LIGHTS);
		m_pointLightPool.Reserve(MAX_POINT_LIGHTS);

		// 詰め直しは毎フレーム走るので、作業配列も同じく先に確保しておく
		m_dlWorkVec.reserve(MAX_DIRECTIONAL_LIGHTS);
		m_plWorkVec.reserve(MAX_POINT_LIGHTS);
	}

	void LightManager::Release()
	{
		// プールを空にする
		// ここを通した時点で配り済みのハンドルはすべて無効になるため、
		// 呼ぶのはシーンの切れ目など、ライトの持ち主ごと消えるタイミングに限ること
		m_directionalLightPool.Release();
		m_pointLightPool.Release();

		m_dlWorkVec.clear();
		m_plWorkVec.clear();
	}

	Handle<DirectionalLight> LightManager::AllocateDL()
	{
		// 上限を超えたぶんは BuildFrameData() で GPU バッファに載らず、
		// 無言で描画から落ちる。ここで弾いて呼び出し側に気づかせる
		if (CountAliveLights(m_directionalLightPool) >= MAX_DIRECTIONAL_LIGHTS)
		{
			ENGINE_WARNING("ディレクショナルライトが上限(%u)に達しています", MAX_DIRECTIONAL_LIGHTS);
			return {};
		}

		return m_directionalLightPool.Add(DirectionalLight{});
	}

	Handle<PointLight> LightManager::AllocatePL()
	{
		// 上限の扱いは AllocateDL() と同じ
		if (CountAliveLights(m_pointLightPool) >= MAX_POINT_LIGHTS)
		{
			ENGINE_WARNING("ポイントライトが上限(%u)に達しています", MAX_POINT_LIGHTS);
			return {};
		}

		return m_pointLightPool.Add(PointLight{});
	}

	const std::vector<DirectionalLight>& LightManager::GetFrameDirectionalLights() const
	{
		return m_dlWorkVec;
	}

	SunLightCB LightManager::GetSunLightCB() const
	{
		SunLightCB _cb = {};

		// 先頭が主光源。
		// 平行光を置くのはシーンの環境設定(SceneManager が1つだけ借りる)なので、ここは実質そのライト。
		// 2つ目以降を足した場合、影を落とすのはあくまで先頭の1つだけになる
		if (m_dlWorkVec.empty()) return _cb;

		const DirectionalLight& _dl = m_dlWorkVec.front();
		_cb.dir        = _dl.dir;
		_cb.brightness = _dl.brightness;
		_cb.color      = _dl.color;
		_cb.enable     = 1;

		return _cb;
	}

	//---------------------------------------------------------------------------------------
	// 主光源の影(シャドウマップのカスケード)
	//---------------------------------------------------------------------------------------
	namespace
	{
		// 箱の中心を寄せる格子の細かさ(箱の1辺をこの数で割る)
		//
		// カメラが動くたびに箱がテクセルの端数だけずれると、影の縁がちらつく(シマリング)。
		// 中心を「テクセルの整数倍」の格子に乗せればずれは起きない。
		// 格子がシャドウマップのテクセルより粗ければ整数倍になるので、
		// 解像度(パスの持ち物)を知らなくて済むよう、あり得る最小の解像度で割っておく
		constexpr float SNAP_DIVISION = 512.0f;

		// 次のカスケードへ混ぜる幅(カスケードの奥行きに対する割合)。
		// 混ぜないと、カスケードの境目で影の粗さが急に変わって線が見える
		constexpr float CASCADE_BLEND_RATE = 0.1f;

		// 背面カリング用のカメラ位置を、光源側へどれだけ離すか。
		// 増幅シェーダーは「カメラ位置からメッシュレットへの向き」で裏表を判定するので、
		// 平行光として扱えるだけ遠くへ置く(近いと箱の端で向きがずれ、影を落とす面まで間引く)
		constexpr float CULL_EYE_DISTANCE = 100000.0f;
	}

	void LightManager::BuildShadowCascades(const CameraData& a_camera)
	{
		m_sunShadowCB = {};

		const DirectionalShadowSettings& _settings = m_shadowSettings;

		// 影の届く範囲はカスケードを組まないフレームでも入れておく。
		// レイトレのフォグ用の影は低解像度で作るので、フォグが自分の画素の範囲を求めるのに使う
		m_sunShadowCB.distance = _settings.distance;

		if (_settings.mode != EDirectionalShadowMode::ShadowMap) return;

		// 平行光が無ければ影の落としようがない
		if (m_dlWorkVec.empty()) return;

		Math::Vector3 _lightDir = m_dlWorkVec.front().dir;
		if (_lightDir.LengthSquared() < 1e-8f) return;
		_lightDir = _lightDir.Normalized();

		//------------------------------------------------------------------
		// カメラの近い面・遠い面と画角を射影行列から取り出す
		//
		// 左手系 : 透視は _33 = f/(f-n) / 平行投影は _33 = 1/(f-n)。どちらも _43 = -n * _33
		//------------------------------------------------------------------
		const Math::Matrix& _proj = a_camera.projMat;
		if (_proj._11 == 0.0f || _proj._22 == 0.0f || _proj._33 == 0.0f) return;

		const bool _isPerspective = (_proj._34 != 0.0f);

		const float _nearZ = -_proj._43 / _proj._33;
		float _farZ = FLT_MAX;
		if (!_isPerspective)			_farZ = _nearZ + 1.0f / _proj._33;
		else if (_proj._33 != 1.0f)		_farZ = _proj._33 * _nearZ / (_proj._33 - 1.0f);

		const float _shadowFar = std::min(_settings.distance, _farZ);
		if (!(_shadowFar > _nearZ)) return;

		// 奥行き d での画面の半分の幅と高さ : 透視は d に比例、平行投影は一定
		const float _halfW = 1.0f / _proj._11;
		const float _halfH = 1.0f / _proj._22;

		// 奥行きの区間 [a_near, a_far] を覆う視錐台の角8つ(ワールド)
		auto _calcSliceCorners = [&](float a_near, float a_far, Math::Vector3(&a_outCorners)[8])
			{
				constexpr float SIGNS[2] = { -1.0f, 1.0f };
				const float _depths[2] = { a_near, a_far };

				int _index = 0;
				for (float _depth : _depths)
				{
					const float _w = _isPerspective ? _halfW * _depth : _halfW;
					const float _h = _isPerspective ? _halfH * _depth : _halfH;

					for (float _sy : SIGNS)
					{
						for (float _sx : SIGNS)
						{
							const Math::Vector3 _viewPos(_w * _sx, _h * _sy, _depth);
							a_outCorners[_index++] = Math::Vector3::Transform(_viewPos, a_camera.viewInvMat);
						}
					}
				}
			};

		//------------------------------------------------------------------
		// 光源の向き
		//
		// 真上/真下から照らすときは Y を上にできないので Z を使う
		//------------------------------------------------------------------
		const Math::Vector3 _up = (std::abs(_lightDir.y) > 0.99f)
			? Math::Vector3(0.0f, 0.0f, 1.0f)
			: Math::Vector3(0.0f, 1.0f, 0.0f);

		// 原点から光の向きを見る回転 : 箱の中心を格子へ寄せるのに使う
		const Math::Matrix _lightRot = Math::Matrix::CreateLookAt(Math::Vector3(), _lightDir, _up);
		const Math::Matrix _lightRotInv = _lightRot.Invert();

		//------------------------------------------------------------------
		// カスケードごとに箱を作る
		//------------------------------------------------------------------
		const uint32_t _count = std::clamp(_settings.cascadeCount, 1u, MAX_SHADOW_CASCADES);
		const float _lambda = std::clamp(_settings.splitLambda, 0.0f, 1.0f);
		const float _backDistance = std::max(_settings.casterDistance, 0.0f);

		// 対数の区切りは近い面が 0 だと割れないので、そのときは等間隔にする
		const bool _canLogSplit = (_nearZ > 0.0f);

		float _cascadeFar[MAX_SHADOW_CASCADES] = {};
		float _cascadeBlendStart[MAX_SHADOW_CASCADES] = {};
		float _cascadeRadius[MAX_SHADOW_CASCADES] = {};
		float _cascadeDepthRange[MAX_SHADOW_CASCADES] = {};

		float _sliceNear = _nearZ;	// この段の箱が覆い始める奥行き
		float _prevFar = _nearZ;	// 1つ前の段の受け持ちの終わり

		for (uint32_t _i = 0; _i < _count; ++_i)
		{
			// 受け持ちの終わり : 等間隔と対数を _lambda で混ぜる(最後の段は影の届く端)
			const float _t = static_cast<float>(_i + 1) / static_cast<float>(_count);
			const float _uniform = _nearZ + (_shadowFar - _nearZ) * _t;
			const float _log = _canLogSplit ? _nearZ * std::pow(_shadowFar / _nearZ, _t) : _uniform;
			const float _far = (_i + 1 == _count) ? _shadowFar : (_uniform + (_log - _uniform) * _lambda);

			// 最後の段は「影なし」へ混ぜて、影の届く端をぼかす
			const float _blendStart = _far - (_far - _prevFar) * CASCADE_BLEND_RATE;

			// ---- 区間を覆う球 ----
			// 箱ではなく球で囲むのは、カメラが回っても大きさが変わらないようにするため。
			// 大きさが変わるとテクセルの大きさも毎フレーム変わり、影の縁がちらつく
			Math::Vector3 _corners[8] = {};
			_calcSliceCorners(_sliceNear, _far, _corners);

			Math::Vector3 _center = {};
			for (const auto& _corner : _corners) _center += _corner;
			_center /= 8.0f;

			float _radius = 0.0f;
			for (const auto& _corner : _corners)
			{
				_radius = std::max(_radius, Math::Vector3::Distance(_center, _corner));
			}

			// 半径の端数でもテクセルの大きさが揺れるので、1/16 単位へ切り上げる
			_radius = std::max(std::ceil(_radius * 16.0f) / 16.0f, 0.0625f);

			// ---- 中心を格子へ寄せる(光源から見た平面上で) ----
			const float _grid = (_radius * 2.0f) / SNAP_DIVISION;
			Math::Vector3 _centerLS = Math::Vector3::Transform(_center, _lightRot);
			_centerLS.x = std::floor(_centerLS.x / _grid) * _grid;
			_centerLS.y = std::floor(_centerLS.y / _grid) * _grid;
			_center = Math::Vector3::Transform(_centerLS, _lightRotInv);

			// ---- ライトのカメラ ----
			// 球の手前から、さらに _backDistance だけ光源側へ下がったところから見る。
			// 画面の外にある遮蔽物(背後の崖など)もこれで箱に入る
			const float _depthRange = _backDistance + _radius * 2.0f;
			const Math::Vector3 _eye = _center - _lightDir * (_backDistance + _radius);

			const Math::Matrix _view = Math::Matrix::CreateLookAt(_eye, _center, _up);
			const Math::Matrix _orthoProj = Math::Matrix::CreateOrthographicOffCenter(
				-_radius, _radius, -_radius, _radius, 0.0f, _depthRange);
			const Math::Matrix _viewProj = _view * _orthoProj;

			// 読む側
			m_sunShadowCB.lightViewProj[_i] = _viewProj.Transpose();
			_cascadeFar[_i] = _far;
			_cascadeBlendStart[_i] = _blendStart;
			_cascadeRadius[_i] = _radius;
			_cascadeDepthRange[_i] = _depthRange;

			// 描く側 : メッシュシェーダーはカメラの定数バッファをそのまま読むので、同じ形で詰める。
			// ジッターも前フレームも無いので、どれも今の行列で埋める
			CameraData& _cam = m_shadowCameraArr[_i];
			_cam = {};
			_cam.viewMat = _view.Transpose();
			_cam.projMat = _orthoProj.Transpose();
			_cam.viewInvMat = _view.Invert().Transpose();
			_cam.projInvMat = _orthoProj.Invert().Transpose();
			_cam.viewProjMat = _viewProj.Transpose();
			_cam.invViewProjMat = _viewProj.Invert().Transpose();
			_cam.nonJitteredProj = _cam.projMat;
			_cam.nonJitteredViewProj = _cam.viewProjMat;
			_cam.nonJitteredInvViewProj = _cam.invViewProjMat;
			_cam.prevView = _cam.viewMat;
			_cam.prevProj = _cam.projMat;
			_cam.prevViewProj = _cam.viewProjMat;

			const Math::Vector3 _cullEye = _center - _lightDir * CULL_EYE_DISTANCE;
			_cam.pos = Math::Vector4(_cullEye.x, _cullEye.y, _cullEye.z, 1.0f);

			// 視錐台カリングの面 : 箱の6面になる
			_cam.ExtractFrustumPlanes(_viewProj);

			// 増幅シェーダーはカリング用カメラを読むので、この段そのものを入れる。
			// (空のままだと面が全部 0 になって何も間引かれず、裏面判定は原点から見てしまう)
			_cam.UseSelfAsCullCamera();

			// 次の段は、この段が混ぜ始めるところから覆う(混ぜる区間では両方を引くため)
			_sliceNear = _blendStart;
			_prevFar = _far;
		}

		auto _toVector4 = [](const float(&a_values)[MAX_SHADOW_CASCADES])
			{
				return Math::Vector4(a_values[0], a_values[1], a_values[2], a_values[3]);
			};

		m_sunShadowCB.cascadeFar = _toVector4(_cascadeFar);
		m_sunShadowCB.cascadeBlendStart = _toVector4(_cascadeBlendStart);
		m_sunShadowCB.cascadeRadius = _toVector4(_cascadeRadius);
		m_sunShadowCB.cascadeDepthRange = _toVector4(_cascadeDepthRange);

		m_sunShadowCB.lightDir = _lightDir;
		m_sunShadowCB.cascadeCount = _count;
		m_sunShadowCB.depthBias = _settings.depthBias;
		m_sunShadowCB.normalBias = _settings.normalBias;
		m_sunShadowCB.softness = std::max(_settings.softness, 0.0f);
	}

	void LightManager::BuildFrameData(FrameLightData& a_frameData)
	{
		// プールの穴を詰めて作業配列へ集める
		a_frameData.dlCount = GatherLights(m_directionalLightPool, m_dlWorkVec, MAX_DIRECTIONAL_LIGHTS);
		a_frameData.plCount = GatherLights(m_pointLightPool, m_plWorkVec, MAX_POINT_LIGHTS);

		// 書き込みオフセットを毎フレーム先頭へ戻す
		// 戻さないと AllocateAndWrite がフレームごとに後方へ書き進み、
		// 先頭(element0)からバインドしているシェーダーは初回フレームのライトを読み続ける
		// (RenderContext のボーン/UIバッファと同じ運用)
		a_frameData.dlBuffer.ResetForNewFrame();
		a_frameData.plBuffer.ResetForNewFrame();

		// 0件なら書き込まない : シェーダーへは Count = 0 が渡るので参照されない
		if (a_frameData.dlCount > 0)
		{
			a_frameData.dlBuffer.AllocateAndWrite(m_dlWorkVec);
		}
		if (a_frameData.plCount > 0)
		{
			a_frameData.plBuffer.AllocateAndWrite(m_plWorkVec);
		}
	}
}

