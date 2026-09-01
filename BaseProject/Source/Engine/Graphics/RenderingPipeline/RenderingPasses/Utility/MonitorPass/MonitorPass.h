#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// MonitorPass
	//
	// パスとパスの間にはさんで、そこを流れている絵をノードの中に出す確認用のパス。
	//
	// 入力をそのまま出力へ写すだけなので、どこへ挟んでも絵は変わらない。
	// 挟んだうえで、同じ絵をこのパスが自前で持つテクスチャへもう1枚写し、
	// それをノード内に描く。
	//
	//--------------------------------------------------------------------------------------
	// なぜグラフのリソースをそのまま出さず、自前のテクスチャへ写すのか
	//
	//   ・グラフのリソースはフレームの終わりに入口のステートへ戻される。
	//     ImGuiが描くのはそれより後なので、読める状態である保証がない
	//   ・SRVを持つかどうかは「そのリソースを誰かがSRVとして読んでいるか」で決まる。
	//     コピー先として作っただけのリソースにはImGui用のビューが無い
	//
	// 自分の持ち物にしておけば、ステートもビューもこのパスの都合で決められる。
	// 写した直後に PIXEL_SHADER_RESOURCE へ戻しておくので、
	// フレーム後半のImGuiはそのまま読める
	//
	//--------------------------------------------------------------------------------------
	// 設計図と実行インスタンス
	//
	// エディターが触っているのは設計図側のパスで、これは一度も実行されない。
	// なのでノードに出す中身は、GraphicsEngine::FindPipelinePass() を通して
	// 実行インスタンス側の同じGUIDのパスから借りてくる。
	// カメラがこのパイプラインを回していないあいだは出るものが無い
	//======================================================================================
	class MonitorPass : public Pass
	{
	public:
		~MonitorPass() override;

		void SetupSlots() override;

		// 出力の形を入力に合わせる : CopyResource は形が一致していないと通らない
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

		// コードから組むとき用 : 出力の名前を決める。
		// フォーマットと大きさは入力からもらうので指定しない
		void Configure(const std::string& a_resourceName)
		{
			m_resourceName = a_resourceName;
			ApplyOutput();
		}

		// ノードに出しているテクスチャ : 持っていなければ nullptr
		const Resource::Texture* GetPreviewTexture() const { return m_upPreviewTex.get(); }

	private:

		// スロットへ出力の設定を反映する
		void ApplyOutput();

		//----------------------------------------------------------------------------------
		// 写し先に使えるフォーマットへ直す
		//
		// 深度は型無し(R32_TYPELESS)や深度専用(D32_FLOAT)で確保されていて、
		// そのままの形では SRV を張れない = ImGuiへ渡せないし、後段も読めない。
		// 同じ型無し親を持つ型付きフォーマットへ直すと、コピーは通ったまま読めるようになる
		//----------------------------------------------------------------------------------
		static DXGI_FORMAT ToCopyableFormat(DXGI_FORMAT a_format);

		// 入力の実体に合わせてモニター用テクスチャを用意する。
		// すでに同じ形のものを持っていれば作り直さない
		void EnsurePreviewTexture(D3D12::GPUResource* a_pSource);

		// モニター用テクスチャを手放す
		void ReleasePreviewTexture();

		// ノードに出す中身を持っているパスを返す。
		// 自分が実行インスタンスならそのまま自分、設計図側なら実行インスタンスを探す
		const MonitorPass* ResolveViewSource();

	private:

		// 写し先のリソース名 : ノードごとに変える。
		// 同じ名前のモニターを2つ置くと、同じリソースへ2回書くことになるので分けること
		std::string m_resourceName = "MonitorResult";

		//----------------------------------------------------------------------------------
		// モニター用のテクスチャ(グラフの外の持ち物)
		//
		// 実行インスタンスだけが中身を持つ。設計図側は最後まで空のまま
		//----------------------------------------------------------------------------------
		std::unique_ptr<Resource::Texture> m_upPreviewTex = nullptr;

		// ノードに出すかどうか。
		// 下ろしているあいだは写しを取らないので、コピー1回ぶん軽くなる
		bool m_isPreview = true;

		// ノード内に出す幅(ピクセル)。高さは元の縦横比から出す。
		// 表示だけの値なので実行インスタンスへは配らない
		float m_previewWidth = 240.0f;
	};
}
