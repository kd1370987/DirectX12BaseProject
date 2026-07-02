
// ピクセルシェーダーに渡す構造体
struct VertexOut
{
	float4 Pos : SV_Position;
	float3 Color : COLOR;
};

// ルートシグネチャ
#define TEST \
"RootFlags(0)"
// ルートシグネチャ定義
[RootSignature(TEST)]

// 出力するトポロジー（三角形）を指定
[outputtopology("triangle")]
// スレッドグループのサイズ（今回は1つのスレッドだけで1枚のポリゴンを作る）
[numthreads(1, 1, 1)]
void MSMain(
    uint gtid : SV_GroupThreadID,
    out indices uint3 tris[1], // 出力するインデックス配列
    out vertices VertexOut verts[3] // 出力する頂点配列
)
{
    // メッシュシェーダーの出力宣言（頂点3つ、プリミティブ1つ）
	SetMeshOutputCounts(3, 1);

    // 頂点0 (上・赤)
	verts[0].Pos = float4(0.0, 0.5, 0.0, 1.0);
	verts[0].Color = float3(1.0, 0.0, 0.0);

    // 頂点1 (右下・緑)
	verts[1].Pos = float4(0.5, -0.5, 0.0, 1.0);
	verts[1].Color = float3(0.0, 1.0, 0.0);

    // 頂点2 (左下・青)
	verts[2].Pos = float4(-0.5, -0.5, 0.0, 1.0);
	verts[2].Color = float3(0.0, 0.0, 1.0);

    // インデックスの設定 (0, 1, 2の頂点を結んで1枚のポリゴンにする)
	tris[0] = uint3(0, 1, 2);
}
