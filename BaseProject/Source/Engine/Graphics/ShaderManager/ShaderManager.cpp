#include "ShaderManager.h"

void ShaderManager::Init(const UINT& a_slotSize)
{
	m_shaderSlot.Init(a_slotSize);
}

Resource::ID ShaderManager::Register(const ShaderItem& a_dst)
{
	// ワイルド文字列に変換
	std::wstring _wstr = StringUtility::ToWideString(a_dst.path);

	// シェーダー構造体準備
	std::shared_ptr<Shader> _spShader = std::make_shared<Shader>();

	// シェーダーの読み込み
	auto _hr = D3DReadFileToBlob(
		_wstr.c_str(), 
		_spShader->blob.GetAddressOf()
	);
	if (FAILED(_hr))
	{
		assert(0 && "シェーダーの読み込みに失敗");
		return 0;
	}
	// バイトデータからコードを生成
	_spShader->byteCode = CD3DX12_SHADER_BYTECODE(_spShader->blob.Get());


	// 頂点シェーダーのみインプット関係を使用
	if (!a_dst.pInputDesc)
	{
		_spShader->vsInputLayout.pInputElementDescs = nullptr;
		_spShader->vsInputLayout.NumElements = 0;
	}
	else
	{
		_spShader->vsInputLayout.pInputElementDescs = a_dst.pInputDesc->pInputElementDescs;
		_spShader->vsInputLayout.NumElements = a_dst.pInputDesc->NumElements;
	}
	
	// シェーダーを登録
	return m_shaderSlot.Add(a_dst.path, _spShader);
}

const Shader* ShaderManager::NGet(const Resource::ID& a_id)
{
	return m_shaderSlot.Get(a_id);
}
