#pragma once
//==========================================================================================
//
// PhysicalResource (Engine::Graphics::Pipeline)
//
// リソースの実体
// パスで使用するGPUリソース。
// パス内でアウトプットとして指定されるRTは作成されるが、
// 外部のフレームリソースのバッファなどは参照をもらい受ける。
//
// 今は仮想リソース1つにつきこれを1つ作る(1:1)。
// 生存区間の重ならないリソースを使い回すエイリアシングは、動くようになってから入れる。
//
// 配列の持ち主は RenderGraph。マネージャークラスは置かない。
//
//==========================================================================================
#include "../Resource/VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	class PhysicalResource
	{
	public:

		PhysicalResource() = default;
		~PhysicalResource() = default;

		// 実体を抱えるのでコピー禁止
		PhysicalResource(const PhysicalResource&) = delete;
		PhysicalResource& operator=(const PhysicalResource&) = delete;

		//----------------------------------------------------------------------------------
		// 生成 / 参照
		//----------------------------------------------------------------------------------
		// 仮想リソースの要件どおりに実体を作る
		bool Create(D3D12::Device* a_pDevice, const VirtualResource& a_virtual);

		// 外部で作られたリソースを参照するだけ(実体は持たない)
		void Import(D3D12::GPUResource* a_pResource);

		// 今持っている実体を、指定の要件のまま使い回せるか。
		// フォーマットやサイズが変わっていたら作り直しになる
		bool IsMatch(const VirtualResource& a_virtual) const;

		// ID3D12Resource とディスクリプタを明示的に手放す。
		// DescriptorHeapManager の解放より前に呼ぶこと
		void Release();

		//----------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------
		D3D12::GPUResource* RefResource() const { return m_pResource; }
		Resource::Texture* RefTexture() const { return m_upTexture.get(); }
		D3D12::GPUBuffer* RefBuffer() const { return m_upBuffer.get(); }

		bool IsOutsideResource() const { return m_isOutsideResource; }
		bool IsBuffer() const { return m_isBuffer; }

	private:

		// 外部の参照しているリソースかどうか
		bool m_isOutsideResource = false;
		D3D12::GPUResource* m_pResource = nullptr;

		// 作成したデータ
		bool m_isBuffer = false;									// バッファかどうか
		std::unique_ptr<Resource::Texture> m_upTexture = nullptr;
		std::unique_ptr<D3D12::GPUBuffer> m_upBuffer = nullptr;

		// 作り直しの判定用に、作ったときの要件を覚えておく
		DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
		UINT64 m_width = 0;			// バッファのときはバイト数
		UINT m_height = 0;
		Resource::TextureUsage m_usage = Resource::TextureUsage::None;
	};
}
