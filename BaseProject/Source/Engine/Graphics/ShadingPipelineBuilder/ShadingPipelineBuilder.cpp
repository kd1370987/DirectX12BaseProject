#include "ShadingPipelineBuilder.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{
	//Handle<ID3D12PipelineState> ShadingPipelineBuilder::Request(PSOKey a_key, RenderGraph* a_pRenderGraph, D3D12::PipelineStateManager* a_pPSOManager)
	//{
	//	// 1. キャッシュを検索
	//	auto _it = m_psoMap.find(a_key);
	//	if (_it != m_psoMap.end())
	//	{
	//		return _it->second; // すでに完成していればそれを返す
	//	}

	//	// 2. まだコンパイルを要求すらしていない場合
	//	if (!m_compilingPasses.contains(a_key))
	//	{
	//		m_compilingPasses.insert(a_key);

	//		// --- PSOの注文票（Desc）を組み立てる ---
	//		D3D12::GraphicsPipelineDesc _desc = {};
	//		_desc.desc.RTVFormats = a_pRenderGraph->GetPassRTVFormats(a_key.passIndex).data();
	//		_desc.desc.DSVFormat = a_pRenderGraph->GetPassDSVFormat(a_key.passIndex);

	//		// ShadingModelTable からシェーダーのGUIDやコードを取得
	//		auto* _pShadingModel = Resource::ResourceManager::Instance().Get(a_key.shadingModelTableHandle);
	//		// VSに埋め込んでいるルートシグネチャを取得してくる
	//		auto _spanShaderHandles = _pShadingModel->GetShaderHandles(a_key.passIndex);
	//		for (auto& _shaderHandle : _spanShaderHandles)
	//		{
	//			auto* _pShader = Resource::ResourceManager::Instance().Get(_shaderHandle);
	//			if (!_pShader) continue;

	//			if (_pShader->GetStage() == Resource::EShaderStage::VS)
	//			{
	//				// アニメーションの場合
	//				if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::Skinned) {
	//					// VSのパスを取得
	//					auto _vsGUID = Resource::ResourceManager::Instance().GetCache(_shaderHandle);
	//					auto _vsPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_vsGUID);

	//					// ルートシグネチャをセット
	//					_desc.SetRootSignature(a_pPSOManager->Request(_vsPath));
	//					_desc.SetVS(_pShader->GetByteCode());
	//				}
	//				else if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::UseGPUInstancing)
	//				{
	//					// VSのパスを取得
	//					auto _vsGUID = Resource::ResourceManager::Instance().GetCache(_shaderHandle);
	//					auto _vsPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_vsGUID);

	//					// ルートシグネチャをセット
	//					_desc.SetRootSignature(a_pPSOManager->Request(_vsPath));
	//					_desc.SetVS(_pShader->GetByteCode());
	//				}
	//				else if (a_key.permutationFlags & (uint32_t)EShaderPermutationFlags::Static)
	//				{
	//					// VSのパスを取得
	//					auto _vsGUID = Resource::ResourceManager::Instance().GetCache(_shaderHandle);
	//					auto _vsPath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_vsGUID);

	//					// ルートシグネチャをセット
	//					_desc.SetRootSignature(a_pPSOManager->Request(_vsPath));
	//					_desc.SetVS(_pShader->GetByteCode());
	//				}
	//				
	//			}
	//		}



	//		// 3. 非同期コンパイル部（タスクシステム等に丸投げする）
	//		/*
	//		m_pPSOManager->RequestAsync(_desc, [this, a_key](Handle<ID3D12PipelineState> compiledPSO) {
	//			// コンパイルが完了したらメインスレッド側のマップに書き込む
	//			m_psoMap[a_key] = compiledPSO;
	//			m_compilingPasses.erase(a_key);
	//		});
	//		*/
	//	}

	//	// 4. 非同期コンパイル中のフレームの対処
	//	// 準備ができるまでは、システム初期化時に作っておいた「フォールバック用PSO」を返して、
	//	// 画面がパッと一瞬消えたりクラッシュしたりするのを防ぐ
	//	return m_fallbackPSO;
	//}
}