#include "RootSignatureManager.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"

void RootSignatureManager::Init()
{
	auto _spRootSig = std::make_shared<RootSignature>();
	if (!_spRootSig->Create({
		{RootParameterType::RootCBV,{}},
		{RootParameterType::RootCBV,{}},
		{RootParameterType::RootCBV,{}},
		{RootParameterType::RootCBV,{}},
		{RootParameterType::DescriptorTable,{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV}},
		})
		)
	{
		assert(0 && "ルートシグネチャの生成に失敗");
		return;
	}

	m_rootSigStorage.Add(m_id, _spRootSig);
	m_id++;
}

std::shared_ptr<RootSignature> RootSignatureManager::Get(RootSigID a_id)
{
	return m_rootSigStorage.Get(a_id);
}

ID3D12RootSignature* RootSignatureManager::NGet(RootSigID a_id)
{
	return m_rootSigStorage.Get(a_id)->Get();
}
