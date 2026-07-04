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
	// DispatchMesh( meshletCount, instanceCount, 1 ); で呼ばれる想定
	// ---------------------------------------------------------
	uint _instanceID = g_baseInstanceIndex + a_gid.y;
	InstanceData _inst = g_instanceData[_instanceID];

	// 対象のメッシュレットを取得
	uint _globalMeshletIndex = _inst.meshletOffset + a_gid.x;
	Meshlet _m = g_meshletData[_globalMeshletIndex];

	// MSの出力を宣言 : これ以降, outVerts / primIndices に書き込みが可能になる
	SetMeshOutputCounts(_m.vertexCount, _m.primitiveCount);

	// ---------------------------------------------------------
	// 頂点の処理 : スレッド 0 ～ 63 が並行して担当
	// ---------------------------------------------------------
	if (a_gtid < _m.vertexCount)
	{
		// ユニーク頂点インデックスの取得
		uint _uviAddr = (_inst.uviOffset + _m.vertexOffset + a_gtid) * 4;
		uint _localVertexIndex = g_uniqueVertexIndices.Load(_uviAddr);

		// メガバッファ内の絶対インデックスを計算
		uint _globalVertexIndex = _inst.vertexOffset + _localVertexIndex;

		// 頂点データのアンパック
		uint _vertexStride = 84; // TODO: 84バイトは重いので40バイトを切る頂点構造体への最適化を推奨
		uint _vAddr = _globalVertexIndex * _vertexStride;

		float3 _pos = asfloat(g_vertices.Load3(_vAddr));
		float3 _nor = asfloat(g_vertices.Load3(_vAddr + 12));
		float2 _uv = asfloat(g_vertices.Load2(_vAddr + 24));
		float3 _tan = asfloat(g_vertices.Load3(_vAddr + 32));
		float4 _color = asfloat(g_vertices.Load4(_vAddr + 44));
		// ※スキニング系はいったん捨てる

		// ---------------------------------------------------------
		// ワールド変換と出力の構築
		// 行優先行列なので、乗算はすべて「mul(ベクトル, 行列)」の順序で行う
		// ---------------------------------------------------------
		VertexOutput _vout;
		
		// 座標変換
		float4 _worldPos = mul(float4(_pos, 1.0f), _inst.worldMat);
		_vout.pos = mul(_worldPos, g_camera.viewProj);
		_vout.worldPos = _worldPos.xyz;
		
		float4 _prevWorldPos = mul(float4(_pos, 1.0f), _inst.prevWorldMat);
		_vout.curClipPos = _vout.pos;
		_vout.prevClipPos = mul(_prevWorldPos, g_camera.prevViewProj);

		// 法線と接線の回転
		// ※非均等スケールを考慮する場合は、ワールド行列の逆転置行列(Inverse Transpose)を掛ける必要があります
		_vout.normal = normalize(mul(_nor, (float3x3) _inst.worldMat));
		_vout.tangent = normalize(mul(_tan, (float3x3) _inst.worldMat));
		
		_vout.uv = _uv;
		_vout.instanceID = _instanceID;

		outVerts[a_gtid] = _vout;
	}
	
	// ---------------------------------------------------------
	// プリミティブの処理 : スレッド 0 ～ 125 が並行して担当
	// ---------------------------------------------------------
	if (a_gtid < _m.primitiveCount)
	{
		uint _primAddr = (_inst.primitiveOffset + _m.primitiveOffset + a_gtid) * 4;
		uint _packedIndices = g_primitiveIndices.Load(_primAddr);

		uint3 _indices;
		_indices.x = _packedIndices & 0x3FF; // 0 ～ 9bit
		_indices.y = (_packedIndices >> 10) & 0x3FF; // 10 ～ 19bit
		_indices.z = (_packedIndices >> 20) & 0x3FF; // 20 ～ 29bit
		
		primIndices[a_gtid] = _indices;
	}
}
