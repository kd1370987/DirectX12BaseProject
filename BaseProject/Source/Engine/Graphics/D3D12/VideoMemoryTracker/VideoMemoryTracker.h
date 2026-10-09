#pragma once
namespace Engine::Graphics::D3D12
{
	//==========================================================================================
	// ビデオメモリの用途
	//
	// 作ったリソースやヒープを、どの用途の持ち物として数えるか。
	// 付け忘れたものは Other に入るので、Other が大きければ付け先を探す目印になる
	//==========================================================================================
	enum class EVideoMemoryCategory : uint8_t
	{
		Other,				// 用途を付けていないもの
		MeshBuffer,			// メッシュを詰め込むメガバッファ
		Texture,			// ファイルから読んだテクスチャ・フォント
		RenderTarget,		// 実行中に作るテクスチャ(カメラの出力・モニターなど)
		RenderGraph,		// レンダーグラフの中間リソースとそのヒープ
		BackBuffer,			// スワップチェインのバックバッファ
		BLAS,				// レイトレの BLAS 本体
		BLASScratch,		// BLAS のビルド・更新用スクラッチ
		RayWorld,			// TLAS・シェーダーテーブル・インスタンス情報
		Particle,			// GPU パーティクル
		ConstantBuffer,		// フレームごとの定数バッファ
		Upload,				// 転送用の中間バッファ

		Count,
	};

	// 表示用の名前
	const char* ToString(EVideoMemoryCategory a_category);

	// 用途 1 つぶんの使用量。単位はバイト
	struct VideoMemoryCategoryUsage
	{
		uint64_t localBytes = 0;		// VRAM(DEFAULT ヒープなど)
		uint64_t nonLocalBytes = 0;		// システムメモリ側(UPLOAD / READBACK ヒープなど)
		uint32_t objectCount = 0;		// 数えているリソース・ヒープの数
	};

	// 用途ごとの使用量の一覧。並びは EVideoMemoryCategory と同じ
	struct VideoMemoryBreakdown
	{
		bool isValid = false;			// デバイスに集計先が付いていなければ false
		std::array<VideoMemoryCategoryUsage, static_cast<size_t>(EVideoMemoryCategory::Count)> categories = {};
	};

	//==========================================================================================
	// ビデオメモリの用途別の集計
	//
	// 作ったリソースに「解放されたら集計から引く札」を付けておく。
	// 札は D3D12 の private data としてリソースに持たせるので、
	// 最後の参照がどこで外れても(遅延解放・ComPtr の上書き・スワップチェインの作り直し)
	// リソースが消えたときに必ず引かれる。解放する側は何もしなくてよい。
	//
	// 集計表はデバイスに付ける。持ち主はデバイスで、寿命もデバイスと同じ。
	// 札が集計表を参照で握るので、デバイスより後にリソースが消えても集計表は残っている。
	// 作る側はリソースからデバイスを引けるので、デバイスや集計表を受け渡す必要がない
	//
	// 数えるのは committed リソースとヒープだけ。
	// placed リソースは置き先のヒープを数えているので、二重に数えないよう対象から外す。
	// DXGI の使用量との差は、ここで数えていないもの
	// (ディスクリプタヒープ・PSO・コマンドアロケーター・ドライバ内部など)になる
	//==========================================================================================
	class VideoMemoryTracker
	{
	public:

		// デバイスに集計表を付ける。デバイスを作った直後に 1 回呼ぶ
		static void AttachTo(Device* a_pDevice);

		// committed リソースを数える。
		// 同じリソースへもう一度呼ぶと、前の用途から引いて新しい用途へ付け替える。
		// placed リソース・集計表の無いデバイスのリソースは何もしない
		static void TrackResource(ID3D12Resource* a_pResource, EVideoMemoryCategory a_category);

		// ヒープを数える。ここへ置く placed リソースはこのヒープのぶんに含まれる
		static void TrackHeap(ID3D12Heap* a_pHeap, EVideoMemoryCategory a_category);

		// 今の用途別の使用量
		static VideoMemoryBreakdown Collect(Device* a_pDevice);
	};
}
