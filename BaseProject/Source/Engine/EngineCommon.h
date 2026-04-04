#pragma once
//==========================================================================================
// 
// 共通仕様
// 
//==========================================================================================
#include "Engine/Common/Color.h"		// 色

// バッファリング数
enum
{
	BACKBUFFER_COUNT = 3,		// 今回はダブルバッファリング
	CPU_FRAME_COUNT = 3				// フレームリソース管理用
}; 

//==========================================================================================
// 
// ストレージ管理
// 
//==========================================================================================
namespace Engine::Resource
{
	using Index = uint16_t;
	using Generation = uint16_t;
	using ID = uint32_t;

	namespace Limits
	{
		constexpr ID			INVALID_ID = std::numeric_limits<ID>::max();
		constexpr Index			INVALID_INDEX = std::numeric_limits<Index>::max();
		constexpr Generation	INVALID_GENERATION = std::numeric_limits<Generation>::max();
	}
}

#include "Engine/SlotStorage/SlotStorage.h"

#include "Engine/Resource/Common/Common.h"

#include "Engine/Storage/HandleStorage/HandleStorage.h"			// ハンドルストレージ

//==========================================================================================
// 
// DirectX12ラッパー
// 
//==========================================================================================
constexpr UINT INVALID_INDEX = UINT_MAX;
#include "D3D12/D3D12Common.h"

//------------------------------------------------------------------------------------------
// オブジェクト
//------------------------------------------------------------------------------------------
#include "D3D12/D3DObject/PipeLineState/PipelineState.h"

//------------------------------------------------------------------------------------------
// バッファー
//------------------------------------------------------------------------------------------
#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/RenderTarget/RenderTarget.h"
#include "Engine/D3D12/D3DObject/Buffer/StructuredBuffer/StructuredBuffer.h"

//==========================================================================================
// 
// 当たり判定
// 
//==========================================================================================
#include "Engine/Collision/Query/CollisionMesh/CollisionMesh.h"		// コリジョンメッシュ

//==========================================================================================
// 
// レイトレ用構造体
// 
//==========================================================================================
#include "Engine/Raytracing/BLAS/BLAS.h"
//==========================================================================================
// 
// リソース
// 
//==========================================================================================
#include "Engine/Resource/Data/Texture/Texture.h"			// テクスチャ
#include "Engine/Resource/Data/Vertex/Vertex.h"				// 頂点データ
#include "Engine/Resource/Data/Mesh/Mesh.h"					// メッシュ
#include "Engine/Resource/Data/Animation/Animation.h"		// アニメーションデータ
#include "Engine/Resource/Data/Material/Material.h"			// マテリアル
#include "Engine/Resource/Data/Node/Node.h"					// ノード

#include "Engine/Resource/Data/Model/Model.h"				// モデル
#include "Engine/Resource/Data/QuadPolygon/QuadPolygon.h"	// クアッドポリゴン

#include "Resource/Data/Shader/Shader.h"
#include "Engine/Resource/Data/ShaderLibrary/ShaderLibrary.h"	// シェーダーライブラリ
//==========================================================================================
// 
// レイトレ用構造体
// 
//==========================================================================================
#include "Engine/Raytracing/Common/RaytracingInstance.h"
#include "Engine/Raytracing/Common/Common.h"


//==========================================================================================
// 
// 描画
// 
//==========================================================================================
#include "Graphics/GraphicCommon.h"

//==========================================================================================
// 
// ECS
// 
//==========================================================================================
#include "Engine/ECS/ECSCommon.h"


//==========================================================================================
// 
// エディター
// 
//==========================================================================================
#include "Editor/ImGui/ImGuiContext.h"

//==========================================================================================
// 
// アニメーション
// 
//==========================================================================================
#include "Animation/AnimationEvaluator/AnimationEvalutor.h"







