#include "RasterizePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{
	void RasterizePass::Init(const PassInitDesc& a_initDesc)
	{
		BaseRenderPass::Init(a_initDesc);

		for(auto&[_type,_desc] : m_psoMap)
		{
			m_pPsoVec.push_back({m_pPipelineStateManager->Request(_desc.psoDesc),_desc.type});
		}
	}
	void RasterizePass::Begine(RenderContext* a_pCtx)
	{
		Editor::MainEditor::Instance().StartWatch(m_name.c_str());
		a_pCtx->BindHeap();
		a_pCtx->SetGraphicsRootSignature(m_pRootSig);
	}
	void RasterizePass::End(RenderContext * a_pCtx)
	{
		Editor::MainEditor::Instance().EndWatch(m_name.c_str());
	}
	void RasterizePass::DrawQueue(RenderContext * a_pCtx)
	{
		for (auto& [_pPso,_type] : m_pPsoVec)
		{
			// PSOのセット
			a_pCtx->SetGraphicPSO(_pPso);

			// 指定タイプの命令キューを取得
			auto _draws = a_pCtx->GetItemVec(_type);
			if (_draws.size() <= 0) continue;

			for (auto& _item : _draws)
			{
				// オブジェクト情報セット
				DXSM::Vector2 _uv = {0,0};
				DXSM::Vector2 _tile = {1,1};
				a_pCtx->BindObuje(1,_uv, _tile);
				

				// マテリアルのバインド
				if (!_item.pMaterial) continue;
				a_pCtx->BindMaterial(3,_item.pMaterial, _item.colorScale, _item.emissiveScale);
				a_pCtx->BindMaterialSRV(5,_item.pMaterial);

				// メッシュのバインド
				if (!_item.pMesh)continue;
				a_pCtx->BindMesh(2,_item.pMesh, _item.worldMat);

				// アニメーションタイプならボーンをバインド
				if (_type == RenderQueueType::AnimationOpaque || _type == RenderQueueType::AnimationTransparent)
				{
					a_pCtx->BindCBBone(_item.boneRange);
				}

				// 描画
				a_pCtx->Draw(_item.pMesh,_item.subIdx);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------------------
	// ヘルパー群
	//------------------------------------------------------------------------------------------------------------
	D3D12::GraphicsPipelineDesc& RasterizePass::AddPSODesc(const ERenderType& a_type,const RenderQueueType& a_queueType)
	{
		m_psoMap[a_type] = {};
		m_psoMap[a_type].type = a_queueType;
		return m_psoMap[a_type].psoDesc;
	}
	void RasterizePass::SetInputLayout(const ERenderType& a_type, const D3D12_INPUT_LAYOUT_DESC& a_desc)
	{
		auto _it = m_psoMap.find(a_type);
		if (_it != m_psoMap.end())
		{
			_it->second.psoDesc.SetInputLayout(a_desc);
			return;
		}
		assert(0 && "追加されていないレイアウトタイプです");
	}
	void RasterizePass::SetVS(const ERenderType & a_type, const std::string & a_filePath)
	{
		auto _it = m_psoMap.find(a_type);
		if (_it != m_psoMap.end())
		{
			Resource::Handle<Resource::Shader> _vsHandle = Resource::ShaderLoader::Request(a_filePath);
			_it->second.psoDesc.SetVS(Resource::ResourceManager::Instance().Get(_vsHandle)->GetByteCode());

			m_pRootSig = m_pPipelineStateManager->Request(a_filePath);
			if (!m_pRootSig)
			{
				assert(0 && "ルートシグネチャが設定されていません");
				return;
			}
			_it->second.psoDesc.SetRootSignature(m_pRootSig);

			return;
		}
		assert(0 && "追加されていないレイアウトタイプです");
	}
	void RasterizePass::SetPS(const ERenderType & a_type, const std::string & a_filePath)
	{
		// 指定タイプと同じタイプのPSOがあるか検索
		auto _it = m_psoMap.find(a_type);
		if (_it != m_psoMap.end())
		{
			// あればPSをリクエストしてセット
			Resource::Handle<Resource::Shader> _psHandle = Resource::ShaderLoader::Request(a_filePath);
			_it->second.psoDesc.SetPS(Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode());
			return;
		}
		assert(0 && "追加されていないレイアウトタイプです");
	}
	void RasterizePass::SetPS(const std::string & a_filePath)
	{
		// 指定したPSを全PSOに適応
		Resource::Handle<Resource::Shader> _psHandle = Resource::ShaderLoader::Request(a_filePath);
		for (auto& [_type, _psoDesc] : m_psoMap)
		{
			_psoDesc.psoDesc.SetPS(Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode());
		}
	}
	Engine::Resource::ID RasterizePass::AddWrite(const std::string & a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		// 依存関係の登録
		auto _id = BaseRenderPass::AddWrite(a_texName, a_type, a_loadOp, a_storeOp);
		auto _format = m_pRG->GetDXGIFormat(_id);

		// PSOの出力レンダーターゲットとして指定
		for (auto& [_type, _psoDesc] : m_psoMap)
		{
			_psoDesc.psoDesc.AddRenderTargetFormat(_format);
		}
		return _id;
	}
}