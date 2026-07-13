#include "MeshCommon.hlsli"

[numthreads(1, 1, 1)]
void ASMain(in uint3 a_groupID : SV_GroupID)
{
	PayloadStruct _payload;
	_payload.myArbitaryData = a_groupID.x;
	DispatchMesh(1, 1, 1, _payload);
}
//// 1. NormalConeのアンパック（-1.0 ～ 1.0 に戻す）
//int x = (int) (cullData.NormalCone & 0xFF);
//int y = (int) ((cullData.NormalCone >> 8) & 0xFF);
//int z = (int) ((cullData.NormalCone >> 16) & 0xFF);
//float3 normalAxis = normalize(float3(x, y, z) / 127.5f - 1.0f);

//// 2. 広がり角（コサイン値）のアンパック
//int w = (int) ((cullData.NormalCone >> 24) & 0xFF);
//float cutoff = (float) w / 255.0f; // これが w = -cos(a + 90) の値

//// 3. コーンの頂点（Apex）の計算
//// ローカル空間での球の中心から、法線ベクトルの逆向きにオフセット分ズラす
//float3 apex = cullData.BoundingSphereCenter - normalAxis * cullData.ApexOffset;

//// 4. ワールド空間に変換（インスタンス行列を使用）
//float3 apexWorld = mul(float4(apex, 1.0f), _inst.worldMat).xyz;
//float3 axisWorld = normalize(mul(normalAxis, (float3x3) _inst.worldMat));

//// 5. カメラからApexへのベクトル
//float3 viewDir = normalize(apexWorld - g_camera.position);

//// 6. バックフェイス判定
//// カメラの視線ベクトルと法線コーンの軸が成す角度が、cutoffを超えていれば「裏」
//bool isBackFace = dot(viewDir, axisWorld) >= cutoff;

//if (isBackFace) {
//    // 描画しない（カリングする）
//}
