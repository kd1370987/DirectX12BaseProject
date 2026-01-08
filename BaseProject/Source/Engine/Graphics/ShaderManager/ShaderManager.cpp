#include "ShaderManager.h"

ShaderID ShaderManager::Register(const std::string& a_path, const ShaderStage& a_stage)
{
	// ワイルド文字列に変換
	std::wstring _wstr = StringUtility::ToWideString(a_path);

	// シェーダー構造体準備
	//Shader _shader = {};
	std::shared_ptr<Shader> _shader = std::make_shared<Shader>();

	// シェーダーの読み込み
	auto _hr = D3DReadFileToBlob(_wstr.c_str(), _shader->blob.GetAddressOf());
	if (FAILED(_hr))
	{
		assert(0 && "シェーダーの読み込みに失敗");
		return 0;
	}
	// バイトデータからコードを生成
	_shader->byteCode = CD3DX12_SHADER_BYTECODE(_shader->blob.Get());


	// 頂点シェーダーのみインプット関係を使用
	if (a_stage == ShaderStage::Vertex)
	{
		const int _inputElementCount = 5;										// 入力情報数を指定
		_shader->vsInputElemnetVec.push_back(
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		_shader->vsInputElemnetVec.push_back(
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		_shader->vsInputElemnetVec.push_back(
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		_shader->vsInputElemnetVec.push_back(
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		_shader->vsInputElemnetVec.push_back(
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		_shader->vsInputLayout.pInputElementDescs = _shader->vsInputElemnetVec.data();
		_shader->vsInputLayout.NumElements = _inputElementCount;
			
	}
	else
	{
		_shader->vsInputElemnetVec = {};
		_shader->vsInputLayout = {};
	}

	// シェーダーを登録
	m_shaderStorage.Add(m_id,_shader);
	return m_id++;
}

std::shared_ptr<Shader> ShaderManager::Get(const ShaderID& a_id)
{
	return m_shaderStorage.Get(a_id);
}
