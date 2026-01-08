#pragma once

using RootSigID = uint32_t;

class RootSignature;

class RootSignatureManager
{
public:

	void Init();

	std::shared_ptr<RootSignature> Get(RootSigID a_id);

	ID3D12RootSignature* NGet(RootSigID a_id);

private:

	Storage<RootSigID, RootSignature> m_rootSigStorage;
	RootSigID m_id = 0;
};