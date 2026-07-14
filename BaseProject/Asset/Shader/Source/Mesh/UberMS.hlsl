#include "MeshCommon.hlsli"

uint3 UnpackPrimitive(uint a_primitive)
{
	return uint3(a_primitive & 0x3FF, (a_primitive >> 10) & 0x3FF, (a_primitive >> 20) & 0x3FF);
}

// ルートシグネチャ定義
[RootSignature(MESHGLOBAL_ROOT_SIG)]

// 出力するトポロジーを指定
[outputtopology("triangle")]
// スレッドグループのサイズ
[numthreads(128, 1, 1)]
void MSMain(
	uint a_gtid : SV_GroupThreadID,			// スレッドグループ内のローカルID（0 ～ 127）
	uint3 a_gid : SV_GroupID,				// ASから起動されたグループのインデックス
	in payload PayloadStruct a_meshPayload,
	out indices uint3 primIndices[126],
	out vertices VertexOutput outVerts[64]
)
{
	
	uint _arbitraryData = a_meshPayload.SurvivingMeshlets;
	// ペイロードから自分が担当する「本当のメッシュレットID」を取得する
	//uint realMeshletIndex = a_meshPayload.MeshletIndices[a_gid.x];
	
	// ---------------------------------------------------------
	// インスタンスとメッシュレットの情報取得
	// ---------------------------------------------------------
	uint _instanceID = g_baseInstanceIndex + a_gid.y;
	//uint _instanceID = a_meshPayload.instanceID;
	InstanceData _inst = g_instanceData[_instanceID];

	// 対象のメッシュレットを取得
	//uint _globalMeshletIndex = _inst.meshletOffset + a_gid.x;
	uint _globalMeshletIndex = _inst.meshletOffset + _arbitraryData;
	//uint _globalMeshletIndex = _inst.meshletOffset + realMeshletIndex;
	Meshlet _m = g_meshletData[_globalMeshletIndex];

	// 上限を超えないように安全装置を入れる
	uint _vCount = min(_m.vertexCount,64);
	uint _pCount = min(_m.primitiveCount,126);

	// MSの出力を宣言
	SetMeshOutputCounts(_m.vertexCount, _m.primitiveCount);

	// ---------------------------------------------------------
	// 頂点の処理 : スレッド 0 ～ 63 が並行して担当
	// ---------------------------------------------------------
	if (a_gtid < _m.vertexCount)
	{
		// ユニーク頂点インデックスの取得 
		uint _globalUVI = _inst.uviOffset + _m.vertexOffset + a_gtid;
		//uint _globalUVI = _inst.uviOffset + _m.vertexOffset + _arbitraryData;
		uint _localVertexIndex = g_uniqueVertexIndices[_globalUVI];

		Vertex _v;
		
		if(_inst.isAnimated == 0)
		{
			// メガバッファ内の絶対インデックスを計算
			uint _globalVertexIndex = _inst.vertexOffset + _localVertexIndex;

			// 頂点データの取得
			_v = g_vertices[_globalVertexIndex];
		}
		else
		{
			// メガバッファ内の絶対インデックスを計算
			uint _globalVertexIndex = _inst.animatedVertexStart + _localVertexIndex;

			// 頂点データの取得
			_v = g_animatedVertices[_globalVertexIndex];
		}

		// ---------------------------------------------------------
		// ワールド変換と出力の構築
		// ---------------------------------------------------------
		VertexOutput _vout;
		
		// 座標変換
		float4 _worldPos = mul(float4(_v.pos, 1.0f), _inst.worldMat);
		_vout.pos = mul(_worldPos, g_camera.viewProj);
		_vout.worldPos = _worldPos.xyz;
		
		float4 _prevWorldPos = mul(float4(_v.pos, 1.0f), _inst.prevWorldMat);
		_vout.curClipPos = _vout.pos;
		_vout.prevClipPos = mul(_prevWorldPos, g_camera.prevViewProj);

		// 法線と接線の回転
		_vout.normal = normalize(mul(_v.normal, (float3x3) _inst.worldMat));
		_vout.tangent = normalize(mul(_v.tangent, (float3x3) _inst.worldMat));
		
		_vout.uv = _v.uv;
		_vout.instanceID = _instanceID;

		outVerts[a_gtid] = _vout;
	}
	
	// ---------------------------------------------------------
	// プリミティブの処理 : スレッド 0 ～ 125 が並行して担当
	// ---------------------------------------------------------
	if (a_gtid < _m.primitiveCount)
	{
		// プリミティブインデックスの取得 
		uint _globalPrimIndex = _inst.primitiveOffset + _m.primitiveOffset + a_gtid;
		uint3 _indices = UnpackPrimitive(g_primitiveIndices[_globalPrimIndex]);
		
		primIndices[a_gtid] = _indices;
	}
}
