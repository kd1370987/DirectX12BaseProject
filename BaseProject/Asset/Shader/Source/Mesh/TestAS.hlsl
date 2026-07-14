#include "MeshCommon.hlsli"

// グループ内で共有するPayload変数
groupshared PayloadStruct s_Payload;

// 可視性チェック
bool IsVisible(MeshletCullData a_cullData,float4x4 a_worldMat)
{
	// ローカル座標系の球の中心にワールド行列を掛けて、ワールド座標に変換する
	float3 _centerWorld = mul(float4(a_cullData.BoundingSphereCenter, 1.0f), a_worldMat).xyz;
	
	float _radiusWorld = a_cullData.BoundingSphereRadius * a_worldMat._11_22_33_44;
	
	for (int _idx = 0; _idx < 6; ++_idx)
	{
		// 座標と平面との距離計算
		float4 _plane = g_camera.frustumPlanes[_idx];
		// 平面の方程式 (dot(Normal, Point) + Distance)
		if (dot(_plane.xyz, _centerWorld) + _plane.w < -_radiusWorld)
		{
			return false;
		}
	}

	return true;
}



[numthreads(32, 1, 1)]
void ASMain(
	uint a_gtid : SV_GroupThreadID, // スレッドのローカルID (0 ~ 31)
    uint3 a_gid : SV_GroupID // グループID
)
{
	// インスタンス情報の取得
	uint _instanceID = g_baseInstanceIndex + a_gid.y;
	InstanceData _inst = g_instanceData[_instanceID];

	// メッシュレットIDを計算 (グループID × スレッド数 + ローカルID)
	uint _dispatchThreadID = (a_gid.x * 32) + a_gtid;

	// カリングデータのインデックス
	uint _cullDataIndex = _inst.cullStart + _dispatchThreadID;

	// 可視性チェック
	bool _isVisible = IsVisible(g_cullData[_cullDataIndex], _inst.worldMat);
	
	
	// 可視性チェックが成功したメッシュレットをペイロードに格納
	if(_isVisible)
	{
		uint _idx = WavePrefixCountBits(_isVisible);	// Wave中で何番目に可視性チェックが成功したか
		s_Payload.MeshletIndices[_idx] = _dispatchThreadID; // 可視性チェックが成功した場合にメッシュレットのインデックス
	}

	uint _visibleCount = WaveActiveCountBits(_isVisible);	// Wave中で可視性チェックが成功したActiveLaneの数
	DispatchMesh(_visibleCount, 1, 1, s_Payload); // 可視性チェックが成功した数分のスレッドグループが生成される
}
//[numthreads(1, 1, 1)]
//void ASMain(in uint3 a_groupID : SV_GroupID)
//{
//	PayloadStruct _payload;
//	_payload.myArbitaryData = a_groupID.x;
//	DispatchMesh(1, 1, 1, _payload);
//}
