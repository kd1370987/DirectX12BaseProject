// カメラの定数バッファ
struct Camera
{
	float3 pos;						// カメラ座標
	float pad;						

	float4x4 view;					// ビュー行列
	float4x4 proj;					// プロジェクション行列

	float4x4 invView;				// 逆ビュー行列
	float4x4 invProj;				// 逆プロジェクション行列

	float4x4 invViewProj;			// 逆ビュープロジェクション行列
};
cbuffer cbCamera : register(b0)
{
	Camera g_camera;
}

struct InstanceData
{
	uint vertexIdx; // SRV
	uint indexIdx; // SRV

	// このメッシュのマテリアル群がg_materialDataの何番目から始まるか
	uint materialOffset;
	uint pad;
};

// 頂点構造体
struct Vertex
{
	float3 pos;
	float3 normal;
	float2 uv;
	float3 tangent;
	float4 color;
};

// マテリアル構造体
struct Material
{
	float4 baseColor;
	float3 emissive;
	float metallic;
	
	float roughness;
	int baseIndex;
	int metaRoughnessIndex;
	int emissiveIndex;
	
	int normalIndex;
	uint startIndexLocation; // このサブメッシュのインデックスバッファが何番目から始まるか
	float2 pad;
};

RaytracingAccelerationStructure g_raytracingWorld : register(t0);	// レイトレワールド
RWTexture2D<float4> gOutPut : register(u0);							// カラー出力先
StructuredBuffer<InstanceData> g_instanceData : register(t1);		// インスタンスごとのデータ
StructuredBuffer<Material> g_materialData : register(t2);			// インスタンスごとのデータ
sampler gSamp : register(s0);

// UV座標を取得
float2 GetUV(BuiltInTriangleIntersectionAttributes a_attribs, InstanceData instance, uint primID, Material material)
{
	float3 _barycentrics = float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, a_attribs.barycentrics.y);

	// メインヒープから、このインスタンスのインデックスバッファと頂点バッファを直接取得
	StructuredBuffer<int> indexBuff = ResourceDescriptorHeap[instance.indexIdx];
	StructuredBuffer<Vertex> vertexBuff = ResourceDescriptorHeap[instance.vertexIdx];

	// プリミティブIDではなくサブメッシュ番号を取得する
	uint _baseIndexLocation = material.startIndexLocation + (primID * 3);

	// プリミティブインデックスから頂点番号を取得
	uint _v0 = indexBuff[_baseIndexLocation];
	uint _v1 = indexBuff[_baseIndexLocation + 1];
	uint _v2 = indexBuff[_baseIndexLocation + 2];

	// UV取得
	float2 _uv0 = vertexBuff[_v0].uv;
	float2 _uv1 = vertexBuff[_v1].uv;
	float2 _uv2 = vertexBuff[_v2].uv;

	float2 _uv = _barycentrics.x * _uv0 + _barycentrics.y * _uv1 + _barycentrics.z * _uv2;
	return _uv;
}
// 法線の取得
float3 GetNormal(BuiltInTriangleIntersectionAttributes a_attribs, float2 a_uv, InstanceData instance, uint primID, Material material)
{
	float3 _barycentrics = float3(1.0 - a_attribs.barycentrics.x - a_attribs.barycentrics.y, a_attribs.barycentrics.x, a_attribs.barycentrics.y);
	
	// 同様にメインヒープから取得
	StructuredBuffer<int> indexBuff = ResourceDescriptorHeap[instance.indexIdx];
	StructuredBuffer<Vertex> vertexBuff = ResourceDescriptorHeap[instance.vertexIdx];
	
	// プリミティブIDではなくサブメッシュ番号を取得する
	uint _baseIndexLocation = material.startIndexLocation + (primID * 3);
	
	uint _v0 = indexBuff[_baseIndexLocation];
	uint _v1 = indexBuff[_baseIndexLocation + 1];
	uint _v2 = indexBuff[_baseIndexLocation + 2];
	
	// 法線取得
	float3 _n0 = vertexBuff[_v0].normal;
	float3 _n1 = vertexBuff[_v1].normal;
	float3 _n2 = vertexBuff[_v2].normal;
	float3 _normal = normalize(_barycentrics.x * _n0 + _barycentrics.y * _n1 + _barycentrics.z * _n2);

	// タンジェント
	float3 _t0 = vertexBuff[_v0].tangent;
	float3 _t1 = vertexBuff[_v1].tangent;
	float3 _t2 = vertexBuff[_v2].tangent;
	float3 _tangent = normalize(_barycentrics.x * _t0 + _barycentrics.y * _t1 + _barycentrics.z * _t2);

	// ビノーマルを計算
	float3 _binormal = normalize(cross(_tangent, _normal));

	// 法線マップもマテリアルに仕込んだベースインデックスから取得
	Texture2D normalTex = ResourceDescriptorHeap[material.normalIndex];
	float3 _binSpaceNormal = normalTex.SampleLevel(gSamp, a_uv, 0).rgb;
	_binSpaceNormal = (_binSpaceNormal * 2.0f) - 1.0f;

	// タンジェント空間からワールド空間に変換
	_normal = _tangent * _binSpaceNormal.x + _binormal * _binSpaceNormal.y + _normal * _binSpaceNormal.z;
	_normal = normalize(mul(_normal, (float3x3) WorldToObject3x4()));
	return _normal;
}
