#pragma once

//===============================================
//
// ウィンドウズ
//
//===============================================

#define NOMINMAX
#include <Windows.h>         // WinAPIの基本（必須）
#include <psapi.h>
#include <stdio.h>

#pragma comment(lib, "Rpcrt4.lib")

#include <wrl/client.h>      // Microsoft::WRL::ComPtr（スマートポインタ）
template<typename T> 
using ComPtr = Microsoft::WRL::ComPtr<T>;

//===============================================
//
// STL
//
//===============================================
#include <array>
#include <vector>
#include <list>
#include <stack>
#include <queue>

#include <map>
#include <unordered_map>
#include <unordered_set>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <memory>
#include <functional>
#include <algorithm>
#include <iterator>
#include <concepts>

#include <thread>
#include <mutex>
#include <future>
#include <atomic>

#include <random>

#include <filesystem>
#include <typeindex>
#include <bitset>
#include <span>

#include <cstdint>
#include <cinttypes>
#include <cmath>
#include <cstring>

// コントローラー対応
#pragma comment(lib,"Xinput.lib")
#include <Xinput.h>

// FPS関係
#pragma comment(lib,"winmm.lib")

//===============================================
//
// DirectX12
//
//===============================================

#pragma comment(lib, "d3d12.lib")		// d3d12ライブラリをリンクする
#pragma comment(lib, "dxgi.lib")		// dxgiライブラリをリンクする
#pragma comment(lib,"dxguid.lib")

#pragma warning(push, 0)				// 警告OFF

#include <dxgi1_6.h>         // スワップチェーンなどDXGI関連（DirectXの基盤）
#include <dxgidebug.h>

#include <d3d12.h>			 // D3D12のメインヘッダー
#include "d3dx12.h"          // D3DX12ユーティリティ（構造体のラッパー）

// メッシュ
#pragma comment(lib,"DirectXMesh.lib")
#include <DirectXMesh.h>

#include <comdef.h>
//===============================================
// ShaderCompiler
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

#include <d3dcompiler.h>
#include <dxcapi.h>
#include <d3d12shader.h>

//===============================================
// DirectX Math / Utility
#include <DirectXMath.h>				// 数学ライブラリ（ベクトル・行列）
#include <DirectXColors.h>				// 色定義（Colors::Whiteなど）
#include <DirectXCollision.h>			// 当たり判定

#include <SimpleMath.h>					// 数学ライブラリ
namespace DXSM = DirectX::SimpleMath;

//===============================================
// Texture
#pragma comment(lib, "DirectXTex.lib")	// DirectXTexライブラリをリンクする
#include <DirectXTex.h>			// テクスチャ読み込み（外部ライブラリ）

//===============================================
//
// ImGui
//
//===============================================

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>

// ノード
#include <imnodes.h>

// ギズモ
#include <imGuizmo.h>

//===============================================
//
// magic_enum
//
//===============================================
#include <magic_enum/magic_enum.hpp>			// enumの拡張

//===============================================
//
// nlohmannJSON
//
//===============================================
#include <nlohmannJSON/json.hpp>

#pragma warning(pop)

//===============================================
//
// 自分で作った共通ヘッダー
//
//===============================================

#include "Core/Core.h"
#include "Engine/EngineCommon.h"