#pragma once

namespace Engine::Graphics::Raytracing
{
	class BLASCompactor;

	//==========================================================================================
	// 静的 BLAS の実体
	//
	// 圧縮(BLASCompactor)がビルドの後で小さい実体に差し替えるので、BLAS と共有で持つ。
	// BLAS が先に消えたら中身が空になり、BLASCompactor はそれを見て手を引く。
	// ロード・描画・完了通知のどのスレッドからも触るので、中身は mutex を取ってから触る
	//==========================================================================================
	struct BLASCompactionTarget
	{
		std::mutex mutex;
		ComPtr<ID3D12Resource> cpResource = nullptr;
	};

	//==========================================================================================
	// 静的 BLAS を作るときの後始末の預け先
	//
	// どれも無ければ、スクラッチは BLAS が持ち続け、圧縮もしない(以前と同じ振る舞い)
	//==========================================================================================
	struct BLASStaticBuildOption
	{
		// ビルドが終わるまで生かしておくものの預け先。スクラッチをここへ渡して手放す
		std::vector<ComPtr<ID3D12Resource>>* pKeepAlive = nullptr;

		// ビルドが終わったら呼ぶものの預け先と、圧縮を進める係。両方そろえば圧縮する
		std::vector<std::function<void()>>* pOnBuildComplete = nullptr;
		BLASCompactor* pCompactor = nullptr;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	};

	/// <summary>
	/// メッシュのポリゴン情報を空間分割ツリーとして保持するリソース
	/// 静的も出るとスキニング等で毎フレーム変形する動的モデル用で振る舞いが変わる
	/// </summary>
	class BLAS
	{
	public:
		BLAS() = default;

		//----------------------------------------------------------------------------------
		// GPU が使っている最中のリソースをその場で手放さないための口
		//
		// BLAS とスクラッチはフレームをまたいで GPU が読む。Release() を通さずに
		// 壊したり上書きしたりすると、実行中のコマンドが参照したまま最終解放され、
		// デバッグレイヤーが CORRUPTION で止まる。
		// デストラクタとムーブ代入でも中身が残っていれば遅延解放へ回し、警告を出す
		// (警告が出たら、その経路に Release() を足すのが本来の直し方)
		//----------------------------------------------------------------------------------
		~BLAS();
		BLAS(const BLAS&) = delete;
		BLAS& operator=(const BLAS&) = delete;
		BLAS(BLAS&&) noexcept = default;
		BLAS& operator=(BLAS&& a_other) noexcept;

		// ===================================================================================
		// 静的BLASの構築
		// ===================================================================================

		/// <summary>
		/// 静的モデル用BLASの作成 : 更新不可
		/// スクラッチはビルドが終われば要らないので、預け先があれば渡して手放す。
		/// 圧縮の係が渡されていれば、ビルドの後で小さい実体に作り直す
		/// </summary>
		void CreateStatic(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
			const BLASStaticBuildOption& a_option = {}
		);


		// ===================================================================================
		// 動的BLASの構築と更新
		// ===================================================================================

		/// <summary>
		/// 動的更新可能なインスタンスBLASとして作成
		/// スキニングキャラクター等の初期化時に呼び出す
		/// </summary>
		/// <param name="a_sourceBLAS">元となる静的モデルのBLAS</param>
		/// <param name="a_animatedGeometries">変形後の頂点バッファアドレスをセットしたGeometryDesc配列</param>
		void CreateDynamic(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_animatedGeometries
		);

		/// <summary>
		/// 動的BLASの頂点座標を更新
		/// 毎フレームの SkinningPass 完了後、TLAS構築の前に呼び出す
		/// 事前に ALLOW_UPDATE フラグ付きで作成(CloneAsDynamic等)されている必要がある
		/// </summary>
		void Update(Graphics::D3D12::GraphicsCommandList* a_pCmdList);

		// ===================================================================================
		// ユーティリティ
		// ===================================================================================
		/// <summary>
		/// BLASのリソース解放 : 動的BLASの場合は更新用スクラッチも解放
		/// </summary>
		void Release();

		/// <summary>
		/// BLASの構築または更新完了を待機するUAVバリアを発行
		/// これを呼んでからTLASの構築を行わないと、GPU側で不完全なツリーを参照しクラッシュする
		/// </summary>
		void UAVBarrier(Graphics::D3D12::GraphicsCommandList* a_pCmdList) const;

		/// <summary>
		/// デバッグ用途 ： PIXやNsightでリソースを識別しやすくする名前付け
		/// </summary>
		void SetName(LPCWSTR a_name);

		// ===================================================================================
		// アクセサ
		// ===================================================================================
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
		UINT GetSubsetCount() const { return static_cast<UINT>(m_geometryDescVec.size()); }
		const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GetGeometryDesc() const { return m_geometryDescVec; }
		bool IsDynamic() const { return m_isDynamic; }

	private:

		// 今の実体。圧縮の対象なら共有している側から引く(差し替わっていればその後のもの)。
		// 返す値を使い終わるまで、a_pLock で実体の差し替えを止めておく
		ID3D12Resource* RefResultResource(std::unique_lock<std::mutex>* a_pLock) const;

		// 持っているリソースを遅延解放キューへ回して空にする。
		// a_pUnexpected が非nullなら、Release() を通らずに来た経路としてその名前で警告する
		void DeferReleaseResources(const char* a_pUnexpected);

		// 内部のビルド/アップデート共通処理
		bool BuildInternal(
			Graphics::D3D12::Device* a_pDevice,
			Graphics::D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags,
			bool a_isUpdate
		);

	private:
		// BLAS本体のリソース。圧縮の対象になった静的 BLAS では空で、実体は m_spCompactionTarget が持つ
		ComPtr<ID3D12Resource> m_cpResource = nullptr;

		// 圧縮の対象になった静的 BLAS の実体(BLASCompactor と共有)
		std::shared_ptr<BLASCompactionTarget> m_spCompactionTarget = nullptr;

		// ビルド・更新用のスクラッチバッファ。
		// 動的 BLAS は更新のたびに使うので持ち続ける。
		// 静的 BLAS は預け先があればビルドの完了まで預けて手放す(無ければ持ち続ける)
		ComPtr<ID3D12Resource> m_cpUpdateScratch = nullptr;

		// BLASを構成するジオメトリ（サブメッシュ）情報のキャッシュ
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescVec = {};

		// このBLASが動的(更新可能)として作成されたかのフラグ
		bool m_isDynamic = false;
	};
}