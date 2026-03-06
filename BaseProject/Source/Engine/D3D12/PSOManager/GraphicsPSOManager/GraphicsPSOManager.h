#pragma once

class PipelineState;

class ShaderManager;
class RootSignatureManager;

struct PSOSetting
{
	Engine::Resource::ID vsStage = 0;
	Engine::Resource::ID psStage = 0;

	Engine::Resource::ID rootsignatureID = 0;
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
		const UINT& a_slotSize
	);

	/// <summary>
	/// PSOの作成
	/// </summary>
	/// <param name="a_setting">生成パラメタ</param>
	/// <returns>管理場所ID</returns>
	Engine::Resource::ID Register(
		const std::string& a_key,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& a_psoDesc
	);

	/// <summary>
	/// パイプラインステートのセット
	/// </summary>
	/// <param name="a_id">使用PSOID</param>
	/// <param name="a_pCmdList">コマンドリスト</param>
	ID3D12PipelineState* NGet(const Engine::Resource::ID& a_id);

private:

	SlotStorage<PipelineState> m_psoSlot;
};