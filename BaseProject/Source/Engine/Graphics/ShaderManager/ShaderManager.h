#pragma once

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

struct ShaderItem
{
	std::string path;
	ShaderStage stage;
	D3D12_INPUT_LAYOUT_DESC* pInputDesc = nullptr;
};

class ShaderManager
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_slotSize">スロットのサイズ</param>
	void Init(const UINT& a_slotSize);
	
	/// <summary>
	/// シェーダーの登録
	/// </summary>
	/// <param name="a_dst">登録するシェーダーの基本情報</param>
	/// <returns>管理ID</returns>
	Resource::ID Register(const ShaderItem& a_dst);

	/// <summary>
	/// シェーダの生ポインタ取得
	/// </summary>
	/// <param name="a_id">管理ID</param>
	const Shader* NGet(const Resource::ID& a_id);


private:

	SlotStorage<Shader> m_shaderSlot;
};