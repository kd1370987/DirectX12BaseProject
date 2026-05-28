// ルートパラメターズ
#include "../../Common/CB/CBCamera.hlsli"
#include "../../Common/RootParameters/BufferIndex.hlsli"
#include "../../Common/RootParameters/InstanceData.hlsli"

// ヘルパー関数
#include "../../Common/Math/Transform.hlsli"
#include "../../Common/Math/Normal.hlsli"

#include "../RootSignatureLayout.hlsli"

#define DEBUGLINE_ROOT_SIG \
RS_FLAGS","\
RS_CAMERA_CB ","\
RS_BUFFERINDEX_CB ","\
RS_INSTANCE_DATA_TABLE

// 頂点シェーダー入力構造体
struct VSInput
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

// アニメーション頂点シェーダー入力構造体
struct AnimationVSInput
{
	float3 pos : POSITION;			// 頂点座標
	uint4 skinIndex : SKININDEX;	// スキンメッシュのボーンインデックス（何番目のボーンに影響しているかのデータ（最大４））
	float4 skinWeight : SKINWEIGHT; // ボーンの影響度（最大４）
};

struct VSOutput
{
	float4 svPos : SV_Position;
	float4 color : COLOR;
};
