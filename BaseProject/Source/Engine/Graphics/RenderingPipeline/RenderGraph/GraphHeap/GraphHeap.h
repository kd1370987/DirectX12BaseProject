#pragma once
namespace Engine::Graphics::Pipeline
{
	/// <summary>
	/// レンダーグラフに持たせるヒープ
	///
	/// リソースを発行する際にエイリアシングできるものはこのヒープに割り当てる
	/// </summary>
	class GraphHeap
	{
	public:

		GraphHeap() = default;
		~GraphHeap() = default;

		// 実体を抱えるのでコピー禁止
		GraphHeap(const GraphHeap&) = delete;
		GraphHeap& operator=(const GraphHeap&) = delete;

		//----------------------------------------------------------------------------------
		// 必要なぶんのヒープを用意する
		//
		// 戻り値は「このヒープへ置ける状態になったか」。
		// false のときはエイリアシングを諦めて、各リソースを個別に作ること(committed)。
		//   ・置くものが無い(サイズ0)
		//   ・Resource Heap Tier 1 のハード(種別の違うリソースを混ぜられない)
		//   ・ヒープの作成に失敗した
		//
		// 大きさが変わっていなければ作り直さない。
		// 毎回作り直すと、GPUがまだ読んでいる実体の下からメモリが消える
		//----------------------------------------------------------------------------------
		bool Create(D3D12::Device* a_pDevice,UINT64 a_maxHeapSize);

		// 実体を手放す。
		// このヒープに置いたリソースは参照を握っているので、
		// 先にそちらを解放してから呼ぶこと
		void Release();

		// 置ける状態か
		bool IsValid() const { return m_cpHeap != nullptr; }

		//----------------------------------------------------------------------------------
		// 1つのヒープに種別の違うリソースを混ぜられるか(Resource Heap Tier 2 以上)
		//
		// Tier 1 では、ヒープごとに「バッファだけ」「RT/DSテクスチャだけ」
		// 「それ以外のテクスチャだけ」のどれかを宣言しないといけない。
		// グラフのリソースは3種類とも出てくるので、混ぜられないなら諦める
		//----------------------------------------------------------------------------------
		static bool IsAliasingSupported(D3D12::Device* a_pDevice);

		// 参照
		ID3D12Heap* RefHeap() const { return m_cpHeap.Get(); }
		UINT64 GetMaxHeapSize() const { return m_maxHeapSize; }

	private:

		ComPtr<ID3D12Heap> m_cpHeap = nullptr;
		UINT64 m_maxHeapSize = 0;
	};
}