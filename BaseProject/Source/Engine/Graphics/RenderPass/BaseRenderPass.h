#pragma once
namespace Engine::Resource
{
	class ShaderManager;
}

class RootSignatureManager;
namespace Engine::D3D12
{
	class GraphicsPSOManager;
}
namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class RenderContext;

	// 作成構造体
	struct PassInitDesc
	{
		RenderGraph* pRG = nullptr;
		Resource::ShaderManager* pShaderMana = nullptr;
		RootSignatureManager* pRootSigMana = nullptr;
		Engine::D3D12::GraphicsPSOManager* pPSOMana = nullptr;
	};

	// パス内での区別用Enum
	enum class ERenderType
	{
		Static,
		Animation,
	};

	// 描画用パスクラス
	class BaseRenderPass
	{
	public:

		BaseRenderPass() = default;
		virtual ~BaseRenderPass() = default;

		// パスの初期化
		void Init(const PassInitDesc& a_initDesc);

		// 実行
		virtual void Excute(RenderContext* a_ctx) = 0;

	protected:
		// 作成ヘルパー
		void SetPassName(const std::string& a_name);

		void AddRead(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);
		void AddRead(const std::string& a_texName);
		virtual Engine::Resource::ID AddWrite(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);

		virtual void CreatePass() = 0;

	protected:

		// パス名
		std::string m_name;

		// 依存関係
		std::vector<Engine::Resource::ID> m_read = {};		// 入力
		std::vector<Engine::Resource::ID> m_write = {};		// 出力

		// レンダーパス開始・終了時のAPI設定
		std::vector<AccessResource> m_resourceAccessVec = {};

		// マネージャーやオーナーのキャッシュ
		RenderGraph* m_pRG = nullptr;
		Resource::ShaderManager* m_pShaderMana = nullptr;
		RootSignatureManager* m_pRootSigMana = nullptr;
		Engine::D3D12::GraphicsPSOManager* m_pPSOMana = nullptr;
	};
}