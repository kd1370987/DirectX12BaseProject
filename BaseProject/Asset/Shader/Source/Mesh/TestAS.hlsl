#include "MeshCommon.hlsli"

// グローバル空間で共有メモリとして宣言
groupshared PayloadStruct s_payload;

// 可視メッシュレット数を数えるための共有カウンター
groupshared uint s_VisibleCount;

// 可視性チェック
bool IsVisible(MeshletCullData a_cullData, float4x4 a_worldMat)
{
	// ローカル座標系の球の中心にワールド行列を掛けて、ワールド座標に変換する
	float3 _centerWorld = mul(float4(a_cullData.BoundingSphereCenter, 1.0f), a_worldMat).xyz;
	
	//float _radiusWorld = a_cullData.BoundingSphereRadius * a_worldMat._11_22_33_44;
	float _scaleX = length(a_worldMat[0].xyz);
	float _scaleY = length(a_worldMat[1].xyz);
	float _scaleZ = length(a_worldMat[2].xyz);
	float _radiusWorld = a_cullData.BoundingSphereRadius * max(_scaleX, max(_scaleY, _scaleZ));
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
    uint a_gtid : SV_GroupThreadID,
    uint3 a_groupID : SV_GroupID)
{
 //   // ペイロードの書き込みは代表スレッド1つにやらせる
	//if (a_gtid == 0)
	//{
	//	s_payload.SurvivingMeshlets = a_groupID.x;
	//}
    
 //   // 書き込みが終わるのをグループ内の全スレッドで待つ
	//GroupMemoryBarrierWithGroupSync();

	//DispatchMesh(1, 1, 1, s_payload);

	// --------------------------------------------------
	// ペイロードとカウンターの初期化（スレッド0が行う）
	// --------------------------------------------------
	uint _instanceID = g_baseInstanceIndex + a_groupID.y;
	
	if (a_gtid == 0)
	{
		s_payload.instanceID = _instanceID;
		s_VisibleCount = 0; // カウンターを0にリセット
	}
	
	// 初期化が終わるのを全スレッドで確実に待つ！
	GroupMemoryBarrierWithGroupSync();

	InstanceData _inst = g_instanceData[_instanceID];

	// メッシュレットIDを計算
	uint _localMeshletID = (a_groupID.x * 32) + a_gtid;
	uint _globalMeshletID = _inst.meshletOffset + _localMeshletID;

	// --------------------------------------------------
	// 可視性チェック
	// --------------------------------------------------
	bool _isVisible = false;
	if (_localMeshletID < _inst.meshletCount)
	{
		uint _cullDataIndex = _inst.cullStart + _localMeshletID;
		_isVisible = IsVisible(g_cullData[_cullDataIndex], _inst.worldMat);
		//_isVisible = true; // 今はテスト用に強制表示
	}
	
	// --------------------------------------------------
	// ペイロードへの格納（Wave関数ではなくInterlockedAddを使う）
	// --------------------------------------------------
	if (_isVisible)
	{
		uint _idx = 0;
		// s_VisibleCount に 1 を足し、足す前の値を _idx に取得する
		InterlockedAdd(s_VisibleCount, 1, _idx);
		
		s_payload.MeshletIndices[_idx] = _globalMeshletID;
	}

	// ペイロードへの書き込みが終わるのを全スレッドで待つ
	GroupMemoryBarrierWithGroupSync();

	// --------------------------------------------------
	// メッシュシェーダーの起動
	// --------------------------------------------------
	DispatchMesh(s_VisibleCount, 1, 1, s_payload);

}
