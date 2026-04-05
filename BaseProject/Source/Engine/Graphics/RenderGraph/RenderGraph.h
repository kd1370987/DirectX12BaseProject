#pragma once

#include "../RenderPass/RenderPass.h"

namespace Engine::Graphics
{
	//struct ResourceDesc
	//{
	//	std::string name = "none";
	//	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	//	uint32_t widht = 1280;
	//	uint32_t height = 720;

	//	Resource::TextureUsage usage = Resource::TextureUsage::None;
	//};

	// 描画先リソース
	//struct RGResource
	//{
	//	// このリソースの識別ID
	//	Resource::ID id = Resource::Limits::INVALID_ID;
	//	Resource::Handle<Resource::Texture> texHandle = {};

	//	// このリソースの設定
	//	ResourceDesc desc = {};

	//	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
	//};

	struct RGBarrier
	{
		Resource::Handle<Resource::Texture> texHandle = {};
		D3D12_RESOURCE_STATES before = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after = D3D12_RESOURCE_STATE_COMMON;
		Engine::Resource::ID resID = Engine::Resource::Limits::INVALID_ID;
	};

	struct CompiledPass
	{
		// パス
		RenderPass* pPass = nullptr;

		// バリア
		std::vector<RGBarrier> barrierVec = {};

		// RTV・DSVチェンジ用
		std::vector<Resource::Handle<RTV>> rtvHadles = {};
		Resource::Handle<DSV> dsvHandle = {};

		// RTV・DSVクリア用
		std::vector<Resource::Handle<Resource::Texture>> clearRTVs = {};
		bool isDepthClear = false;
	};

	class RGResourceManager;

	class RenderGraph
	{
	public:

		RenderGraph();
		~RenderGraph();

		void Init(
			RenderContext* a_pCtx,
			Resource::ShaderManager* a_pShaderMana,
			RootSignatureManager* a_pRootSigMana,
			Engine::D3D12::GraphicsPSOManager* a_pPSOMana
		);							// 初回

		void Release();

		void Compile();							// Pass追加後
		void Excute(RenderContext* a_pCtx);		// パスを順次実行

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const std::string& a_name);
		// リソース作成
		Engine::Resource::Handle<Engine::Resource::Texture> CreateTexture(
			const std::string& a_name,
			const DXGI_FORMAT& format,
			const UINT64& a_widht,
			const UINT& a_height,
			const Resource::TextureUsage& a_texUsage
		);

		Resource::ID Read(const std::string& a_resourceName, const AccessType& a_type);
		Resource::ID Write(const std::string& a_resourceName, const AccessType& a_type);

		// パス登録
		template<typename Pass>
		void RegisterPass()
		{
			std::shared_ptr<RenderPass> _pass = std::make_shared<Pass>();
			m_spPassVec.push_back(_pass);
		}

		// アクセサ
		DXGI_FORMAT GetDXGIFormat(Resource::ID a_id);	// フォーマット取得
		std::vector<std::string> GetRGResourceList();	// リソース名一覧
	private:

		// 実行中の関数
		void AutoBarrier(CompiledPass& a_pass);		// バリア更新

	private:

		// パスの保管場所
		std::vector<std::shared_ptr<RenderPass>> m_spPassVec = {};
		std::vector<RenderPass*> m_sortedPassed = {};							// ソート後のパス

		// コンパイル後のパス
		std::vector<CompiledPass> m_compiledPasses = {};

		// リソース管理
		std::unique_ptr<RGResourceManager> m_upRGResourceManager = nullptr;

		RenderContext* m_pCtx = nullptr;
	};
}