#pragma once

class RootSignature;

class RootSignatureManager
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_slotMax">管理スロット数</param>
	void Init(const UINT& a_slotMax = 10);

	/// <summary>
	/// ルートシグネチャを作成
	/// </summary>
	/// <param name="a_key">ルートシグネチャ名</param>
	/// <param name="a_rootParamsVec">ルートシグネチャの構成</param>
	/// <returns>管理ID</returns>
	Resource::ID Register(
		const std::string& a_key,
		const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec
	);

	/// <summary>
	/// ID3D12RootSignatureの生ポインタを直接取得
	/// </summary>
	/// <param name="a_id">管理ID</param>
	ID3D12RootSignature* NGet(Resource::ID a_id);

private:

	SlotStorage<RootSignature> m_rootStorage;
};