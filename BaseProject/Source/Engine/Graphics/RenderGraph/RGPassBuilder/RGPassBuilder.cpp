#include "RGPassBuilder.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics
{
	// =========================================================
	// RGPassBuilderBase : リソース宣言
	// =========================================================
	RGResourceRef RGPassBuilderBase::PushAccess(const RGAccessDecl& a_decl)
	{
		m_pNode->accesses.push_back(a_decl);
		return { static_cast<uint16_t>(m_pNode->accesses.size() - 1) };
	}

	RGResourceRef RGPassBuilderBase::ReadSRV(const std::string& a_texName)
	{
		return PushAccess({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::Store, false, false });
	}

	RGResourceRef RGPassBuilderBase::ReadHistorySRV(const std::string& a_texName)
	{
		// テンポラルフラグを立てておく
		return PushAccess({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, true, false });
	}

	RGResourceRef RGPassBuilderBase::ReadDepth(const std::string& a_texName)
	{
		return PushAccess({ a_texName, AccessType::Depth_Read, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, false, false });
	}

	RGResourceRef RGPassBuilderBase::CopySrc(const std::string& a_texName)
	{
		return PushAccess({ a_texName, AccessType::CopySrc, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, false, false });
	}

	RGResourceRef RGPassBuilderBase::WriteRTV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp, false, true });
	}

	RGResourceRef RGPassBuilderBase::WriteDepth(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::Depth_Write, a_format, a_texScale, a_loadOp, a_storeOp, false, true });
	}

	RGResourceRef RGPassBuilderBase::WriteUAV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::UAV, a_format, a_texScale, a_loadOp, a_storeOp, false, true });
	}

	RGResourceRef RGPassBuilderBase::WriteTemporalRTV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::RTV, a_format, a_texScale, a_loadOp, a_storeOp, true, true });
	}

	RGResourceRef RGPassBuilderBase::WriteTemporalUAV(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::UAV, a_format, a_texScale, a_loadOp, a_storeOp, true, true });
	}

	RGResourceRef RGPassBuilderBase::CopyDst(const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		return PushAccess({ a_texName, AccessType::CopyDst, a_format, a_texScale, a_loadOp, a_storeOp, false, true });
	}

	// =========================================================
	// RGPassBuilderBase : 宣言的バインド
	// =========================================================
	RGSrvTableScope RGPassBuilderBase::SrvTable(UINT a_rootIndex)
	{
		RGBindDecl _bind = {};
		_bind.type			= ERGBindType::SrvTable;
		_bind.rootIndex		= a_rootIndex;
		_bind.firstAccess	= static_cast<uint16_t>(m_pNode->accesses.size());
		_bind.count			= 0;

		m_pNode->binds.push_back(_bind);
		return RGSrvTableScope(this, m_pNode->binds.size() - 1);
	}

	RGResourceRef RGPassBuilderBase::BindSRV(UINT a_rootIndex, const std::string& a_texName)
	{
		RGResourceRef _ref = ReadSRV(a_texName);

		RGBindDecl _bind = {};
		_bind.type			= ERGBindType::Srv;
		_bind.rootIndex		= a_rootIndex;
		_bind.firstAccess	= _ref.index;
		_bind.count			= 1;
		m_pNode->binds.push_back(_bind);

		return _ref;
	}

	RGResourceRef RGPassBuilderBase::BindUAV(UINT a_rootIndex, const std::string& a_texName, DXGI_FORMAT a_format, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		RGResourceRef _ref = WriteUAV(a_texName, a_format, a_loadOp, a_storeOp, a_texScale);

		RGBindDecl _bind = {};
		_bind.type			= ERGBindType::Uav;
		_bind.rootIndex		= a_rootIndex;
		_bind.firstAccess	= _ref.index;
		_bind.count			= 1;
		m_pNode->binds.push_back(_bind);

		return _ref;
	}

	void RGPassBuilderBase::Copy(const std::string& a_srcName, const std::string& a_dstName, DXGI_FORMAT a_dstFormat, LoadOp a_loadOp, StoreOp a_storeOp, float a_texScale)
	{
		RGResourceRef _src = CopySrc(a_srcName);
		RGResourceRef _dst = CopyDst(a_dstName, a_dstFormat, a_loadOp, a_storeOp, a_texScale);

		m_pNode->copies.push_back({ _src.index, _dst.index });
	}

	// =========================================================
	// RGSrvTableScope
	// =========================================================
	RGResourceRef RGSrvTableScope::Push(const RGAccessDecl& a_decl)
	{
		auto& _bind = m_pBuilder->m_pNode->binds[m_bindIndex];

		// テーブルはディスクリプタが連続していることが前提。
		// 途中で別の宣言を挟むと崩れるのでビルド時に弾く
		ENGINE_ERRLOG(
			m_pBuilder->m_pNode->accesses.size() == static_cast<size_t>(_bind.firstAccess) + _bind.count,
			"SrvTableへの追加の途中で別のリソース宣言が挟まっています : %s",
			m_pBuilder->m_pNode->name.c_str()
		);

		m_lastRef = m_pBuilder->PushAccess(a_decl);
		++_bind.count;

		return m_lastRef;
	}

	RGSrvTableScope& RGSrvTableScope::Add(const std::string& a_texName)
	{
		Push({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::Store, false, false });
		return *this;
	}

	RGSrvTableScope& RGSrvTableScope::AddHistory(const std::string& a_texName)
	{
		Push({ a_texName, AccessType::SRV, DXGI_FORMAT_UNKNOWN, 1.0f, LoadOp::Load, StoreOp::DontCare, true, false });
		return *this;
	}

	// =========================================================
	// RGRasterPassBuilder
	// =========================================================
	D3D12::GraphicsPipelineDesc& RGRasterPassBuilder::CreatePSODesc(const std::string& a_name, uint8_t& a_outIndex)
	{
		// 構造体を作成して名前をセット
		D3D12::GraphicsPipelineDesc _desc = {};
		_desc.SetName(a_name);
		m_tempPSODescVec.push_back({ _desc, &a_outIndex });

		// PSO作成用構造体のみ参照で返す
		return m_tempPSODescVec.back().desc;
	}

	void RGRasterPassBuilder::ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager)
	{
		// パスの出力フォーマットは宣言済みのRTVアクセスから引く
		std::vector<DXGI_FORMAT> _rtvFormatVec = {};
		for (const auto& _access : m_pNode->accesses)
		{
			if (_access.type == AccessType::RTV) _rtvFormatVec.push_back(_access.format);
		}

		for (auto& _tempPSO : m_tempPSODescVec)
		{
			// ルートシグネチャ設定
			_tempPSO.desc.SetRootSignature(m_pNode->pRootSig);

			// 出力フォーマットセット
			for (auto& _fmt : _rtvFormatVec)
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

	ID3D12RootSignature* RGRasterPassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob)
	{
		auto* _pRootSig = a_pPSOManager->Request(a_pBlob);
		if (!_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return nullptr;
		}
		StoreRootSignature(_pRootSig);
		return _pRootSig;
	}

	ID3DBlob* RGRasterPassBuilder::SetVS(
		D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_vsPath, const D3D12_INPUT_LAYOUT_DESC& a_desc
	)
	{
		// インプットレイアウトセット
		a_pso.SetInputLayout(a_desc);

		// 頂点シェーダーセット
		auto _vsHandle = Resource::ShaderLoader::Request(a_vsPath);
		auto* _pShader = Resource::ResourceManager::Instance().Ref(_vsHandle);
		a_pso.SetVS(_pShader->GetByteCode());

		return _pShader->Get();
	}

	void RGRasterPassBuilder::SetPS(D3D12::GraphicsPipelineDesc& a_pso, const std::string& a_psPath)
	{
		auto _psHandle = Resource::ShaderLoader::Request(a_psPath);
		auto _byteCode = Resource::ResourceManager::Instance().Get(_psHandle)->GetByteCode();
		a_pso.SetPS(_byteCode);
	}

	// =========================================================
	// RGMeshShaderPassBuilder
	// =========================================================
	ID3D12RootSignature* RGMeshShaderPassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob)
	{
		auto* _pRootSig = a_pPSOManager->Request(a_pBlob);
		if (!_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return nullptr;
		}
		StoreRootSignature(_pRootSig);
		return _pRootSig;
	}

	// =========================================================
	// RGComputePassBuilder
	// =========================================================
	void RGComputePassBuilder::ResolveAndCompile(D3D12::PipelineStateManager* a_pPSOManager)
	{
		m_desc.SetRootSignature(m_pNode->pRootSig);
		if (a_pPSOManager && m_pOutIndex)
		{
			auto _handle = a_pPSOManager->RequestHandle(m_desc);
			*m_pOutIndex = static_cast<uint8_t>(_handle.GetIndex());
			m_pNode->psoIndexMap[m_desc.name] = *m_pOutIndex;

			// グラフが実行前に自動でセットするPSOとして記録
			m_pNode->psoIndex = *m_pOutIndex;
		}
	}

	ID3D12RootSignature* RGComputePassBuilder::SetRootSignature(D3D12::PipelineStateManager* a_pPSOManager, ID3DBlob* a_pBlob)
	{
		auto* _pRootSig = a_pPSOManager->Request(a_pBlob);
		if (!_pRootSig)
		{
			Engine::Editor::MainEditor::Instance().ErrorLog("ルートシグネチャが生成されませんでした");
			return nullptr;
		}
		StoreRootSignature(_pRootSig);
		return _pRootSig;
	}

	ID3DBlob* RGComputePassBuilder::SetShader(const std::string& a_csPath, const std::string& a_name, uint8_t& a_outIndex)
	{
		m_desc.SetName(a_name);
		auto _csHandle = Resource::ShaderLoader::Request(a_csPath);
		auto* _cs = Resource::ResourceManager::Instance().Ref(_csHandle);
		m_desc.desc.CS.pShaderBytecode = _cs->Get()->GetBufferPointer();
		m_desc.desc.CS.BytecodeLength = _cs->Get()->GetBufferSize();
		m_pOutIndex = &a_outIndex;

		return _cs->Get();
	}
}
