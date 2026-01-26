#include "GraphicsPSOManager.h"

#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"

void GraphicsPSOManager::Init(
	const UINT& a_slotSize,
	std::shared_ptr<ShaderManager> a_spShaderManager,
	std::shared_ptr<RootSignatureManager> a_spRootSigManager
)
{
	m_psoSlot.Init(a_slotSize);

	m_wpShaderManager = a_spShaderManager;
	m_wpRootSigManager = a_spRootSigManager;
}

Resource::ID GraphicsPSOManager::Register(const std::string& a_key, const PSOSetting& a_setting)
{
	if (m_wpShaderManager.expired())
	{
		assert(0 && "シェーダーマネージャーが設定されていません");
		return 0;
	}
	if (m_wpRootSigManager.expired())
	{
		assert(0 && "ルートシグネチャマネージャーが設定されていません");
		return 0;
	}

	auto _spShaderManager = m_wpShaderManager.lock();
	auto _spRootSigManager = m_wpRootSigManager.lock();

	// PSO作成
	std::shared_ptr<PipelineState> _spPSO = std::make_shared<PipelineState>();

	auto _layout = _spShaderManager->NGet(a_setting.vsStage)->vsInputLayout;
	_spPSO->SetInputLayout(_layout);
	auto _vs = _spShaderManager->NGet(a_setting.vsStage)->byteCode;
	_spPSO->SetVS(_vs);
	auto _ps = _spShaderManager->NGet(a_setting.psStage)->byteCode;
	_spPSO->SetPS(_ps);

	_spPSO->SetRootSignature(_spRootSigManager->NGet(a_setting.rootsignatureID));

	_spPSO->Create();

	// 登録
	return m_psoSlot.Add(a_key,_spPSO);
}

ID3D12PipelineState* GraphicsPSOManager::NGet(const Resource::ID& a_id)
{
	return m_psoSlot.Ref(a_id)->Get();
}

