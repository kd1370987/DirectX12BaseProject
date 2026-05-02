#include "RootSignatureManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
namespace Engine::D3D12
{
	void RootSignatureManager::Init(const UINT& a_slotMax)
	{
		// 管理スロット確保
		m_rootStorage.Init(a_slotMax);
	}

	Engine::Resource::ID RootSignatureManager::Register(
		const std::string& a_key,
		const std::vector<std::pair<RootParameterType, std::vector<RangeType>>>& a_rootParamsVec
	)
	{
		auto _spRootSig = std::make_shared<RootSignature>();
		if (!_spRootSig->Create(a_rootParamsVec))
		{
			assert(0 && "ルートシグネチャの生成に失敗");
			return Engine::Resource::Limits::INVALID_ID;
		}

		return m_rootStorage.Add(a_key, _spRootSig);
	}

	Engine::Resource::ID RootSignatureManager::CreateRootSig(const std::string& a_key, const std::vector<RootSigLayout>& a_rootParamsVec)
	{
		std::vector< std::pair<RootParameterType, std::vector<RangeType>>> _inputLayout;
		for (auto& _layout : a_rootParamsVec)
		{
			_inputLayout.push_back({ _layout.parameType ,_layout.rangeTypeVec });
		}

		// ルートシグネチャ生成
		auto _spRootSig = std::make_shared<RootSignature>();
		if (!_spRootSig->Create(_inputLayout))
		{
			assert(0 && "ルートシグネチャの生成に失敗");
			return Engine::Resource::Limits::INVALID_ID;
		}

		Engine::Resource::ID _id = m_rootStorage.Add(a_key, _spRootSig);

		for (auto& _layout : a_rootParamsVec)
		{
			m_rootLayout[_id].push_back(_layout);
		}

		return _id;
	}

	Engine::Resource::ID RootSignatureManager::CreateRootSig(const std::string& a_key, const std::vector<RootSigLayout>& a_rootParamsVec, const D3D12_ROOT_SIGNATURE_FLAGS& a_flags)
	{
		std::vector< std::pair<RootParameterType, std::vector<RangeType>>> _inputLayout;
		for (auto& _layout : a_rootParamsVec)
		{
			_inputLayout.push_back({ _layout.parameType ,_layout.rangeTypeVec });
		}

		// ルートシグネチャ生成
		auto _spRootSig = std::make_shared<RootSignature>();
		if (!_spRootSig->Create(_inputLayout, a_flags))
		{
			assert(0 && "ルートシグネチャの生成に失敗");
			return Engine::Resource::Limits::INVALID_ID;
		}

		Engine::Resource::ID _id = m_rootStorage.Add(a_key, _spRootSig);

		for (auto& _layout : a_rootParamsVec)
		{
			m_rootLayout[_id].push_back(_layout);
		}

		return _id;
	}

	Engine::Resource::ID RootSignatureManager::CreateRootSig(const std::string& a_key, const RootSigInit& a_rootInit)
	{
		// ルートシグネチャ生成
		auto _spRootSig = std::make_shared<RootSignature>();
		if (!_spRootSig->Create(a_rootInit))
		{
			assert(0 && "ルートシグネチャの生成に失敗");
			return Engine::Resource::Limits::INVALID_ID;
		}

		Engine::Resource::ID _id = m_rootStorage.Add(a_key, _spRootSig);

		return _id;
	}

	ID3D12RootSignature* RootSignatureManager::NGet(Engine::Resource::ID a_id)
	{
		return m_rootStorage.Ref(a_id)->Get();
	}

	ID3D12RootSignature* RootSignatureManager::Ref(const std::string& a_key)
	{
		auto _id = m_rootStorage.GetID(a_key);
		return m_rootStorage.Ref(_id)->Get();
	}

	Engine::Resource::ID RootSignatureManager::GetID(const std::string& a_key)
	{
		return m_rootStorage.GetID(a_key);
	}

	UINT RootSignatureManager::GetRegiNum(Engine::Resource::ID a_id, RootSigSemantic a_sema)
	{
		auto _it = m_rootLayout.find(a_id);
		if (_it != m_rootLayout.end())
		{
			for (UINT _i = 0; _i < _it->second.size(); ++_i)
			{
				if (a_sema == _it->second[_i].semantic)
				{
					return _i;
				}
			}
		}

		return ERR_UINT;
	}
}