#include "SceneAmbientObject.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

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
//==========================================================================================
namespace App::Object
{
	SceneAmbientObject::SceneAmbientObject()
	{
		// GraphicsEngine の初期値と同じものを入れておく。
		// 置いた瞬間に真っ暗になると「壊れている」ように見えるため
		m_ambient.ambientColorScale = { 0.0f, 0.0f, 0.0f };
		m_ambient.dlDir             = { 0.5f, -1.0f, 0.5f };
		m_ambient.dlColor           = { 4.0f, 4.0f, 4.0f };
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
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		_pGE->SetSkyTexture({});
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
	}

	void SceneAmbientObject::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// ---- 環境光・平行光 ----
		a_ar.Field("AmbientColor", m_ambient.ambientColorScale);
		a_ar.Field("DLDir", m_ambient.dlDir);
		a_ar.Field("DLColor", m_ambient.dlColor);

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

		// 読み込み時は復元したGUIDでテクスチャを引き直す。
		// 実体が届くのを待つ必要はないので要求だけ出して先へ進む。
		// スカイパスは実体が引けないフレームは何も描かない
		if (a_ar.IsLoading())
		{
			if (!m_skyTexGUID.IsValid()) return;
			if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

			m_skyTexRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_skyTexGUID);
		}
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

		// 触った値をその場で反映させる(次のDrawを待たずに絵が変わる)
		Apply(a_context);
	}

	void SceneAmbientObject::DrawLightingInspector()
	{
		ImGui::SeparatorText("Lighting");

		ImGui::DragFloat3("AmbientColor", &m_ambient.ambientColorScale.x, 0.01f);
		ImGui::DragFloat3("DLColor", &m_ambient.dlColor.x, 0.01f);
		ImGui::DragFloat3("DLDir", &m_ambient.dlDir.x, 0.01f);
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
	}
}
