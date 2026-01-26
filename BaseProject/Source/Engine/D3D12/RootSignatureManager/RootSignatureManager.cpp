#include "RootSignatureManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"

void RootSignatureManager::Init(const UINT& a_slotMax)
{
	// 管理スロット確保
	m_rootStorage.Init(a_slotMax);
}

Resource::ID RootSignatureManager::Register(
	const std::string& a_key,
	const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec
)
{
	auto _spRootSig = std::make_shared<RootSignature>();
	if (!_spRootSig->Create(a_rootParamsVec))
	{
		assert(0 && "ルートシグネチャの生成に失敗");
		return Resource::Limits::MAX_STORAGE;
	}

	return m_rootStorage.Add(a_key,_spRootSig);
}

ID3D12RootSignature* RootSignatureManager::NGet(Resource::ID a_id)
{
	return m_rootStorage.Ref(a_id)->Get();
}
