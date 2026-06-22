#include "ParticleShader.hlsli"

[RootSignature(PARTICLE_ROOT_SIG)]
VSOutput VSMain(VSInput a_input)
{
	VSOutput _out;

	// パーティクルデータを取得
	ParticleData _particleData = g_particleBuffer[a_input.instID];

	// パーティクルの寿命判定
	if(_particleData.life <= 0.0f)
	{
		// 寿命が切れている場合は処理をスキップ
		_out.pos = float4(0,0,0,0);
		_out.color = float4(0,0,0,0);
		return _out;
	}
	
	// カメラの右、上ベクトルを取得
	float3 _camRight = g_camera.invView[0].xyz;
	float3 _camUp = g_camera.invView[1].xyz;

	// ビルボード計算
	float3 _localPos = a_input.pos.xyz * _particleData.size;

	// ワールド座標の算出
	float3 _worldPos = _particleData.pos + (_camRight * _localPos.x) + (_camUp * _localPos.y);

	// ワールド座標から射影空間へ変換
	_out.pos = mul(float4(_worldPos, 1.0f), g_camera.viewProj);

	// UV座標のパススルー
	_out.uv = a_input.uv;
	
	// 寿命に応じたフェードアウト
	float alpha = saturate(_particleData.life / 1.0f);
	_out.color = float4(1.0f,1.0f,1.0f, alpha);
	
	return _out;
}
