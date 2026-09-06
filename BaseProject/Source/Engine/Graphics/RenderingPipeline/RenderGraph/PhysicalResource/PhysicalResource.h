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
// 参照しか持たないので実体は要らない
namespace Engine::Graphics::Pipeline
{
	class VirtualResource;

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
		// 仮想リソースの要件どおりに実体を作る。
		//
		// a_pHeap を渡すと、席に着いているものはその上へ置く(placed)。
		// nullptr のときと、席に着いていないものは今までどおり個別に作る(committed)
		bool Create(D3D12::Device* a_pDevice, const VirtualResource& a_virtual, ID3D12Heap* a_pHeap);

		// 外部で作られたリソースを参照するだけ(実体は持たない)
		void Import(D3D12::GPUResource* a_pResource);

		// 今持っている実体を、指定の要件のまま使い回せるか。
		// フォーマットやサイズが変わっていたら作り直しになる。
		//
		// 置き場所も見る : 席は再コンパイルのたびに動くので、
		// 要件が同じでもオフセットが変われば作り直しになる
		bool IsMatch(const VirtualResource& a_virtual, ID3D12Heap* a_pHeap) const;

		//----------------------------------------------------------------------------------
		// 実体をヒープ上へ置くかどうかを決める
		//
		// Create と IsMatch が別々に判断すると、置き場所が変わったのに
		// 「要件は同じ」で古い実体を使い回してしまうので、判断はここ1箇所に寄せる
		//----------------------------------------------------------------------------------
		static bool ResolvePlacement(const VirtualResource& a_virtual, ID3D12Heap* a_pHeap, uint64_t* a_pOutOffset);

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

		// ヒープ上に置かれているか(placed)
		bool IsPlaced() const { return m_pHeap != nullptr; }
		uint64_t GetHeapOffset() const { return m_heapOffset; }

	private:

		// 外部の参照しているリソースかどうか
		bool m_isOutsideResource = false;
		D3D12::GPUResource* m_pResource = nullptr;

		// 置き場所 : committed で作ったなら nullptr。
		// 作り直しの判定に使うので、作ったときの場所をそのまま覚えておく
		ID3D12Heap* m_pHeap = nullptr;
		uint64_t m_heapOffset = 0;

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
