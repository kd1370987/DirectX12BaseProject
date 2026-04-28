#pragma once

#include "../RenderPass/BaseRenderPass.h"

namespace Engine::Graphics
{

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
		BaseRenderPass* pPass = nullptr;

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
			Resource::ShaderManager* a_pShaderMana,
			RootSignatureManager* a_pRootSigMana,
			Engine::D3D12::GraphicsPSOManager* a_pPSOMana
		);

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

		Resource::ID GetID(const std::string& a_resourceName);
		Resource::Handle<Resource::Texture> GetTexHandle(const std::string& a_resourceName);

		// パス登録
		template<typename Pass>
		void RegisterPass()
		{
			std::shared_ptr<BaseRenderPass> _pass = std::make_shared<Pass>();
			m_spPassVec.push_back(_pass);
		}

		// アクセサ
		DXGI_FORMAT GetDXGIFormat(Resource::ID a_id);	// フォーマット取得
		std::vector<std::string> GetRGResourceList();	// リソース名一覧
	private:

		// 実行中の関数
		void AutoBarrier(RenderContext* a_pCtx,CompiledPass& a_pass);		// バリア更新

	private:

		// パスの保管場所
		std::vector<std::shared_ptr<BaseRenderPass>> m_spPassVec = {};
		std::vector<BaseRenderPass*> m_sortedPassed = {};							// ソート後のパス

		// コンパイル後のパス
		std::vector<CompiledPass> m_compiledPasses = {};

		// リソース管理
		std::unique_ptr<RGResourceManager> m_upRGResourceManager = nullptr;
	};
}