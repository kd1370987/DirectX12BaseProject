#include "GraphicsPSOManager.h"

#include "Engine/GPUResource/PipeLineState/PipelineState.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/Graphics/RootSignatureManager/RootSignatureManager.h"

void GraphicsPSOManager::Init(
	std::shared_ptr<ShaderManager> a_spShaderManager,
	std::shared_ptr<RootSignatureManager> a_spRootSigManager
)
{
	m_wpShaderManager = a_spShaderManager;
	m_wpRootSigManager = a_spRootSigManager;
}

GraphicsPSOID GraphicsPSOManager::Register(const PSOSetting& a_setting)
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

	auto _layout = _spShaderManager->Get(a_setting.vsStage)->vsInputLayout;
	_spPSO->SetInputLayout(_layout);
	auto _vs = _spShaderManager->Get(a_setting.vsStage)->byteCode;
	_spPSO->SetVS(_vs);
	auto _ps = _spShaderManager->Get(a_setting.psStage)->byteCode;
	_spPSO->SetPS(_ps);

	_spPSO->SetRootSignature(_spRootSigManager->NGet(a_setting.rootsignatureID));

	_spPSO->Create();

	// 登録
	m_pipelineStorage.Add(m_id, _spPSO);
	return m_id++;
}

ID3D12PipelineState* GraphicsPSOManager::NGet(const GraphicsPSOID& a_id)
{
	return m_pipelineStorage.Get(a_id)->Get();
}
