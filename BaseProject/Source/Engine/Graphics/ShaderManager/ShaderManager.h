#pragma once

using ShaderID = uint32_t;

enum class ShaderStage
{
	Vertex,
	Pixel,
	Compute,
	RayGen,
	Miss,
	ClosestHit
};

struct Shader
{
	// シェーダーの種類
	ShaderStage stage;

	// シェーダーのバイトデータ
	ComPtr<ID3DBlob> blob;
	D3D12_SHADER_BYTECODE byteCode{};

	// 頂点シェーダーのみ
	std::vector<D3D12_INPUT_ELEMENT_DESC> vsInputElemnetVec;
	D3D12_INPUT_LAYOUT_DESC vsInputLayout{};
};

class ShaderManager
{
public:

	/// <summary>
	/// シェーダーの登録
	/// </summary>
	/// <param name="a_path">ファイルパス</param>
	/// <param name="a_stage">シェーダーのステージ</param>
	/// <returns>管理番号</returns>
	ShaderID Register(const std::string& a_path, const ShaderStage& a_stage);

	/// <summary>
	/// シェーダーの取得
	/// </summary>
	/// <param name="a_id">ID</param>
	/// <returns>シェーダーのシェアードポインタ</returns>
	std::shared_ptr<Shader> Get(const ShaderID& a_id);


private:

	ShaderID m_id = 0;
	Storage<ShaderID, Shader> m_shaderStorage;
};