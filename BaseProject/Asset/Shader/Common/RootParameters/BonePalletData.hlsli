// スキニング用ボーン行列。全ワールドで1本のパレットを共有する
#ifndef ROOTPARAM_BONE_PALLET_DATA_HLSLI
#define ROOTPARAM_BONE_PALLET_DATA_HLSLI

struct BonePallet
{
	row_major float4x4 mat;
};

#endif
