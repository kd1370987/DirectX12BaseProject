#pragma once

struct PassDesc
{
	std::string name;

	UINT rootSigID;
	UINT psoID;

	std::vector<Resource::ID> readResource;		// 入力(SRV,Mesh,Vertex)
	std::vector<Resource::ID> writeResource;	// 出力(RTV,UAV)

	RenderQueueType queueType;

	bool isCulled = false;		// 依存関係的に不要ならスキップ
	bool isAsync = false;		// 将来用

	std::vector<AttachementDesc> colorAttachements;
	std::optional<AttachementDesc> depthAttachement;
};

class ShaderManager;
class RootSignatureManager;
class GraphicsPSOManager;
class RenderGraph;

class RenderContext;

class RenderPass
{
public:

	virtual ~RenderPass() = default;
	
	void Init(
		RenderGraph* a_graph,
		ShaderManager* a_pShaderMana,
		RootSignatureManager* a_pRootSigMana,
		GraphicsPSOManager* a_pPSOMana
	);

	const PassDesc& GetDesc() const
	{
		return m_passDesc;
	}

	virtual void Excute(RenderContext* a_ctx) = 0;

protected:

	virtual void CreatePass() = 0;

	PassDesc m_passDesc;

	ShaderManager* m_pShaderMana = nullptr;
	RootSignatureManager* m_pRootSigMana = nullptr;
	GraphicsPSOManager* m_pPSOMana = nullptr;
	RenderGraph* m_pRenderGraph = nullptr;
};
