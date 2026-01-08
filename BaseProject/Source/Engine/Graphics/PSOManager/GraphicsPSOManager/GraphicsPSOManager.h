#pragma once

using GraphicsPSOID = uint32_t;

class PipelineState;

class ShaderManager;
class RootSignatureManager;

struct PSOSetting
{
	uint32_t vsStage = 0;
	uint32_t psStage = 0;

	uint32_t rootsignatureID = 0;
};

class GraphicsPSOManager
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_spShaderManager">シェーダーマネージャーのポインタ</param>
	/// <param name="a_spRootSigManager">ルートシグネチャマネージャーのポインタ</param>
	void Init(
		std::shared_ptr<ShaderManager> a_spShaderManager,
		std::shared_ptr<RootSignatureManager> a_spRootSigManager
	);

	/// <summary>
	/// PSOの作成
	/// </summary>
	/// <param name="a_setting">生成パラメタ</param>
	/// <returns>管理場所ID</returns>
	GraphicsPSOID Register(const PSOSetting& a_setting);

	/// <summary>
	/// パイプラインステートのセット
	/// </summary>
	/// <param name="a_id">使用PSOID</param>
	/// <param name="a_pCmdList">コマンドリスト</param>
	//void SetGraphicsPSO(const GraphicsPSOID& a_id,ID3D12GraphicsCommandList* a_pCmdList);

	ID3D12PipelineState* NGet(const GraphicsPSOID& a_id);

private:

	std::weak_ptr<ShaderManager> m_wpShaderManager;
	std::weak_ptr<RootSignatureManager> m_wpRootSigManager;

	GraphicsPSOID m_id = 0;
	Storage<GraphicsPSOID, PipelineState> m_pipelineStorage;
};