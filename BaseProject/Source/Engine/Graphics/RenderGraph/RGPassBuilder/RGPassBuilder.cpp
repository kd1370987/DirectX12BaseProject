#include "RGPassBuilder.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics
{
	void RGRasterPassBuilder::ReadSRV(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::Store ,false}
		);
	}
	void RGRasterPassBuilder::ReadDepth(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::Depth_Read, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare,false }
		);
	}
	void RGRasterPassBuilder::ReadHistorySRV(const std::string& a_texName)
	{
		// テンポラルフラグを立てておく
		m_pNode->readRequests.push_back({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, true });
	}
	void RGRasterPassBuilder::WriteRTV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp }
		);
		m_rtvFormatVec.push_back(a_format);
	}
	void RGRasterPassBuilder::WriteDepth(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::Depth_Write, a_format, a_texScale, a_loadOp, a_storeOp }
		);
	}
	void RGRasterPassBuilder::WriteTemporalRTV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back({ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp, true });
		m_rtvFormatVec.push_back(a_format);
	}
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

	void RGComputePassBuilder::ReadSRV(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare }
		);
	}

	void RGComputePassBuilder::ReadHistorySRV(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare,true }
		);
	}

	void RGComputePassBuilder::WriteUAV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::UAV, a_format, a_texScale, a_loadOp, a_storeOp }
		);
	}

	void RGComputePassBuilder::WriteTemporalUAV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back({ a_texName, AccessType::UAV, a_format, a_texScale, a_loadOp, a_storeOp, true });
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

	void RGGlobalsPassBuilder::CopySrc(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::CopySrc, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare }
		);
	}

	void RGGlobalsPassBuilder::CopyDst(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::CopyDst, a_format, a_texScale,a_loadOp, a_storeOp }
		);
	}
	void RGGlobalsPassBuilder::ReadSRV(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare }
		);
	}

	void RGGlobalsPassBuilder::WriteUAV(const std::string & a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::UAV, a_format,a_texScale,a_loadOp, a_storeOp }
		);
	}
	void RGMeshShaderPassBuilder::ReadSRV(const std::string& a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::Store ,false }
		);
	}
	void RGMeshShaderPassBuilder::ReadDepth(const std::string & a_texName)
	{
		m_pNode->readRequests.push_back(
			{ a_texName, AccessType::Depth_Read, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare,false }
		);
	}
	void RGMeshShaderPassBuilder::ReadHistorySRV(const std::string & a_texName)
	{
		// テンポラルフラグを立てておく
		m_pNode->readRequests.push_back({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, true });
	}
	void RGMeshShaderPassBuilder::WriteRTV(const std::string & a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp }
		);
		m_rtvFormatVec.push_back(a_format);
	}
	void RGMeshShaderPassBuilder::WriteDepth(const std::string & a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back(
			{ a_texName, AccessType::Depth_Write, a_format, a_texScale, a_loadOp, a_storeOp }
		);
	}
	void RGMeshShaderPassBuilder::WriteTemporalRTV(const std::string & a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		m_pNode->writeRequests.push_back({ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp, true });
		m_rtvFormatVec.push_back(a_format);
	}
	D3D12::MeshPipelineBuilder& RGMeshShaderPassBuilder::CreatePSODesc(const std::string & a_name, uint8_t & a_outIndex)
	{
		// 構造体を作成して名前をセット
		D3D12::MeshPipelineBuilder _desc = {};
		_desc.SetName(a_name);
		m_tempMSPSODescVec.push_back({ _desc,&a_outIndex });

		// PSO作成用構造体のみ参照で返す
		return m_tempMSPSODescVec.back().desc;
	}
	ID3D12RootSignature* RGMeshShaderPassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, const std::string& a_shaderPath)
	{
		m_pRootSig = a_pPSOManager->Request(a_shaderPath);
		if (!m_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return nullptr;
		}
		return m_pRootSig;
	}
	void RGMeshShaderPassBuilder::SetRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_pRootSig = a_pRootSig;
	}
	void RGMeshShaderPassBuilder::SetMS(D3D12::MeshPipelineBuilder& a_pso, const std::string& a_msPath)
	{
		// 頂点シェーダーセット
		auto _msHandle = Resource::ShaderLoader::Request(a_msPath);
		auto* _pBlob = Resource::ResourceManager::Instance().Ref(_msHandle)->Get();
		a_pso.SetMS(_pBlob);
	}
	void RGMeshShaderPassBuilder::SetPS(D3D12::MeshPipelineBuilder & a_pso, const std::string & a_psPath)
	{
		auto _psHandle = Resource::ShaderLoader::Request(a_psPath);
		auto* _pBlob = Resource::ResourceManager::Instance().Ref(_psHandle)->Get();
		a_pso.SetPS(_pBlob);
	}
	void RGMeshShaderPassBuilder::ResolveAndCompile(D3D12::PipelineStateManager * a_pPSOManager)
	{
		// 保持しているPSOの出力を設定
		for (auto& _tempPSO : m_tempMSPSODescVec)
		{
			// ルートシグネチャ設定
			_tempPSO.desc.SetRootSignature(m_pRootSig);

			// 出力フォーマットセット
			for (auto& _fmt : m_rtvFormatVec)
			{
				_tempPSO.desc.AddRenderTarget(_fmt);
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
}