// スキニングディスパッチ1回ぶんの担当範囲。
// 頂点もボーンもメガバッファ1本に積んであるので、開始位置と個数で切り出す
#ifndef ROOTPARAM_SKINNING_INFO_HLSLI
#define ROOTPARAM_SKINNING_INFO_HLSLI

struct SkinningInfo
{
	uint vertexStart;			// 元頂点の開始インデックス
	uint animatedVertStart;		// 変形後頂点の書き込み開始インデックス
	uint vertexCount;
	uint boneOffset;			// このキャラのボーン開始位置
};

#endif
