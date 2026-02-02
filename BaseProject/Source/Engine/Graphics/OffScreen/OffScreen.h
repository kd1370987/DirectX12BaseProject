#pragma once

class ShaderManager;
class RootSignatureManager;
class GraphicsPSOManager;

class OffScreen
{
public:

	bool CreateScreenVertex();

	ComPtr<ID3D12Resource> m_screenVB;
	D3D12_VERTEX_BUFFER_VIEW m_screenVBView;
};