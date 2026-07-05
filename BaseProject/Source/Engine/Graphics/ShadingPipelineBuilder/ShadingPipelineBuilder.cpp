#include "ShadingPipelineBuilder.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{

	void ShadingPipelineBuilder::Init(const std::vector<DXGI_FORMAT>& a_rtvFormat, DXGI_FORMAT a_dsvFormat, UINT a_passNameHash)
	{
		m_rtvFormats = a_rtvFormat;
		m_dsvFormat = a_dsvFormat;
		m_passNameHash = a_passNameHash;
	}


	Handle<ID3D12PipelineState> ShadingPipelineBuilder::Request(PSOKey a_key, RenderGraph* a_pRenderGraph, D3D12::PipelineStateManager* a_pPSOManager)
	{
		// キャッシュを検索
		auto _it = m_psoMap.find(a_key);
		if (_it != m_psoMap.end())
		{
			return _it->second; // すでに完成していればそれを返す
		}

		// コンパイル要求
		if (!m_compilingPasses.contains(a_key))
		{
			m_compilingPasses.insert(a_key);
			D3D12::GraphicsPipelineDesc _desc = {};

			for (auto& _rtvFormat : m_rtvFormats) {
				_desc.AddRenderTargetFormat(_rtvFormat);
			}
			_desc.desc.DSVFormat = m_dsvFormat;

			// =========================================================
			// Vertex Shader の解決 (パスが知っている情報を元にフラグから取得)
			// =========================================================
			Handle<Resource::Shader> _targetVSHandle;

			// アニメーションか、インスタンシングか、静的か等の優先順位でVSを決定
			if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::Skinned) {
				_targetVSHandle = m_vsMap[EShaderPermutationFlags::Skinned];
				_desc.SetInputLayout(D3D12::Input::AnimationInputLayout);
			}
			else if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::UseGPUInstancing) {
				_targetVSHandle = m_vsMap[EShaderPermutationFlags::UseGPUInstancing];
			}
			else {
				_targetVSHandle = m_vsMap[EShaderPermutationFlags::Static];
				_desc.SetInputLayout(D3D12::Input::StaticLayout);
			}

			// VSのセットとルートシグネチャの抽出
			if (auto* _pVS = Resource::ResourceManager::Instance().Get(_targetVSHandle))
			{
				auto _vsGUID = Resource::ResourceManager::Instance().GetCache(_targetVSHandle);
				auto _vsPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_vsGUID);

				_desc.SetRootSignature(a_pPSOManager->Request(_vsPath));
				_desc.SetVS(_pVS->GetByteCode());
			}
			else {
				ENGINE_ERRLOG(_pVS, "Vertex Shaderが見つかりません");
			}

			// =========================================================
			// Pixel Shader の解決 (マテリアル・ShadingModelから取得)
			// =========================================================
			auto* _pShadingModel = Resource::ResourceManager::Instance().Get(a_key.shadingModelTableHandle);
			if (_pShadingModel)
			{
				auto _spanShaderHandles = _pShadingModel->GetShaderHandles(m_passNameHash);
				for (auto& _shaderHandle : _spanShaderHandles)
				{
					auto* _pShader = Resource::ResourceManager::Instance().Get(_shaderHandle);
					if (!_pShader) continue;

					// シェーディングモデルにはPSしかセットできないのでそのままセット
					_desc.SetPS(_pShader->GetByteCode());
				}
			}

			// いったんランタイムで止めて作成
			auto _psoHandle = a_pPSOManager->RequestHandle(_desc);
			m_psoMap[a_key] = _psoHandle;
			m_compilingPasses.erase(a_key);
			return _psoHandle;
		}

		return m_fallbackPSO;
	}
	void ShadingPipelineBuilder::RegisterVertexShader(EShaderPermutationFlags a_flag, Handle<Resource::Shader> a_vsHandle)
	{
		m_vsMap[a_flag] = a_vsHandle;
	}
}