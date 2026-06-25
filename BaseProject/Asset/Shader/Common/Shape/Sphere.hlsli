// インクルードガード
#ifndef SHAPE_SPHERE
#define SHAPE_SPHERE

#define PI 3.14159265359f
float3 GetSpherePoint(uint vID)
{
	uint segments = 16;
	uint circleIdx = vID / 32; // 0=YZ, 1=XZ, 2=XY
	uint ptID = vID % 32;
	
	// LINELIST用に [0,1], [1,2], [2,3]... と繋がる角度を計算
	float angle = ((ptID / 2) + (ptID % 2)) * (2.0f * PI / segments);
	
	float c = cos(angle) * 0.5f; // 半径0.5
	float s = sin(angle) * 0.5f;
	
	if (circleIdx == 0)
		return float3(0, c, s);
	if (circleIdx == 1)
		return float3(c, 0, s);
	return float3(c, s, 0);
}
#endif
