#pragma once

#include "RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderPassRegistry;


	// リソースバリア
	struct RGBarrier
	{
		Handle<Resource::Texture> texHandle = {};
		D3D12_RESOURCE_STATES before		= D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after			= D3D12_RESOURCE_STATE_COMMON;
		Engine::Resource::ID resID			= Engine::Resource::Limits::INVALID_ID;
		bool isRead							= false;
	};

	struct CompiledPass
	{
		// パス
		RenderPassNode* pNode = nullptr;

		// バリア
		std::vector<RGBarrier> barrierVec = {};

		// RTV・DSVチェンジ用
		std::vector<Handle<D3D12::RTV>> rtvHadles = {};
		Handle<D3D12::DSV> dsvHandle = {};

		// RTV・DSVクリア用
		std::vector<Handle<Resource::Texture>> clearRTVs = {};
		bool isDepthClear = false;
	};

	class RGResourceManager;

	class RenderGraph
	{
	public:

		RenderGraph();
		~RenderGraph();

		void Init(RenderPassRegistry* a_pRenderPassRegister);

		void Release();

		void Compile();													// Pass追加後
		void Excute(GraphicsEngine* a_pGE,RenderContext* a_pCtx);		// パスを順次実行

		void AddPassNode(const EDrawPhase& a_pahse,const RenderPassNode& a_node);
		void AddPassNode(const EDrawPhase& a_pahse,RenderPassNode* a_pNode);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const std::string& a_name);
		Handle<D3D12::SRV> GetSRVHandle(const std::string& a_name);
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPU(const std::string& a_name);
		Handle<D3D12::UAV> GetUAVHandle(const std::string& a_name,bool a_read = false);
		D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPU(const std::string& a_name, bool a_read = false);

		Resource::ID Read(const std::string& a_resourceName, const AccessType& a_type);
		Resource::ID Write(const std::string& a_resourceName, const AccessType& a_type);

		Resource::ID GetID(const std::string& a_resourceName);
		Handle<Resource::Texture> GetTexHandle(const std::string& a_resourceName);

		uint8_t GetPassIndex(const std::string& a_passName);
		const RenderPassNode* GetPass(const std::string& a_passName);

		// アクセサ
		DXGI_FORMAT GetDXGIFormat(Resource::ID a_id);	// フォーマット取得
		std::vector<std::string> GetRGResourceList();	// リソース名一覧

		// パスの使用するフォーマットを取得
		// コピーされるのでランタイム中索引には使わない
		std::vector<DXGI_FORMAT> GetPassRTVFormats(uint8_t a_passIndex);
		DXGI_FORMAT GetPassDSVFormat(uint8_t a_passIndex);

		UINT GetTemporalIndex() const; // テンポラルインデックス取得

		// リソースマネージャーにアクセス
		const RGResourceManager* GetRGResourceManager() const;
		RGResourceManager* RefRGResourceManager();
	private:

		// 実行中の関数
		void AutoBarrier(RenderContext* a_pCtx,CompiledPass& a_pass);		// バリア更新

		// テンポラルインデックス更新 : フレーム用テンポラル
		void Swap();

		// =========================================================
		// コンパイル時関数
		void CreateResource();

	private:

		// テンポラル用インデックス
		UINT m_temporalIndex = 0;

		// パスデータ格納
		std::map<EDrawPhase, std::vector<RenderPassNode*>> m_pPassNodeMap;
		std::vector<RenderPassNode*> m_sortedPassed = {};							// ソート後のパス

		// コンパイル後のパス
		std::vector<CompiledPass> m_compiledPasses = {};

		// リソース管理
		std::unique_ptr<RGResourceManager> m_upRGResourceManager = nullptr;
	};
}