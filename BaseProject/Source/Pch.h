#pragma once

//=============================================================================
// C/C++ Standard Library (STL)
//=============================================================================
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cinttypes>
#include <cmath>
#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <span>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//=============================================================================
// Windows API
//=============================================================================
#define NOMINMAX
#include <Windows.h>         // WinAPIの基本ヘッダー
#include <psapi.h>           // プロセスステータスAPI
#include <stdio.h>
#include <wrl/client.h>      // Microsoft::WRL::ComPtr（スマートポインタ）

template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma comment(lib, "Rpcrt4.lib") // UUID生成等
#pragma comment(lib, "winmm.lib")  // マルチメディアタイマー（FPS制御等）

//=============================================================================
// 外部ライブラリ (警告レベルを無効化してインクルード)
//=============================================================================
#pragma warning(push, 0)

//---------------------------------------------------------
// DirectX 12 Base
//---------------------------------------------------------
#include <dxgi1_6.h>         // スワップチェーンなどDXGI関連（DirectXの基盤）
#include <dxgidebug.h>       // DXGIデバッグ機能
#include <d3d12.h>           // D3D12のメインヘッダー
#include "d3dx12.h"          // D3DX12ユーティリティ（構造体のラッパー）

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"DirectXTK12.lib")
//---------------------------------------------------------
// DirectX Shader Compiler
//---------------------------------------------------------
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

//---------------------------------------------------------
// DirectX Math & Collision
//---------------------------------------------------------
#include <DirectXMath.h>       // 数学ライブラリ（ベクトル・行列）
#include <DirectXColors.h>     // 色定義（Colors::Whiteなど）
#include <DirectXCollision.h>  // 当たり判定処理
#include <SimpleMath.h>        // 直感的な数学ライブラリ（DirectXTK）

namespace DXSM = DirectX::SimpleMath;

//---------------------------------------------------------
// DirectX Extensions (Mesh, Texture, Audio, Input)
//---------------------------------------------------------
#include <DirectXMesh.h>       // メッシュ処理
#include <DirectXTex.h>        // テクスチャ読み込み
#include <Audio.h>             // サウンド処理
#include <comdef.h>            // COMエラーハンドリング
#include <Xinput.h>            // コントローラー対応

#pragma comment(lib, "DirectXMesh.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "Xinput.lib")

//---------------------------------------------------------
// ImGui
//---------------------------------------------------------
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>
#include <imnodes.h>           // ノードエディタ
#include <imGuizmo.h>          // 3DギズモUI

//---------------------------------------------------------
// Other Third-Party Libraries
//---------------------------------------------------------
#include <magic_enum/magic_enum.hpp> // enumの拡張機能
#include <nlohmannJSON/json.hpp>     // JSONパーサー

#pragma warning(pop) // 外部ライブラリの警告無効化を解除

//=============================================================================
// Project Core / Engine (自作ヘッダー)
//=============================================================================
#include "Core/Core.h"
#include "Engine/EngineCommon.h"