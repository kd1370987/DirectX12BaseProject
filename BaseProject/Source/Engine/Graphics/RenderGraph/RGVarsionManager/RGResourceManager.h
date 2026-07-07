#pragma once

namespace Engine::Graphics
{
	// リソースタイプ
	enum class ERGResourceType {
		Texture,
		Buffer
	};

	

	// レンダーグラフで使うリソースの抽象化IDを管理する
	class RGResourceManager
	{
	public:

		RGResourceManager() = default;
		~RGResourceManager() = default;

		// ==========================================================
		// 一時リソース : グラフ内で実体を作成・管理するもの
		// ==========================================================
		void DeclareTexture(
			const std::string& a_name,
			const DXGI_FORMAT& a_format,
			const UINT64& a_width,
			const UINT& a_height,
			const Resource::TextureUsage& a_usage,
			const DXSM::Color& a_clearColor = { 0, 0, 0, 1 }
		);

		void DeclareBuffer(
			const std::string& a_name,
			const UINT64& a_sizeBytes
		);

		// ==========================================================
		// 外部リソース : グラフ外で作成され、状態遷移のみ管理するもの
		// ==========================================================
		void ImportResource(
			ERGResourceType a_type,
			const std::string& a_name,
			D3D12::GPUResource* a_pExternalResource,
			D3D12_RESOURCE_STATES a_initialState
		);

		// ==========================================================
		// 依存関係の構築 (バージョン進行) : ソート用ではないので、パスからは呼ばずに
		// コンパイル時のリソース依存のみを扱う
		// ==========================================================
		RGResourceHandle Read(const std::string& a_name);
		RGResourceHandle Write(const std::string& a_name);

		// ==========================================================
		// グラフのコンパイルと実行管理
		// ==========================================================
		// 宣言された一時リソースの実体（D3D12Resource）を生成、またはプールから割り当てる
		void AllocateResources(D3D12::Device* a_pDevice);

		// フレーム終了時に論理リソースの設定をリセットする
		// （物理リソースの配列は維持して次フレームで使い回す）
		void ResetForNextFrame();

		// フレーム終了時にリソースを初期状態に戻す
		void ResetForNextFrame(D3D12::GraphicsCommandList* a_pCmdList);


		// ==========================================================
		// アクセサ (バリア構築やパス実行時に使用)
		// ==========================================================
		RGResourceHandle GetHandle(const std::string& a_name) const;

		// RTV/DSV/SRV/UAVなどの生成は、実体である D3D12Resource から直接取得する形にする
		D3D12::GPUResource* GetPhysicalResource(RGResourceHandle a_handle) const;

		D3D12_RESOURCE_STATES GetCurrentState(RGResourceHandle a_handle) const;
		void SetCurrentState(RGResourceHandle a_handle, D3D12_RESOURCE_STATES a_newState);

		DXGI_FORMAT GetDXGIFormat(RGResourceHandle a_handle) const;

	private:


		// 論理リソース
		struct LogicalResource
		{
			// リソーステクスチャの作成情報
			std::string name = "none";
			ERGResourceType type = ERGResourceType::Texture;

			// --- 要件 (Declareされた場合のみ有効) ---
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			UINT64 width = 0;
			UINT height = 0;
			Resource::TextureUsage usage = Resource::TextureUsage::None;
			DXSM::Color clearColor = { 0, 0, 0, 1 };

			// --- 状態管理 ---
			Resource::Generation currentVersion = 0;
			D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

			// --- 物理リソースとの紐付け ---
			bool isImported = false;
			D3D12::GPUResource* pPhysicalResource = nullptr;
		};

		// リソース参照
		const LogicalResource& GetRes(RGResourceHandle a_handle) const;
		LogicalResource& RefRes(RGResourceHandle a_handle);

	private:

		// 倫理リソースの管理
		std::unordered_map<std::string, uint32_t> m_nameMap = {};
		std::vector<LogicalResource> m_logicalResourceVec = {};

		// 一時リソースの実態管理
		std::vector<std::unique_ptr<Resource::Texture>> m_tempTextures;
		std::vector< std::unique_ptr<D3D12::GPUBuffer>> m_tempBuffers;
	};
};