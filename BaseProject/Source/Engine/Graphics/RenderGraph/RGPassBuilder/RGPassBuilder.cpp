#include "RGPassBuilder.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../RenderGraph.h"
namespace Engine::Graphics
{
	D3D12::GraphicsPipelineDesc& RGRasterPassBuilder::CreatePSODesc(const std::string& a_name, uint8_t& a_outIndex)
	{
		// 構造体を作成して名前をセット
		D3D12::GraphicsPipelineDesc _desc = {};
		_desc.SetName(a_name);
		m_tempPSODescVec.push_back({ _desc,&a_outIndex });

		// PSO作成用構造体のみ参照で返す
		return m_tempPSODescVec.back().desc;
	}
	void RGRasterPassBuilder::ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager)
	{
		// 保持しているPSOの出力を設定
		for (auto& _tempPSO : m_tempPSODescVec)
		{
			// ルートシグネチャ設定
			_tempPSO.desc.SetRootSignature(m_pRootSig);

			// 出力フォーマットセット
			for (auto& _fmt : m_rtvFormatVec)
			{
				_tempPSO.desc.AddRenderTargetFormat(_fmt);
			}

			// PipelineStateManagerにリクエストしてPSOインデックスを取得
			if (a_pPSOManager)
			{
				auto _handle = a_pPSOManager->RequestHandle(_tempPSO.desc);
				*_tempPSO.pOutIndex = static_cast<uint8_t>(_handle.GetIndex());
				m_pNode->psoIndexMap[_tempPSO.desc.name] = *_tempPSO.pOutIndex;
			}
		}
	}
	bool RGRasterPassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath)
	{
		m_pRootSig = a_pPSOManager->Request(a_shaderPath);
		if (!m_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return false;
		}
		return true;
	}
	void RGRasterPassBuilder::Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Read(a_texName, a_type);
		m_pNode->read.push_back(_resID);

		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}
	void RGRasterPassBuilder::Write(const std::string & a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Write(a_texName, a_type);
		m_pNode->write.push_back(_resID);

		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });

		// 出力用フォーマットとして記憶(RTVの時のみ)
		if (a_type == AccessType::RTV)
		{
			auto _format = m_pRG->GetDXGIFormat(_resID);
			m_rtvFormatVec.push_back(_format);
		}
	}

	void RGRasterPassBuilder::SetVS(
		D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_vsPath, const D3D12_INPUT_LAYOUT_DESC& a_desc
	)
	{
		// インプットレイアウトセット
		a_pso.SetInputLayout(a_desc);

		// 頂点シェーダーセット
		auto _vsHandle = Resource::ShaderLoader::Request(a_vsPath);
		auto _byteCode = Resource::ResourceManager::Instance().Get(_vsHandle)->GetByteCode();
		a_pso.SetVS(_byteCode);
	}
	void RGRasterPassBuilder::SetPS(D3D12::GraphicsPipelineDesc & a_pso, const std::string & a_psPath)
	{
		auto _psHandle = Resource::ShaderLoader::Request(a_psPath);
		auto _byteCode = Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode();
		a_pso.SetPS(_byteCode);
	}

	// ---------------------------------------------------------
	// RGComputePassBuilder
	// ---------------------------------------------------------
	void RGComputePassBuilder::ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager)
	{
		m_desc.SetRootSignature(m_pRootSig);
		if (a_pPSOManager && m_pOutIndex)
		{
			auto _handle = a_pPSOManager->RequestHandle(m_desc);
			*m_pOutIndex = static_cast<uint8_t>(_handle.GetIndex());
			m_pNode->psoIndexMap[m_desc.name] = *m_pOutIndex;
		}
	}

	ID3D12RootSignature* RGComputePassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath)
	{
		m_pRootSig = a_pPSOManager->Request(a_shaderPath);
		if (!m_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return nullptr;
		}
		return a_pPSOManager->Request(a_shaderPath);
	}

	void RGComputePassBuilder::Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Read(a_texName, AccessType::SRV);
		m_pNode->read.push_back(_resID);
		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}

	void RGComputePassBuilder::Write(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Write(a_texName, a_type);
		m_pNode->write.push_back(_resID);
		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}

	void RGComputePassBuilder::SetShader(const std::string& a_csPath, const std::string& a_name, uint8_t& a_outIndex)
	{
		m_desc.SetName(a_name);
		auto _csHandle = Resource::ShaderLoader::Request(a_csPath);
		auto* _cs = Resource::ResourceManager::Instance().Ref(_csHandle);
		m_desc.desc.CS.pShaderBytecode = _cs->Get()->GetBufferPointer();
		m_desc.desc.CS.BytecodeLength = _cs->Get()->GetBufferSize();
		m_pOutIndex = &a_outIndex;
	}

	// ---------------------------------------------------------
	// RGGlobalsPassBuilder
	// ---------------------------------------------------------
	void RGGlobalsPassBuilder::Read(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Read(a_texName, a_type);
		m_pNode->read.push_back(_resID);
		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}

	void RGGlobalsPassBuilder::Write(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		auto _resID = m_pRG->Write(a_texName, a_type);
		m_pNode->write.push_back(_resID);
		m_pNode->resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}
}