#pragma once

class RenderingEngine;
class DescriptorHeapManager;

class ImGuiContex
{
public:

	void Init(HWND a_hwnd);

	void CallImGuiDrawData(ID3D12GraphicsCommandList* a_pCmdList);

	void Release();

private:



};