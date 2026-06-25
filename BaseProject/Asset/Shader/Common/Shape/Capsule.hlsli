// インクルードガード
#ifndef SHAPE_CAPSULE
#define SHAPE_CAPSULE

#define PI 3.14159265359f
float3 GetCapsulePoint(uint vID)
{
	float r = 0.5f;
	float h = 0.5f; // 円柱部分の半分の高さ
	
	// 1. 縦のライン 4本 (8頂点: 0~7)
	if (vID < 8)
	{
		float y = (vID % 2 == 0) ? -h : h;
		uint lineIdx = vID / 2;
		if (lineIdx == 0)
			return float3(r, y, 0);
		if (lineIdx == 1)
			return float3(-r, y, 0);
		if (lineIdx == 2)
			return float3(0, y, r);
		return float3(0, y, -r);
	}
	
	// 2. 上下のフタ（XZ平面の円） (64頂点: 8~71)
	if (vID < 72)
	{
		uint localID = vID - 8;
		bool isTop = (localID < 32);
		uint ptID = localID % 32;
		float angle = ((ptID / 2) + (ptID % 2)) * (2.0f * PI / 16.0f);
		return float3(cos(angle) * r, isTop ? h : -h, sin(angle) * r);
	}
	
	// 3. 上下の半球アーク（XY, YZ平面の半円） (64頂点: 72~135)
	if (vID < 136)
	{
		uint localID = vID - 72;
		uint arcIdx = localID / 16; // 0:TopXY, 1:TopYZ, 2:BotXY, 3:BotYZ
		uint ptID = localID % 16;
		float angle = ((ptID / 2) + (ptID % 2)) * (PI / 8.0f); // 0 ~ PI
		
		bool isTop = (arcIdx < 2);
		bool isXY = (arcIdx % 2 == 0);
		
		if (!isTop)
			angle += PI; // 下半球は PI ~ 2PI にシフト
		
		float c = cos(angle) * r;
		float s = sin(angle) * r;
		
		float3 pos = isXY ? float3(c, s, 0) : float3(0, s, c);
		pos.y += isTop ? h : -h;
		return pos;
	}
	
	return float3(0, 0, 0);
}

#endif
