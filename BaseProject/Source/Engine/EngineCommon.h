#pragma once
//==========================================================================================
// 
// 共通仕様
// 
//==========================================================================================
#include "Engine/Common/Color.h"				// 色
#include "Engine/Common/Handle.h"				// ハンドル
#include "Engine/Common/EngineConfigTypes.h"	// エンジン基盤設定

#include "Engine/D3D12/D3D12Types.h"			// D3D12の共通設定

// 共通数学
#include "Utility/Math/Alignment.h"				// アライメント
#include "Utility/Math/Random.h"				// ランダム

// マクロ
#include "Engine/Common/Macros/ClassMacros.h"	// クラス用マクロ

//==========================================================================================
// 
// 補助クラス・関数
// 
//==========================================================================================
#include "Utility/Debug/DebugLog.h"				// デバッグ用ログ
#include "Utility/GUID/GUID.h"					// GUID
#include "Utility/JSONHelper/JSONHelper.h"		// Jsonヘルパー
#include "Utility/Pool/HandlePool/HandlePool.h"	// ハンドル管理ストレージ
#include "Utility/Pool/ItemPool/ItemPool.h"		// 実体管理ストレージ
#include "Utility/Pool/RangePool/RangePool.h"	// レンジ管理ストレージ

#include "D3D12/D3D12Helper.h"					// D3D12関連のヘルパー関数

//==========================================================================================
// 
// 保存
// 
//==========================================================================================
#include "Persistence/Archive/Archive.h"

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

	inline Index GetIndex(ID a_id)
	{
		return Index(a_id & 0xFFFF);
	}
	inline Generation GetGeneration(ID a_id)
	{
		return Generation(a_id >> 16);
	}
	inline ID GetID(Index a_idx,Generation a_gen)
	{
		return ID(a_gen) << 16 | a_idx;
	}
}


#include "Engine/Resource/Common/Common.h"


//==========================================================================================
// 
// DirectX12ラッパー
// 
//==========================================================================================
#include "D3D12/D3D12Common.h"

//------------------------------------------------------------------------------------------
// オブジェクト
//------------------------------------------------------------------------------------------
#include "D3D12/D3DObject/PipeLineState/PipelineState.h"

//------------------------------------------------------------------------------------------
// バッファー
//------------------------------------------------------------------------------------------
#include "Engine/D3D12/GPUBuffer/VertexBuffer/DynamicVertexBuffer.h"			// 頂点バッファ
#include "Engine/D3D12/GPUBuffer/IndexBuffer/DynamicIndexBuffer.h"				// ダイナミックインデックスバッファ
#include "Engine/D3D12/GPUBuffer/StructuredBuffer/StaticStructuredBuffer.h"		// スタティックストラクチャバッファ
//#include "Engine/D3D12/GPUBuffer/RWStructuredBuffer/RWStructuredBuffer.h"		// GPU用UAV構造体バッファ
#include "Engine/Resource/Data/Vertex/Vertex.h"									// 頂点データ
//==========================================================================================
// 
// 入力
// 
//==========================================================================================
#include "Input/InputManager/InputManager.h"
#include "Input/InputCollector/InputCollector.h"
#include "Input/InputDevice/Axis/InputAxisBase.h"
#include "Input/InputDevice/Button/InputButtonBase.h"
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
// 当たり判定
// 
//==========================================================================================
#include "Collision/CollisionCommon.h"

//==========================================================================================
// 
// エディター
// 
//==========================================================================================
#include "Editor/Editor.h"
#include "Editor/ECSView/ComponentEdit/ComponentEdit.h"

//==========================================================================================
// 
// アニメーション
// 
//==========================================================================================
#include "Animation/AnimationEvaluator/AnimationEvalutor.h"









