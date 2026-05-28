// ルートパラメターズ
#include "../../Common/CB/CBCamera.hlsli"
#include "../../Common/RootParameters/BufferIndex.hlsli"
#include "../../Common/RootParameters/InstanceData.hlsli"
#include "../../Common/RootParameters/SubsetData.hlsli"
#include "../../Common/RootParameters/BonePalletData.hlsli"
#include "../../Common/RootParameters/Material.hlsli"

// 計算用
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

// ルートシグネチャ作成
#include "../RootSignatureLayout.hlsli"

#define ZPRE_ROOT_SIG \
RS_FLAGS ","\
RS_CAMERA_CB ","\
RS_BUFFERINDEX_CB ","\
RS_INSTANCE_DATA_TABLE ","\
RS_SUBSET_DATA_TABLE ","\
RS_BONEPALLET_DATA_TABLE ","\
RS_MATERIAL_TABLE ","\
RS_STATIC_SAMPLER

// サンプラー
SamplerState smp : register(s0);

// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
};


