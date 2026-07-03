#include "MeshCommon.hlsli"

// ルートシグネチャ定義
[RootSignature(MESHGLOBAL_ROOT_SIG)]
// 出力するトポロジーを指定
[outputtopology("triangle")]
// スレッドグループのサイズ
[numthreads(128, 1, 1)]
void MSMain(
	uint a_gtid : SV_GroupThreadID,			// スレッドグループ内のローカルID（0 ～ 127）
	uint3 a_gid : SV_GroupID,				// グループID(x : メッシュレットID,y : インスタンスID)
	out indices uint3 primIndices[126],
	out vertices VertexOutput outVerts[64]
)
{
	// ---------------------------------------------------------
	// インスタンスとメッシュレットの情報取得
	// DispatchMesh( meshletCount , instanceCount , 1);で呼ばれる想定
	uint _instanceID = g_baseInstanceIndex + a_gid.y; // 同時に複数描画する場合 Y 軸で個数を設定
	InstanceData _inst = g_instanceData[_instanceID];

	// 対象のメッシュレットを取得
	uint _globalMeshletIndex = _inst.meshletOffset + a_gid.x;
	Meshlet _m = g_meshletData[_globalMeshletIndex];

	// MSの出力を宣言 : これ以降,out指定した outVerts / primIndices に書き込みができる
	SetMeshOutputCounts(_m.vertexCount, _m.primitiveCount);

	// ---------------------------------------------------------
	// 頂点の処理 : スレッド 0 ～ 63 が平行して担当
	if (a_gtid < _m.vertexCount)
	{
		// ユニーク頂点インデックスの取得
		uint _uviAddr = (_inst.uviOffset + _m.vertexOffset + a_gtid) * 4;
		uint _localVertexIndex = g_uniqueVertexIndices.Load(_uviAddr);

		// メガバッファ内の絶対インデックスを計算
		uint _glovalVertexIndex = _inst.vertexOffset + _localVertexIndex;

		// 頂点データのアンパック
		uint _vertexStride = 84; // 84バイトは重いので40バイトを切る頂点構造体にすべき
		uint _vAddr = _glovalVertexIndex * _vertexStride;

		float3 _pos = asfloat(g_vertices.Load3(_vAddr));
		float3 _nor = asfloat(g_vertices.Load3(_vAddr + 12));
		float2 _uv = asfloat(g_vertices.Load2(_vAddr + 24));
		float3 _tan = asfloat(g_vertices.Load3(_vAddr + 32));
		float4 _color = asfloat(g_vertices.Load4(_vAddr + 44));
		// スキニング系はいったん捨てる

		// ワールド変換と出力の構築
		VertexOutput _vout;
		float4 _worldPos = mul(_inst.worldMat, float4(_pos, 1.0f));
		_vout.pos = mul(g_camera.viewProj, _worldPos);
		_vout.worldPos = _worldPos.xyz;
		float4 _prevWorldPos = mul(_inst.prevWorldMat, float4(_pos, 1.0f));
		_vout.curClipPos = _vout.pos;
		_vout.prevClipPos = mul(g_camera.prevViewProj, _prevWorldPos);

		// 法線と接戦の回転 : スケールを考慮していないためあとで追加必須
		_vout.normal = normalize(mul((float3x3) _inst.worldMat, _nor));
		_vout.tangent = normalize(mul((float3x3) _inst.worldMat, _tan));
		_vout.uv = _uv;
		_vout.instanceID = _instanceID;

		outVerts[a_gtid] = _vout;
	}
	
	// ---------------------------------------------------------
	// プリミティブの処理 : スレッド 0 ～ 125が並行して担当
	if (a_gtid < _m.primitiveCount)
	{
		uint _primAddr = (_inst.primitiveOffset + _m.primitiveOffset + a_gtid) * 4;
		uint _packedIndices = g_primitiveIndices.Load(_primAddr);

		uint3 _indices;
		_indices.x = _packedIndices & 0x3FF; // 0 ～ 9bit
		//_indices.y = (_packedIndices >> 10) & 0x3FF;// 10 ～ 19bit
		//_indices.z = (_packedIndices >> 20) & 0x3FF;// 20 ～ 29bit
		_indices.z = (_packedIndices >> 10) & 0x3FF;
		_indices.y = (_packedIndices >> 20) & 0x3FF;
		
		primIndices[a_gtid] = _indices;
	}

}
