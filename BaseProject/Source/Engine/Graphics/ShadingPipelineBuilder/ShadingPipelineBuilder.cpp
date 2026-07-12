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
		if (DXGI_FORMAT_R32_TYPELESS == a_dsvFormat)
		{
			m_dsvFormat = DXGI_FORMAT_D32_FLOAT;
		}
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

			// ★ 新しい統合版ビルダーをインスタンス化
			D3D12::RenderPipelineBuilder _builder;

			// 共通のステート・フォーマット設定
			_builder.DepthEnable(m_depthEnable);
			_builder.DepthWriteMask(m_depthWrite);
			_builder.DepthFunc(m_depthFunc);

			for (auto& _rtvFormat : m_rtvFormats) {
				_builder.AddRenderTargetFormat(_rtvFormat);
			}
			_builder.SetDepthStencilFormat(m_dsvFormat);

			// =========================================================
			// VS / MS / AS の解決
			// =========================================================
			bool _useMeshShader = (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::MeshShader);
			if (_useMeshShader)
			{
				Handle<Resource::Shader> _targetMSHandle;
				Handle<Resource::Shader> _targetASHandle;

				if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::Skinned)
				{
					_targetMSHandle = m_msMap[EShaderPermutationFlags::Skinned];
					_targetASHandle = m_asMap[EShaderPermutationFlags::Skinned];
				}
				else if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::UseGPUInstancing)
				{
					_targetMSHandle = m_msMap[EShaderPermutationFlags::UseGPUInstancing];
					_targetASHandle = m_asMap[EShaderPermutationFlags::UseGPUInstancing];
				}
				else {
					_targetMSHandle = m_msMap[EShaderPermutationFlags::Static];
					_targetASHandle = m_asMap[EShaderPermutationFlags::Static];
				}

				// MSのセットとルートシグネチャの抽出
				if (auto* _pMS = Resource::ResourceManager::Instance().Get(_targetMSHandle))
				{
					auto _msGUID = Resource::ResourceManager::Instance().GetCache(_targetMSHandle);
					auto _msPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_msGUID);

					_builder.SetRootSignature(a_pPSOManager->Request(_msPath));
					_builder.SetMS(_pMS->GetByteCode()); // ★ビルダーにMSをセット
					ENGINE_LOG("PSO RS = %p\n", _builder.GetRootSignature());
				}
				else {
					ENGINE_LOG("Mesh Shaderが見つかりません");
				}

				// ASのセット（存在する場合のみ）
				if (auto* _pAS = Resource::ResourceManager::Instance().Get(_targetASHandle))
				{
					_builder.SetAS(_pAS->GetByteCode()); // ★ビルダーにASをセット
				}
			}
			else
			{
				// 従来の VS パイプライン
				Handle<Resource::Shader> _targetVSHandle;

				// アニメーションか、インスタンシングか、静的か等の優先順位でVSを決定
				if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::Skinned)
				{
					_targetVSHandle = m_vsMap[EShaderPermutationFlags::Skinned];
					_builder.SetInputLayout(D3D12::Input::AnimationInputLayout);
				}
				else if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::UseGPUInstancing)
				{
					_targetVSHandle = m_vsMap[EShaderPermutationFlags::UseGPUInstancing];
				}
				else {
					_targetVSHandle = m_vsMap[EShaderPermutationFlags::Static];
					_builder.SetInputLayout(D3D12::Input::StaticLayout);
				}

				// VSのセットとルートシグネチャの抽出
				if (auto* _pVS = Resource::ResourceManager::Instance().Get(_targetVSHandle))
				{
					auto _vsGUID = Resource::ResourceManager::Instance().GetCache(_targetVSHandle);
					auto _vsPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_vsGUID);

					_builder.SetRootSignature(a_pPSOManager->Request(_vsPath));
					_builder.SetVS(_pVS->GetByteCode()); // ★ビルダーにVSをセット
				}
				else {
					ENGINE_LOG("Vertex Shaderが見つかりません");
				}
			}

			// =========================================================
			// Pixel Shader の解決 (マテリアル・ShadingModelから取得)
			// =========================================================
			auto* _pShadingModel = Resource::ResourceManager::Instance().Get(a_key.shadingModelTableHandle);
			if (_pShadingModel)
			{
				// ZPreかつ不透明(Opaque)なら、PSのセットをスキップ
				bool _isZPrePass = (m_passNameHash == StringUtility::ToHash("ZPre"));
				bool _isOpaque = !(a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::AlphaMasked);

				if (!(_isZPrePass && _isOpaque))
				{
					auto _spanShaderHandles = _pShadingModel->GetShaderHandles(m_passNameHash);
					for (auto& _shaderHandle : _spanShaderHandles)
					{
						auto* _pShader = Resource::ResourceManager::Instance().Get(_shaderHandle);
						if (!_pShader) continue;

						_builder.SetPS(_pShader->GetByteCode());
					}
				}
			}

			// マネージャーにPSOをリクエスト
			auto _psoHandle = a_pPSOManager->RequestHandle(_builder);

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
	void ShadingPipelineBuilder::RegisterMeshShader(EShaderPermutationFlags a_flag, Handle<Resource::Shader> a_msHandle)
	{
		m_msMap[a_flag] = a_msHandle;
	}
	void ShadingPipelineBuilder::RegisterAmplificationShader(EShaderPermutationFlags a_flag, Handle<Resource::Shader> a_asHandle)
	{
		m_asMap[a_flag] = a_asHandle;
	}
	void ShadingPipelineBuilder::SetDepthConfig(bool a_enable, bool a_write, D3D12_COMPARISON_FUNC a_func)
	{
		m_depthEnable = a_enable;
		m_depthWrite = a_write;
		m_depthFunc = a_func;
	}
	bool ShadingPipelineBuilder::HasMeshShader() const
	{
		if (m_msMap.empty())
		{
			return false;
		}
		return true;
	}
}