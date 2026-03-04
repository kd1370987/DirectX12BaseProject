#pragma once
//==========================================================================================
// 
// 共通仕様
// 
//==========================================================================================
#include "Engine/Common/Color.h"		// 色



//==========================================================================================
// 
// ECS
// 
//==========================================================================================
#include "Engine/ECS/ECSCommon.h"

//==========================================================================================
// 
// DirectX12ラッパー
// 
//==========================================================================================
constexpr UINT INVALID_INDEX = UINT_MAX;
#include "D3D12/D3D12Common.h"
//------------------------------------------------------------------------------------------
// バッファー
//------------------------------------------------------------------------------------------
#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/RenderTarget/RenderTarget.h"

//==========================================================================================
// 
// ストレージ管理
// 
//==========================================================================================
namespace Resource
{
	using DataIndex = uint16_t;
	using DataGeneration = uint16_t;
	using ID = uint32_t;

	namespace Limits
	{
		constexpr ID INVALID_ID = std::numeric_limits<ID>::max();
	}
}

#include "Engine/SlotStorage/SlotStorage.h"

//==========================================================================================
// 
// 描画
// 
//==========================================================================================
#include "Graphics/GraphicCommon.h"

//==========================================================================================
// 
// エディター
// 
//==========================================================================================
#include "Editor/ImGui/ImGuiContext.h"

// バッファリング数
enum
{
	BACKBUFFER_COUNT = 3,		// 今回はダブルバッファリング
	CPU_FRAME_COUNT = 3				// フレームリソース管理用
};
// ディスクリプタハンドル構造体
struct DescriptorHandle
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU{};
};

//==========================================================================================
// 
// 当たり判定
// 
//==========================================================================================
#include "Engine/Collision/Query/CollisionMesh/CollisionMesh.h"		// コリジョンメッシュ


//==========================================================================================
// 
// リソース
// 
//==========================================================================================
#include "Engine/Graphics/GraphicResource/Resource/Vertex/Vertex.h"				// 頂点データ
#include "Engine/Graphics/GraphicResource/Resource/Mesh/Mesh.h"					// メッシュ
#include "Engine/Graphics/GraphicResource/Resource/Animation/Animation.h"		// アニメーションデータ
#include "Engine/Graphics/GraphicResource/Resource/Material/Material.h"			// マテリアル
#include "Engine/Graphics/GraphicResource/Resource/Node/Node.h"					// ノード

#include "Engine/Graphics/GraphicResource/Resource/Model/Model.h"				// モデル
#include "Engine/Graphics/GraphicResource/Resource/QuadPolygon/QuadPolygon.h"	// クアッドポリゴン
#include "Engine/Graphics/GraphicResource/Resource/Texture/Texture.h"			// テクスチャ
//------------------------------------------------------------------------------------------
// リソースの読み込み
//------------------------------------------------------------------------------------------
#include "Engine/Graphics/GraphicResource/Serialize/ModelSerialize/TinyGLTFSerialize/TinyGLTFSerialize.h"
#include "Engine/Graphics/GraphicResource/Serialize/ModelSerialize/AssimpSerialize/AssimpSerialize.h"