#pragma once

#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Graphics/CBData.h"

namespace App::Object
{
	/// <summary>
	/// シーンの環境設定 : シーンに一つ置く
	/// </summary>
	/// <remarks>
	/// 環境光・平行光・フォグ・空を「シーンの持ち物」として一か所にまとめたもの。
	///
	/// もともとこれらはグラフィックス設定(OptionPanel から GraphicsEngine の
	/// AmbientData を直接いじる形)にあったが、それだとプロジェクトに1組しか持てず、
	/// 昼のステージと夜のステージを作り分けられなかった。
	/// シーンと一緒に保存されるオブジェクトへ移すことで、シーンごとに天候を持てる。
	///
	/// 空はスカイドームのメッシュを置くのをやめ、ここのスカイテクスチャ(正距円筒)を
	/// スカイパスが方向から直接引く形にした。ドームの形(地平線の高さ・半径)と
	/// 方位の回転もここが持つ。
	///
	/// このオブジェクトが持っているだけでは絵は変わらない。毎フレーム
	/// GraphicsEngine へ流し込むところまでが仕事で、実際に使うのは各レンダーパス。
	/// </remarks>
	class SceneAmbientObject : public Engine::GameObject::BaseObject
	{
	public:

		// 置いた直後から絵になるよう、平行光だけ既定値を入れておく
		// (AmbientData の素の既定値は全部 0 なので、そのままだと真っ暗になる)
		SceneAmbientObject();

		// 描画処理 : 保持している設定を GraphicsEngine へ流し込む
		//
		// Update ではなく Draw に置いてある。Draw はエディター編集中も毎フレーム
		// レンダーグラフの直前に走るので、プレイしていなくてもシーンの空と光が出る
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : スカイテクスチャの貸し出しを取り消す
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "SceneAmbient"; }

		// インスペクター : 環境光・平行光・フォグ・空の設定
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// 保持している設定を GraphicsEngine へ送る
		void Apply(Engine::GameObject::ObjectContext& a_context);

		// インスペクターの各セクション
		void DrawLightingInspector();
		void DrawFogInspector();
		void DrawSkyInspector(Engine::GameObject::ObjectContext& a_context);

	private:

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		// 環境光・平行光・フォグ。定数バッファそのままの形で持つ
		// (シェーダーへ送る単位と分けても、二重に持ち替えるだけなので合わせてある)
		Engine::Graphics::AmbientData m_ambient = {};

		// 空の見え方(露出 / 地平線の高さ / 仮想ドームの半径 / 方位の回転)
		Engine::Graphics::SkyData m_sky = {};

		// スカイテクスチャ(正距円筒。横:縦 = 2:1 のもの)
		Engine::GUID m_skyTexGUID = {};

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		// テクスチャの実体はこちらが握る。GraphicsEngine へはハンドルだけ貸す
		Engine::ResourceRef<Engine::Resource::Texture> m_skyTexRef = {};
	};
}
