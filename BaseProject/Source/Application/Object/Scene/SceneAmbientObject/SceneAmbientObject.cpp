#include "SceneAmbientObject.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectAsset/EffectAsset.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../../../Engine/ECS/World/World.h"

#include "Application/InstanceResource/SingletonEntityResource.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Utility/EffectSpawnHelper.h"

//==========================================================================================
// SceneAmbientObject
//
// シーンの環境設定(環境光・平行光・フォグ・空)をまとめて持ち、毎フレーム
// GraphicsEngine へ流し込むだけのオブジェクト。
//
// ・なぜシーンの持ち物にしたか
//     もとはグラフィックス設定(OptionPanel)から GraphicsEngine の AmbientData を
//     直接いじっていたので、プロジェクトに1組しか持てなかった。
//     シーンと一緒に保存されるオブジェクトにすれば、ステージごとに天候を作り分けられる。
//
// ・空はメッシュを置かない
//     スカイドームのモデルをシーンに置く代わりに、ここのスカイテクスチャ(正距円筒)を
//     SkyPass が「そのピクセルが見ている方向」から直接引く。
//     ドームの形(地平線の高さ・半径)と方位の回転もここが持っていて、
//     ドームを置いたのと同じ見え方を値だけで作る。
//
// ・テクスチャの所有はこちら
//     GraphicsEngine へ渡すのはハンドルだけ。シーンが変わってこのオブジェクトが
//     消えるときに空のハンドルを渡し直して、参照を残さないようにする。
//
// ・空気中のチリだけは「送る」ではなく「出す」
//     チリはレンダーパスの設定ではなくエフェクトなので、ECS のエンティティを
//     1つ作って持ち続け、その位置を毎フレームカメラへ寄せる。
//     出したエンティティの始末もこのオブジェクトの仕事(Release で返す)。
//==========================================================================================
namespace App::Object
{
	SceneAmbientObject::SceneAmbientObject()
	{
		// GraphicsEngine の初期値と同じものを入れておく。
		// 置いた瞬間に真っ暗になると「壊れている」ように見えるため
		m_ambient.ambientColorScale = { 0.0f, 0.0f, 0.0f };
		m_dlDir                     = { 0.5f, -1.0f, 0.5f };
		m_dlColor                   = { 4.0f, 4.0f, 4.0f };
	}

	//======================================================================================
	// 更新 : チリをカメラへ追従させる
	//
	// カメラが center から length より離れたら、speed で「length だけ離れたところ」まで詰める。
	// 詰め切ったら止まるので、チリの塊はカメラの後ろを引きずるように付いてくる。
	//
	// ・追従は距離で判定する。毎フレーム無条件にカメラへ寄せると、
	//   チリがカメラと一緒に動いてしまい「自分が進んでいる感じ」が出ない。
	// ・カメラが見つからないフレームは何もしない(その場に置いたままにする)。
	//   シーンの切り替え中など、一瞬カメラが居ないフレームがあるため
	//======================================================================================
	void SceneAmbientObject::Update(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pWorld) return;

		// 未設定なら出しているものを片付けて終わり
		if (m_dast.effectGUID == Engine::DefaultGUID)
		{
			ReleaseDastEntity(a_context);
			return;
		}

		Math::Vector3 _camPos = {};
		if (!TryGetMainCameraPos(a_context, _camPos)) return;

		if (!m_isDastCentered)
		{
			// 初回はカメラの位置へ置く。
			// ここを追従に任せると、原点からカメラまでの距離を延々となめることになる
			m_dast.center = _camPos;
			m_isDastCentered = true;
		}
		else
		{
			const Math::Vector3 _toCam = _camPos - m_dast.center;
			const float _dist = _toCam.Length();
			const float _length = std::max(m_dast.length, 0.0f);

			if (_dist > _length && _dist > 1e-6f)
			{
				// 詰めてよいのは「length だけ離れたところ」まで。
				// speed が 0 以下なら追従の意味が無いので、その場で詰め切る
				const float _remain = _dist - _length;
				const float _step = (m_dast.speed > 0.0f)
					? std::min(m_dast.speed * a_context.dt, _remain)
					: _remain;

				m_dast.center = m_dast.center + (_toCam / _dist) * _step;
			}
		}

		EnsureDastEntity(a_context);
		ApplyDastToEntity(a_context);
	}

	void SceneAmbientObject::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		Apply(a_context);
	}

	//======================================================================================
	// 解放 : 貸していたスカイテクスチャのハンドルを返す
	//
	// ハンドルを残したままシーンを切り替えると、次のシーンにアンビエントオブジェクトが
	// 無い場合に前のシーンの空が描かれ続けてしまう
	//======================================================================================
	void SceneAmbientObject::Release(Engine::GameObject::ObjectContext& a_context)
	{
		// 出しているチリも一緒に片付ける。
		// 置いていくと、シーンを抜けたあとも空中にチリだけが残る
		ReleaseDastEntity(a_context);

		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		_pGE->SetSkyTexture({});

		// 平行光の席を返す。
		// 返さないままシーンを読み直すと席が減り、最後は上限に達して太陽が出なくなる
		if (m_dlHandle.IsValid())
		{
			_pGE->RefLightManager()->RemoveLight(m_dlHandle);
			m_dlHandle = {};
		}
	}

	//======================================================================================
	// 保持している設定を GraphicsEngine へ送る
	//
	// 送るだけで、どう使うかは各レンダーパスの担当。
	//   AmbientData … ディファードライティング / GI / 影
	//   SkyData ＋ テクスチャ … スカイパス
	//======================================================================================
	void SceneAmbientObject::Apply(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		_pGE->SetAmbientData(m_ambient);
		_pGE->SetSkyData(m_sky);
		_pGE->SetSkyTexture(m_skyTexRef);

		//----------------------------------------------------------------------------
		// 平行光
		//
		// 席が無ければ取る。生成直後だけでなく、シーンを読み直した後もここを通る
		// (ハンドルは保存しないため)。
		// 上限に達していると無効が返るので、そのフレームは何もしない
		//----------------------------------------------------------------------------
		auto* _pLightManager = _pGE->RefLightManager();
		if (!m_dlHandle.IsValid())
		{
			m_dlHandle = _pLightManager->AllocateDL();
		}

		if (auto* _pLight = _pLightManager->RefLight(m_dlHandle))
		{
			_pLight->dir   = { m_dlDir.x, m_dlDir.y, m_dlDir.z };
			_pLight->color = { m_dlColor.x, m_dlColor.y, m_dlColor.z, 1.0f };

			// 色を 1.0 超えで持たせる従来の形をそのまま残すため、強さは掛けない。
			// (ポイントライトのように色と明るさを分けたくなったらここへ欄を足す)
			_pLight->brightness = 1.0f;
		}
	}

	void SceneAmbientObject::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// ---- 環境光・平行光 ----
		a_ar.Field("AmbientColor", m_ambient.ambientColorScale);
		a_ar.Field("DLDir", m_dlDir);
		a_ar.Field("DLColor", m_dlColor);

		// ---- 高さフォグ ----
		a_ar.Field("HeightFogColor", m_ambient.heightFogColor);
		a_ar.Field("HeightFogMaxRange", m_ambient.heightFogMaxRange);
		a_ar.Field("HeightFogHeight", m_ambient.heightFogHeight);
		a_ar.Field("HeightFogEnable", m_ambient.heightFogEnable);
		a_ar.Field("HeightFogDenseDown", m_ambient.heightFogDenseDown);

		// ---- 距離フォグ ----
		a_ar.Field("DistanceFogColor", m_ambient.distanceFogColor);
		a_ar.Field("DistanceFogMaxRange", m_ambient.distanceFogMaxRange);
		a_ar.Field("DistanceFogStart", m_ambient.distanceFogStart);
		a_ar.Field("DistanceFogEnable", m_ambient.distanceFogEnable);

		// ---- 空 ----
		a_ar.GUIDField("SkyTexGUID", m_skyTexGUID);
		a_ar.Field("SkyExposure", m_sky.exposure);
		a_ar.Field("SkyHorizonHeight", m_sky.horizonHeight);
		a_ar.Field("SkyRadius", m_sky.radius);
		a_ar.Field("SkyRotationDeg", m_sky.rotationDeg);
		a_ar.Field("IsSkyDof", m_sky.isSkyDof);
		a_ar.Field("SkyDofScale", m_sky.dofScale);

		// ---- 空気中のチリ ----
		// center は追従の結果なので保存しない(最初のフレームでカメラの位置へ置き直す)
		a_ar.GUIDField("DastEffectGUID", m_dast.effectGUID);
		a_ar.Field("DastColorScale", m_dast.m_colorScale);
		a_ar.Field("DastScale", m_dast.scale);
		a_ar.Field("DastFollowLength", m_dast.length);
		a_ar.Field("DastFollowSpeed", m_dast.speed);

		// 読み込み時は復元したGUIDでテクスチャを引き直す。
		// 実体が届くのを待つ必要はないので要求だけ出して先へ進む。
		// スカイパスは実体が引けないフレームは何も描かない
		if (a_ar.IsLoading())
		{
			// 読み込み直後はまだカメラの位置が分からないので、
			// 追従の初期化からやり直させる(最初のフレームでカメラへ置かれる)
			m_isDastCentered = false;

			if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

			// チリのエフェクト。実体を使うのはエンティティ側(EffectAssetComponent)なので、
			// こちらが握るのはインスペクターの表示用 ＋ 読み込みを始めさせるため
			if (m_dast.effectGUID != Engine::DefaultGUID)
			{
				m_dast.m_effectAsset =
					a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::EffectAsset>(m_dast.effectGUID);
			}

			if (!m_skyTexGUID.IsValid()) return;

			m_skyTexRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_skyTexGUID);
		}
	}

	//======================================================================================
	// メインカメラのワールド座標
	//
	// どれがメインカメラかは MainCameraSystem が決めて
	// SingletonEntityResource へ置いてあるので、ここでは探さずに引くだけ
	//======================================================================================
	bool SceneAmbientObject::TryGetMainCameraPos(
		Engine::GameObject::ObjectContext& a_context, Math::Vector3& a_outPos) const
	{
		if (!a_context.pWorld) return false;
		if (!a_context.pWorld->HasResource<SingletonEntityResource>()) return false;

		const Engine::ECS::Entity _camera =
			a_context.pWorld->GetResource<SingletonEntityResource>().mainCamera;

		if (!a_context.pWorld->IsAliveEntity(_camera)) return false;
		if (!a_context.pWorld->HasComponent<WorldMatrixComponent>(_camera)) return false;

		const auto* _pWorldMat = a_context.pWorld->RefData<WorldMatrixComponent>(_camera);
		if (!_pWorldMat) return false;

		a_outPos = Math::Matrix(_pWorldMat->worldMat).Translation();
		return true;
	}

	//======================================================================================
	// チリのエンティティを用意する
	//
	// 出しっぱなしのエフェクトなので destroyOnFinish は立てない。
	// 消すのはこちらの都合(設定の差し替え・シーンの終わり)だけ。
	//
	// エフェクトを差し替えられたら作り直す。中身を書き換えても
	// アセットのハンドルを取り直すのは PostDeserialize なので、
	// 作り直したほうが素直に済む
	//======================================================================================
	void SceneAmbientObject::EnsureDastEntity(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pWorld) return;

		// 生きていて、しかも今の設定で出したものならそのまま使う
		if (a_context.pWorld->IsAliveEntity(m_dastEntity) &&
			m_dastSpawnedGUID == m_dast.effectGUID)
		{
			return;
		}

		// 古いものを片付ける(もう居なければ何も起きない)
		ReleaseDastEntity(a_context);

		// 即時生成で作る。GameObject の Update は ECS の反復の外なので呼んでよい。
		// 遅延生成だとエンティティが返らず、追従させる相手を握れない
		m_dastEntity = App::Utility::SpawnEffectAtNow(
			*a_context.pWorld,
			m_dast.effectGUID,
			m_dast.center,
			false,						// 出し切っても消さない(寿命はこちらが握る)
			{},							// 向きはアセットのパーツ側に任せる
			std::max(m_dast.scale, 0.01f));

		m_dastSpawnedGUID = (m_dastEntity != Engine::ECS::Limits::INVALID_ENTITY)
			? m_dast.effectGUID
			: Engine::DefaultGUID;
	}

	//======================================================================================
	// 追従の結果をチリのエンティティへ書き込む
	//
	// ワールド行列も一緒に入れる。GameObject の Update は CalcMatrixSystem(PostUpdate)の
	// 後に走るので、ローカル座標だけ書いても絵に反映されるのは次のフレームになる。
	// チリはカメラと一緒に動く相手なので、その1フレームがそのまま置いていかれた見た目になる
	//======================================================================================
	void SceneAmbientObject::ApplyDastToEntity(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pWorld) return;
		if (!a_context.pWorld->IsAliveEntity(m_dastEntity)) return;

		if (a_context.pWorld->HasComponent<LocalTransformComponent>(m_dastEntity))
		{
			if (auto* _pTrs = a_context.pWorld->RefData<LocalTransformComponent>(m_dastEntity))
			{
				_pTrs->pos = m_dast.center;
				_pTrs->isDirty = true;
			}
		}

		if (a_context.pWorld->HasComponent<WorldMatrixComponent>(m_dastEntity))
		{
			if (auto* _pWorldMat = a_context.pWorld->RefData<WorldMatrixComponent>(m_dastEntity))
			{
				_pWorldMat->worldMat = Math::Matrix::CreateTranslation(m_dast.center);
			}
		}

		// 出現空間の広さ。エフェクト全体の倍率なので、
		// ばらつき半径と一緒に粒の大きさにも掛かる(EffectDrawSystem)
		if (a_context.pWorld->HasComponent<EffectAssetComponent>(m_dastEntity))
		{
			if (auto* _pEffect = a_context.pWorld->RefData<EffectAssetComponent>(m_dastEntity))
			{
				_pEffect->effectScale = std::max(m_dast.scale, 0.01f);
			}
		}
	}

	//======================================================================================
	// チリのエンティティを手放す
	//
	// 解放予約だけ。実際に消えるのは次の BeginFrame で、その前に Release フェーズが
	// 走るのでパーティクルの発生源の席も返ってから消える
	//======================================================================================
	void SceneAmbientObject::ReleaseDastEntity(Engine::GameObject::ObjectContext& a_context)
	{
		if (a_context.pWorld && a_context.pWorld->IsAliveEntity(m_dastEntity))
		{
			a_context.pWorld->AddReleaseEntity(m_dastEntity);
		}

		m_dastEntity = Engine::ECS::Limits::INVALID_ENTITY;
		m_dastSpawnedGUID = Engine::DefaultGUID;
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void SceneAmbientObject::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::TextDisabled("シーン全体の環境設定。シーンに1つだけ置く");
		ImGui::Separator();

		DrawLightingInspector();
		DrawFogInspector();
		DrawSkyInspector(a_context);
		DrawDastInspector(a_context);

		// 触った値をその場で反映させる(次のDrawを待たずに絵が変わる)
		Apply(a_context);
	}

	void SceneAmbientObject::DrawLightingInspector()
	{
		ImGui::SeparatorText("Lighting");

		ImGui::DragFloat3("AmbientColor", &m_ambient.ambientColorScale.x, 0.01f);
		ImGui::DragFloat3("DLColor", &m_dlColor.x, 0.01f);
		ImGui::DragFloat3("DLDir", &m_dlDir.x, 0.01f);
		if (!m_dlHandle.IsValid())
		{
			ImGui::TextDisabled("(平行光の席が取れていません : 上限かも)");
		}
	}

	void SceneAmbientObject::DrawFogInspector()
	{
		// enable が false の間はシェーダー側で計算ごとスキップされる
		ImGui::SeparatorText("HeightFog");
		{
			bool _enable = (m_ambient.heightFogEnable != 0);
			if (ImGui::Checkbox("HeightFogEnable", &_enable))
			{
				m_ambient.heightFogEnable = _enable ? 1 : 0;
			}

			ImGui::ColorEdit3("HeightFogColor", &m_ambient.heightFogColor.x);
			ImGui::DragFloat("HeightFogHeight", &m_ambient.heightFogHeight, 0.1f);
			// 基準高さからこの距離だけ進むと 100%
			ImGui::DragFloat("HeightFogMaxRange", &m_ambient.heightFogMaxRange, 0.1f, 0.0f);

			// どちら側へ濃くしていくか
			static const char* _denseName[] = { "Upward", "Downward" };
			int _denseDown = m_ambient.heightFogDenseDown != 0 ? 1 : 0;
			if (ImGui::Combo("HeightFogDense", &_denseDown, _denseName, IM_ARRAYSIZE(_denseName)))
			{
				m_ambient.heightFogDenseDown = _denseDown;
			}
		}

		ImGui::SeparatorText("DistanceFog");
		{
			bool _enable = (m_ambient.distanceFogEnable != 0);
			if (ImGui::Checkbox("DistanceFogEnable", &_enable))
			{
				m_ambient.distanceFogEnable = _enable ? 1 : 0;
			}

			ImGui::ColorEdit3("DistanceFogColor", &m_ambient.distanceFogColor.x);
			ImGui::DragFloat("DistanceFogStart", &m_ambient.distanceFogStart, 0.1f, 0.0f);
			// この距離で 100%。開始距離より手前には下げられないようにしておく
			ImGui::DragFloat("DistanceFogMaxRange", &m_ambient.distanceFogMaxRange, 0.1f,
				m_ambient.distanceFogStart, FLT_MAX);
		}
	}

	void SceneAmbientObject::DrawSkyInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("Sky");

		if (!a_context.pServices || !a_context.pServices->pResourceManager)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "ResourceManager is null");
			return;
		}

		// 正距円筒(横:縦 = 2:1)のテクスチャを想定している。
		// 選ぶだけで空になるので、スカイドームのモデルは置かなくてよい
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Sky Texture", "Texture", m_skyTexGUID))
		{
			m_skyTexRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_skyTexGUID);
		}
		Engine::Editor::EditorHelper::DrawTexture(m_skyTexRef, 256, 128);

		if (!m_skyTexGUID.IsValid())
		{
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f),
				"(Sky Texture 未設定 : 空は描かれません)");
		}

		ImGui::Spacing();

		// 露出 : 出力先がHDRなので 1.0 を超えて構わない。
		// 超えた分はブルームの抽出しきい値に乗り、最後にトーンマップで落ちる
		ImGui::DragFloat("Exposure", &m_sky.exposure, 0.01f, 0.0f, 100.0f);

		// 地平線の高さ = 仮想ドームの中心の高さ。カメラがここから離れると地平線が動く
		ImGui::DragFloat("HorizonHeight", &m_sky.horizonHeight, 0.1f);

		// 半径 : 小さいほどカメラの上下で地平線が強く動く。
		// 十分大きく取ると、ほぼ無限遠のスカイボックスと同じ見え方になる
		ImGui::DragFloat("Radius", &m_sky.radius, 1.0f, 0.01f, 100000.0f);
		if (m_sky.radius < 0.01f) m_sky.radius = 0.01f;

		// 方位の回転 : 空を回して太陽や雲の位置を平行光の向きに合わせる
		ImGui::DragFloat("RotationDeg", &m_sky.rotationDeg, 0.5f, -360.0f, 360.0f);
		if (m_sky.rotationDeg >= 360.0f) m_sky.rotationDeg -= 360.0f;
		if (m_sky.rotationDeg <= -360.0f) m_sky.rotationDeg += 360.0f;

		ImGui::Spacing();

		//----------------------------------------------------------------------
		// 空に被写界深度を掛けるか
		//
		// 空は深度が far のまま残るので、切っておかないと「一番遠いもの」として
		// 最大の奥ボケが掛かり、空だけがべったり滲む。
		// 掛けたいときだけ有効にして、倍率で強さを決める(1.0 で他の遠景と同じ)
		//----------------------------------------------------------------------
		bool _isSkyDof = (m_sky.isSkyDof != 0);
		if (ImGui::Checkbox("IsSkyDof", &_isSkyDof))
		{
			m_sky.isSkyDof = _isSkyDof ? 1 : 0;
		}

		ImGui::BeginDisabled(!_isSkyDof);
		ImGui::DragFloat("SkyDofScale", &m_sky.dofScale, 0.01f, 0.0f, 1.0f);
		ImGui::EndDisabled();
		if (!_isSkyDof)
		{
			ImGui::TextDisabled("(空にはボケが掛かりません)");
		}
	}

	//======================================================================================
	// 空気中のチリ
	//======================================================================================
	void SceneAmbientObject::DrawDastInspector(Engine::GameObject::ObjectContext& a_context)
	{
		ImGui::SeparatorText("Dast");

		if (!a_context.pServices || !a_context.pServices->pResourceManager)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "ResourceManager is null");
			return;
		}

		// 差し替えたら次の Update が古いエンティティを片付けて出し直す
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Dast Effect", "EffectAsset", m_dast.effectGUID))
		{
			m_dast.m_effectAsset = (m_dast.effectGUID != Engine::DefaultGUID)
				? a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::EffectAsset>(m_dast.effectGUID)
				: Engine::ResourceRef<Engine::Resource::EffectAsset>{};

			// 置き直す : 別のエフェクトはカメラの位置から出し始めたい
			m_isDastCentered = false;
		}

		if (m_dast.effectGUID == Engine::DefaultGUID)
		{
			ImGui::TextDisabled("(未設定 : チリは出ません)");
			return;
		}

		// 中身の確認だけ。絵の編集はエフェクトアセット側のインスペクターで行う。
		// ここで実体を握っているのは、シーンに入った瞬間からチリが出るように
		// 読み込みを先に始めさせるため(出すのはエンティティ側のハンドル)
		if (const auto* _pEffect = a_context.pServices->pResourceManager->Ref(m_dast.m_effectAsset))
		{
			ImGui::Text("Particle Parts : %d", static_cast<int>(_pEffect->GetParticleParts().size()));
			ImGui::Text("Mesh Parts     : %d", static_cast<int>(_pEffect->GetMeshParts().size()));
		}
		else
		{
			ImGui::TextDisabled("(読み込み中)");
		}

		ImGui::Spacing();

		// 出現空間の広さ。エフェクト全体の倍率として渡すので、
		// ばらつき半径だけでなく粒の大きさにも掛かる
		ImGui::DragFloat("Volume Scale", &m_dast.scale, 0.1f, 0.01f, 10000.0f);
		if (m_dast.scale < 0.01f) m_dast.scale = 0.01f;

		// 色スケール : まだ絵には効かない。
		// 粒の色はパーティクルアセットの定数バッファ(全員で共有)が持っているので、
		// 個体ごとに掛けるには描画側に受け口を足す必要がある
		ImGui::ColorEdit4("Color Scale", m_dast.m_colorScale.Data());
		ImGui::TextDisabled("(色はパーティクルアセット側。ここはまだ絵に反映されません)");

		ImGui::Spacing();

		// カメラがこの距離だけ離れたら追従を始める。
		// 0 にすると常にカメラへ張り付くので、進んでいる感じが出なくなる
		ImGui::DragFloat("Follow Length", &m_dast.length, 0.1f, 0.0f, 10000.0f);
		if (m_dast.length < 0.0f) m_dast.length = 0.0f;

		// 追従スピード(毎秒)。0 以下ならその場で詰め切る(＝常に張り付く)
		ImGui::DragFloat("Follow Speed", &m_dast.speed, 0.1f, 0.0f, 10000.0f);
		if (m_dast.speed < 0.0f) m_dast.speed = 0.0f;

		ImGui::Spacing();

		// 今どこに居るか。追従の具合を見るための表示なので触らせない
		ImGui::Text("Center : %.1f, %.1f, %.1f",
			m_dast.center.x, m_dast.center.y, m_dast.center.z);

		if (!a_context.pWorld || !a_context.pWorld->IsAliveEntity(m_dastEntity))
		{
			ImGui::TextDisabled("(まだ出ていません : 次の更新で出ます)");
		}

		if (ImGui::Button("Reset Center"))
		{
			// 次の更新でカメラの位置へ置き直す
			m_isDastCentered = false;
		}
	}
}
