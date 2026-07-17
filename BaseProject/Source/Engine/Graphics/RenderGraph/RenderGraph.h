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

	// =========================================================
	// リソースバリア（コンパイル時に計算済みのもの）
	// =========================================================
	struct RGBarrier
	{
		D3D12::GPUResource* pResource		= nullptr;
		D3D12_RESOURCE_STATES before		= D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES after			= D3D12_RESOURCE_STATE_COMMON;

		bool isUAVBarrier					= false;
	};

	// =========================================================
	// コンパイル済みバインド : ルートパラメータと焼き込み済みディスクリプタの対応
	// =========================================================
	struct RGCompiledBind
	{
		ERGBindType	type		= ERGBindType::SrvTable;
		UINT		rootIndex	= 0;
		uint16_t	firstHandle	= 0;	// descriptorTable への開始添字
		uint16_t	count		= 1;
	};

	// =========================================================
	// コンパイル済みパス : 実行時に必要なデータを全てキャッシュ
	// =========================================================
	struct CompiledPass
	{
		RenderPassNode* pNode = nullptr;

		// このパスの「実行直前」に張るバリアのリスト
		std::vector<RGBarrier> preBarriers = {};

		// RTV・DSVチェンジ用（D3D12のCPUハンドルを直接持つ）
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles = {};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = { 0 };
		bool hasDSV = false;

		// RTV・DSVクリア用（クリアカラー等を保持）
		std::vector<size_t> clearRtvIndices = {};
		bool isDepthClear = false;

		// ---- 宣言から焼き込んだ実行時データ ----
		// pNode->accesses と一対一で並ぶ、解決済みの物理リソース
		std::vector<D3D12::GPUResource*> resources = {};

		// pNode->accesses と一対一で並ぶCPUディスクリプタ。
		// テーブルは宣言が連続している前提なので、そのまま連続領域として渡せる
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> descriptorTable = {};

		// 実行前にグラフが張るバインド
		std::vector<RGCompiledBind> binds = {};

		// グラフが直接実行するコピー（src, dst）
		std::vector<std::pair<D3D12::GPUResource*, D3D12::GPUResource*>> copies = {};
	};

	// =========================================================
	// executeFunc に渡すパスのビュー
	//
	// ビルド時に受け取った RGResourceRef で O(1) 参照する。
	// 実行時に文字列でリソースを引くことはもう無い
	// =========================================================
	class RGPassResources
	{
	public:

		RGPassResources(const CompiledPass* a_pPass)
			: m_pPass(a_pPass) {}

		// 描画キューのソートキー用インデックス
		uint8_t PassIndex() const { return m_pPass->pNode->passIndex; }

		// 物理リソース
		D3D12::GPUResource* Resource(RGResourceRef a_ref) const
		{
			return m_pPass->resources[a_ref.index];
		}

		// 解決済みCPUディスクリプタ（アクセスタイプに対応したもの）
		D3D12_CPU_DESCRIPTOR_HANDLE Descriptor(RGResourceRef a_ref) const
		{
			return m_pPass->descriptorTable[a_ref.index];
		}

		// バインドレス用のディスクリプタインデックス
		Handle<D3D12::SRV> SRVHandle(RGResourceRef a_ref) const
		{
			return Resource(a_ref)->GetSRV();
		}
		Handle<D3D12::UAV> UAVHandle(RGResourceRef a_ref) const
		{
			return Resource(a_ref)->GetUAV();
		}
		int BindlessSRVIndex(RGResourceRef a_ref) const
		{
			return static_cast<int>(Resource(a_ref)->GetSRV().GetIndex());
		}

	private:

		const CompiledPass* m_pPass = nullptr;
	};

	class RGResourceManager;

	class RenderGraph
	{
	public:

		RenderGraph();
		~RenderGraph();

		void Init(RenderPassRegistry* a_pRenderPassRegister);
		void Release();

		// パス追加後、依存関係を整理してバリアを計算する
		void Compile();

		// 毎フレーム呼ばれる実行関数
		void Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx);

		// リソースマネージャーにアクセス
		const RGResourceManager* GetRGResourceManager() const;
		RGResourceManager* RefRGResourceManager();

		UINT GetTemporalIndex() const { return m_tempralIndex; } // テンポラルインデックス取得

		// エディター用変数
		Resource::Texture* GetTmepTexture(const std::string& a_texNmae);

		RenderPassNode* GetPass(UINT a_passHash);

	private:
		// テンポラルインデックス更新 : フレーム用テンポラル
		void Swap();

		// コンパイル時の内部処理
		void CreateResource();					// パスからリソースを生成
		void TopologicalSort();					// パスの依存関係を解決
		void ResolveResourceHandles();			// 文字列からRGResourceHandleへ変換
		void ComputeBarriersAndVersions();		// バリア計算とバージョンUP
		void ResolveBindings();					// 宣言をCPUディスクリプタまで焼き込む

		// このフレームでパスを実行するか（偶奇のみの静的判定）
		bool IsPassActive(const RenderPassNode* a_pNode) const;

		// 宣言済みのルートシグネチャ・PSO・ヒープ・バインドを張る
		void ApplyStaticBindings(RenderContext* a_pCtx, const CompiledPass& a_pass);

	private:

		UINT m_tempralIndex = 0;

		// パスデータ格納
		std::map<EDrawPhase, std::vector<RenderPassNode*>> m_pPassNodeMap;
		std::vector<RenderPassNode*> m_sortedPasses = {};

		// コンパイル後の実行用データ
		std::vector<CompiledPass> m_compiledPasses = {};

		// リソース管理
		std::unique_ptr<RGResourceManager> m_upRGResourceManager = nullptr;
	};
}
