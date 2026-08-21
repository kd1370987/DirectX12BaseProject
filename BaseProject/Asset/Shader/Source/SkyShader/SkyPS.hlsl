#include "../Mesh/MeshCommon.hlsli"

//==============================================================================
// SkyPS
//
// スカイ(Sky)シェーディングモデルのピクセルシェーダー。
//
// ・GBuffer には一切書かない。ライティングの計算対象にならないので、
//   ここで出した色がそのまま最終的な空の色になる。
// ・出力先はディファードライティング後の HDR バッファ(AfterLighting)。
//   1.0 を超えて構わない(R16G16B16A16_FLOAT)。超えた分はブルームに乗る。
// ・明るさの調整は SkyOption の露出倍率(定数バッファ)で行う。
//   トーンマップと同じく「曲線ではなく倍率だけ動かしたい」種類の値なので、
//   シェーダーを差し替えずオプションから送っている。
// ・SV_Target1 へモーションベクターも出す。
//   空は GBufferPass を通らないため、ここで書かないと速度が0のまま残り、
//   カメラを振ったときに TAA が「動いていない」と判断して空が尾を引く。
//==============================================================================

struct PSOutput
{
	float4 color    : SV_Target0;	// AfterLighting(HDR)
	float2 velocity : SV_Target1;	// GBufferVelocity
};

// ルートシグネチャ定義
[RootSignature(MESHGLOBAL_ROOT_SIG)]
PSOutput PSMain(VertexOutput a_input)
{
	PSOutput _out;
	float2 _uv = a_input.uv;

	// -----------------------------------------------------------
	// マテリアル情報のフェッチ
	// -----------------------------------------------------------
	uint _instanceIdx = a_input.instanceID;
	InstanceData _inst = g_instanceData[_instanceIdx];
	Material _mat = g_materialData[_inst.materialOffset];

	// -----------------------------------------------------------
	// 空の色 : ベースカラーテクスチャ × マテリアル色
	// -----------------------------------------------------------
	float4 _baseTex = float4(1, 1, 1, 1);
	if (_mat.albedoIndex >= 0)
	{
		Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(_mat.albedoIndex)];
		_baseTex = albedoTex.Sample(smp, _uv);
	}
	float3 _skyColor = (_baseTex * _mat.baseColor).rgb;

	// マテリアルとは独立した自己発光。
	// テクスチャを持たない単色スカイでも明るさを足せるようにしておく
	_skyColor += _mat.emissiveAdd;

	// 露出を掛けて HDR のまま出す
	_out.color = float4(_skyColor * g_sky.exposure, 1.0f);

	// -----------------------------------------------------------
	// モーションベクター (Velocity)
	// -----------------------------------------------------------
	// クリップ空間からNDC座標へ変換
	float2 _curNDCPos = a_input.curClipPos.xy / a_input.curClipPos.w;
	float2 _prevNDCPos = a_input.prevClipPos.xy / a_input.prevClipPos.w;

	// NDC空間(幅2)の差分をUV空間(幅1)の差分へ変換（Y軸反転込み）
	_out.velocity = (_curNDCPos - _prevNDCPos) * float2(0.5f, -0.5f);

	return _out;
}
