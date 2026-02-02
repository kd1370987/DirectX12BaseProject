#pragma once

//===============================================
//
// 基本
//
//===============================================

#define NOMINMAX
#include <Windows.h>         // WinAPIの基本（必須）
#include <stdio.h>

#include <wrl/client.h>      // Microsoft::WRL::ComPtr（スマートポインタ）
template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

//===============================================
//
// STL
//
//===============================================
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <vector>
#include <stack>
#include <list>
#include <iterator>
#include <queue>
#include <algorithm>
#include <memory>
#include <random>
#include <fstream>
#include <iostream>
#include <sstream>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <fileSystem>
#include <typeindex>
#include <bitset>
#include <cinttypes>
#include <cstdint>

//===============================================
//
// Direct3D12
//
//===============================================

#pragma comment(lib, "d3d12.lib")			// d3d12ライブラリをリンクする
#pragma comment(lib, "dxgi.lib")			// dxgiライブラリをリンクする
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")	// DirectXTexライブラリをリンクする

#include <d3d12.h>           // D3D12のメインヘッダー
#include "d3dx12.h"          // D3DX12ユーティリティ（構造体のラッパー）
#include <dxgi1_6.h>         // スワップチェーンなどDXGI関連（DirectXの基盤）
#include <d3dcompiler.h>
#include <comdef.h>

#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

#include <SimpleMath.h>			// 数学ライブラリ

#include <DirectXMath.h>		// 数学ライブラリ（ベクトル・行列）
#include <DirectXColors.h>		// 色定義（Colors::Whiteなど）
#include <DirectXTex.h>			// テクスチャ読み込み（外部ライブラリ）
#include <DirectXCollision.h>   // 当たり判定

//===============================================
//
// ImGui
//
//===============================================

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui-docking/imgui.h"
#include "imgui-docking/imgui_impl_dx12.h"
#include "imgui-docking/imgui_impl_win32.h"

//===============================================
//
// 自分で作った共通ヘッダー
//
//===============================================

#include "Application/AppInc.h"
#include "Core/Core.h"
#include "Engine/EngineCommon.h"

// バッファリング数
enum
{
	BACKBUFFER_COUNT = 2,		// 今回はダブルバッファリング
	CPU_FRAME_COUNT = 3				// フレームリソース管理用
};
// ディスクリプタハンドル構造体
struct DescriptorHandle
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU{};
};