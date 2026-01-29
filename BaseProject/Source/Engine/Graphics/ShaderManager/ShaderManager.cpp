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
		if (a_dst.stage == ShaderStage::Vertex)
		{
			const int _inputElementCount = 5;										// 入力情報数を指定
			_spShader->vsInputElemnetVec.push_back(
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			_spShader->vsInputElemnetVec.push_back(
				{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			_spShader->vsInputElemnetVec.push_back(
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			_spShader->vsInputElemnetVec.push_back(
				{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			_spShader->vsInputElemnetVec.push_back(
				{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
			_spShader->vsInputLayout.pInputElementDescs = _spShader->vsInputElemnetVec.data();
			_spShader->vsInputLayout.NumElements = _inputElementCount;

		}
		else
		{
			_spShader->vsInputElemnetVec = {};
			_spShader->vsInputLayout = {};
		}
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
