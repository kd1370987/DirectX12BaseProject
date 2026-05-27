#ifndef NORMAL_HLSLI
#define NORMAL_HLSLI

float3x3 GetNormalMatrix(float4x4 a_world)
{
	// inverse();これを使う必要があるが、
	// 重いため非等方スケールをしたいのなら
	// メッシュのトランスと一緒にＣＰＵ側からもらい受ける必要がある
	return transpose((float3x3) a_world);
}

// 法線をローカルからワールドに変換
float3 Normal_LocalToWorld(float3 a_normal, float4x4 a_worldMat)
{
	return normalize(mul(a_normal, GetNormalMatrix(a_worldMat)));
}

#endif
