#include "RasterizePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{
	void RasterizePass::Init(const PassInitDesc& a_initDesc)
	{
		BaseRenderPass::Init(a_initDesc);

		for(auto&[_type,_desc] : m_psoMap)
		{
			m_psoHandle.push_back({ m_pPSOMana->Request(_desc.psoDesc),_desc.type });
			m_pPsoVec.push_back({m_pPipelineStateManager->Request(_desc.psoDesc),_desc.type});
		}
	}
	void RasterizePass::Begine(RenderContext* a_pCtx)
	{
		a_pCtx->BindHeap();
		a_pCtx->SetGraphicsRootSignature(m_rootSigID);
		a_pCtx->BindCameraCB();
	}
	void RasterizePass::End(RenderContext * a_pCtx)
	{
	}
	void RasterizePass::DrawQueue(RenderContext * a_pCtx)
	{
		for (auto& _pso : m_psoHandle)
		{
			// PSOのセット
			a_pCtx->SetGraphicPSO(_pso.first);

			// 指定タイプの命令キューを取得
			auto& _draws = a_pCtx->GetItemVec(_pso.second);
			if (_draws.size() <= 0) continue;

			for (auto& _item : _draws)
			{
				// オブジェクト情報セット
				DXSM::Vector2 _uv = {0,0};
				DXSM::Vector2 _tile = {1,1};
				a_pCtx->BindObuje(_uv, _tile);

				// マテリアルのバインド
				a_pCtx->BindMaterial(_item.pMaterial, _item.colorScale, _item.emissiveScale);

				// メッシュのバインド
				a_pCtx->BindMesh(_item.pMesh, _item.worldMat);

				// アニメーションタイプならボーンをバインド
				if (_pso.second == RenderQueueType::AnimationOpaque || _pso.second == RenderQueueType::AnimationTransparent)
				{
					a_pCtx->BindBone(
						_item.pBoneMatrices,
						_item.boneCount
					);
				}

				// 描画
				a_pCtx->Draw(_item.pMesh,_item.subIdx);
			}
		}
	}

	void RasterizePass::DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type, Resource::Handle<D3D12::PipelineState> a_handle)
	{
		// PSOのセット
		a_pCtx->SetGraphicPSO(a_handle);

		// 指定タイプの命令キューを取得
		auto& _draws = a_pCtx->GetItemVec(a_type);
		if (_draws.size() <= 0) return;

		for (auto& _item : _draws)
		{
			// オブジェクト情報セット
			DXSM::Vector2 _uv = { 0,0 };
			DXSM::Vector2 _tile = { 1,1 };
			a_pCtx->BindObuje(_uv, _tile);

			// マテリアルのバインド
			a_pCtx->BindMaterial(_item.pMaterial, _item.colorScale, _item.emissiveScale);

			// メッシュのバインド
			a_pCtx->BindMesh(_item.pMesh, _item.worldMat);

			// アニメーションタイプならボーンをバインド
			if (a_type == RenderQueueType::AnimationOpaque || a_type == RenderQueueType::AnimationTransparent)
			{
				a_pCtx->BindBone(
					_item.pBoneMatrices,
					_item.boneCount
				);
			}

			// 描画
			a_pCtx->Draw(_item.pMesh, _item.subIdx);
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

			//Resource::Handle<Resource::Shader> _vsHandle = m_pShaderMana->Request(a_filePath);
			Resource::Handle<Resource::Shader> _vsHandle = Resource::ShaderLoader::Request(a_filePath);
			//_it->second.psoDesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
			_it->second.psoDesc.SetVS(Resource::ResourceManager::Instance().Get(_vsHandle)->GetByteCode());

			return;
		}
		assert(0 && "追加されていないレイアウトタイプです");
	}
	void RasterizePass::SetPS(const ERenderType & a_type, const std::string & a_filePath)
	{
		auto _it = m_psoMap.find(a_type);
		if (_it != m_psoMap.end())
		{

			//Resource::Handle<Resource::Shader> _psHandle = m_pShaderMana->Request(a_filePath);
			//_it->second.psoDesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
			Resource::Handle<Resource::Shader> _psHandle = Resource::ShaderLoader::Request(a_filePath);
			_it->second.psoDesc.SetPS(Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode());

			return;
		}
		assert(0 && "追加されていないレイアウトタイプです");
	}
	void RasterizePass::SetPS(const std::string & a_filePath)
	{
		//Resource::Handle<Resource::Shader> _psHandle = m_pShaderMana->Request(a_filePath);
		Resource::Handle<Resource::Shader> _psHandle = Resource::ShaderLoader::Request(a_filePath);
		for (auto& [_type, _psoDesc] : m_psoMap)
		{
			//_psoDesc.psoDesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
			_psoDesc.psoDesc.SetPS(Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode());
		}
	}
	void RasterizePass::SetRootSig(const std::string & a_rootName)
	{
		// ルートシグネチャの取得
		m_rootSigID = m_pRootSigMana->GetID(a_rootName);
		for (auto& [_type, _psoDesc] : m_psoMap)
		{
			_psoDesc.psoDesc.SetRootSignature(m_pRootSigMana->NGet(m_rootSigID));
		}
	}
	void RasterizePass::SetRootSig(ID3D12RootSignature* a_pRootSignature)
	{
		for (auto& [_type, _psoDesc] : m_psoMap)
		{
			_psoDesc.psoDesc.SetRootSignature(a_pRootSignature);
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