#pragma once

namespace Engine::D3D12
{
	class RootSignature;

	constexpr UINT ERR_UINT = UINT_MAX;

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
		Engine::Resource::ID Register(
			const std::string& a_key,
			const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec
		);
		Engine::Resource::ID CreateRootSig(
			const std::string& a_key,
			const std::vector<RootSigLayout>& a_rootParamsVec
		);
		Engine::Resource::ID CreateRootSig(
			const std::string& a_key,
			const std::vector<RootSigLayout>& a_rootParamsVec,
			const D3D12_ROOT_SIGNATURE_FLAGS& a_flags
		);
		Engine::Resource::ID CreateRootSig(
			const std::string& a_key,
			const RootSigInit& a_rootInit
		);

		/// <summary>
		/// ID3D12RootSignatureの生ポインタを直接取得
		/// </summary>
		/// <param name="a_id">管理ID</param>
		ID3D12RootSignature* NGet(Engine::Resource::ID a_id);
		ID3D12RootSignature* Ref(const std::string& a_key);
		Engine::Resource::ID GetID(const std::string& a_key);


		UINT GetRegiNum(Engine::Resource::ID a_id, RootSigSemantic a_sema);

	private:

		Engine::Storage::SlotStorage<RootSignature> m_rootStorage;
		std::unordered_map<Engine::Resource::ID, std::vector<RootSigLayout>> m_rootLayout;
	};
}