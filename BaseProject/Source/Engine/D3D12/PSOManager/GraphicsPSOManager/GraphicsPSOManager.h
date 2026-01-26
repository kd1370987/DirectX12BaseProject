#pragma once

class PipelineState;

class ShaderManager;
class RootSignatureManager;

struct PSOSetting
{
	Resource::ID vsStage = 0;
	Resource::ID psStage = 0;

	Resource::ID rootsignatureID = 0;
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
		const UINT& a_slotSize,
		std::shared_ptr<ShaderManager> a_spShaderManager,
		std::shared_ptr<RootSignatureManager> a_spRootSigManager
	);

	/// <summary>
	/// PSOの作成
	/// </summary>
	/// <param name="a_setting">生成パラメタ</param>
	/// <returns>管理場所ID</returns>
	Resource::ID Register(const std::string& a_key,const PSOSetting& a_setting);

	/// <summary>
	/// パイプラインステートのセット
	/// </summary>
	/// <param name="a_id">使用PSOID</param>
	/// <param name="a_pCmdList">コマンドリスト</param>
	ID3D12PipelineState* NGet(const Resource::ID& a_id);

private:

	std::weak_ptr<ShaderManager> m_wpShaderManager;
	std::weak_ptr<RootSignatureManager> m_wpRootSigManager;

	SlotStorage<PipelineState> m_psoSlot;
};