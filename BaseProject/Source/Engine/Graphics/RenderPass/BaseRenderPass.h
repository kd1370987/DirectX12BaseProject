#pragma once


namespace Engine::D3D12
{
	class PipelineStateManager;
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

		D3D12::PipelineStateManager* pPipelineStateManager = nullptr;
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
		virtual void Init(const PassInitDesc& a_initDesc);

		// 実行
		virtual void Excute(RenderContext* a_ctx) = 0;

		// アクセサ
		const std::vector<Engine::Resource::ID>& GetRead() const { return m_read; }
		const std::vector<Engine::Resource::ID>& GetWrite() const { return m_write; }
		const std::vector<AccessResource>& GetResourceAccessVec() const { return m_resourceAccessVec; }

		const std::string& GetName() const { return m_name; }
		void SetPassIndex(uint8_t a_passIdx) { m_passIndex = a_passIdx; }
		uint8_t GetPassIndex() const { return m_passIndex; }

		uint8_t GetPSOIndex(const std::string& a_psoName) const;

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
		uint8_t m_passIndex = 255;
		std::unordered_map<std::string, uint8_t> m_psoIndexMap = {};

		// 依存関係
		std::vector<Engine::Resource::ID> m_read = {};		// 入力
		std::vector<Engine::Resource::ID> m_write = {};		// 出力

		// レンダーパス開始・終了時のAPI設定
		std::vector<AccessResource> m_resourceAccessVec = {};

		// マネージャーやオーナーのキャッシュ
		RenderGraph* m_pRG = nullptr;
		D3D12::PipelineStateManager* m_pPipelineStateManager = nullptr;
	};
}