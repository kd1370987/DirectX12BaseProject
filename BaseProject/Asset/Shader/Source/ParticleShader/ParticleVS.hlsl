#include "ParticleShader.hlsli"

[RootSignature(PARTICLE_ROOT_SIG)]
VSOutput VSMain(VSInput a_input)
{
	VSOutput _out;

	// パーティクルデータを取得
	ParticleData _particleData = g_particleBuffer[a_input.instID];

	// パーティクルの寿命判定
	if (_particleData.life <= 0.0f)
	{
		// 寿命が切れている場合は処理をスキップ

		_out.pos = float4(0, 0, 0, 0);
		_out.color = float4(1, 0, 0, 1);
		return _out;
	}

	float _debugSize = (_particleData.size <= 0.0f) ? 1.0f : _particleData.size;
	float3 _debugPos = g_camera.invView[3].xyz + (g_camera.invView[2].xyz * 5.0f);
	
	// カメラの右、上ベクトルを取得
	float3 _camRight = g_camera.invView[0].xyz;
	float3 _camUp = g_camera.invView[1].xyz;

	// ---------------------------------------------------------------
	// 板ポリの軸を決める
	// 既定はカメラ正面のビルボード。進行方向に合わせる指定なら、
	// 板の縦軸(+Y = テクスチャの上)を速度ベクトルへ向ける。
	// 板ポリの頂点は y=+1 が v=0 なので、+Y = 画像の上端になる。
	// ---------------------------------------------------------------
	float3 _axisX = _camRight;
	float3 _axisY = _camUp;
	float _stretch = 1.0f;

	if (g_draw.orientation != PARTICLE_ORIENT_BILLBOARD)
	{
		float3 _velocity = _particleData.velocity;
		float _speedSq = dot(_velocity, _velocity);

		// 止まっているものは向きが決まらないので、そのままビルボードで出す
		if (_speedSq > 1e-8f)
		{
			float3 _dir = _velocity * rsqrt(_speedSq);

			if (g_draw.orientation == PARTICLE_ORIENT_VELOCITY_BILLBOARD)
			{
				// 進行方向を画面へ射影して、その向きへ板を回す。
				// 常にカメラ正面を向いたままなので、真横から見ても板が消えない
				float2 _screenDir = float2(dot(_dir, _camRight), dot(_dir, _camUp));
				if (dot(_screenDir, _screenDir) > 1e-8f)
				{
					_screenDir = normalize(_screenDir);
					_axisY = (_camRight * _screenDir.x) + (_camUp * _screenDir.y);
					_axisX = (_camRight * _screenDir.y) - (_camUp * _screenDir.x);
					_stretch = g_draw.stretch;
				}
			}
			else
			{
				// ワールドの進行方向をそのまま縦軸にする。
				// カメラへ真っ直ぐ向かってくるものは短く(点に)見えるので、
				// 弾道や火花のように「線として飛んでいる」表現向き
				float3 _toCamera = g_camera.cameraPos.xyz - _particleData.pos;
				float3 _side = cross(_toCamera, _dir);
				if (dot(_side, _side) > 1e-8f)
				{
					_axisY = _dir;
					_axisX = normalize(_side);
					_stretch = g_draw.stretch;
				}
			}
		}
	}

	// ---------------------------------------------------------------
	// 寿命のどこまで進んだか (0 = 生まれた瞬間 / 1 = 消える直前)
	// サイズ・色・フェードはすべてこれを基準にする
	// ---------------------------------------------------------------
	float _startLife = max(_particleData.startLife, 1e-4f);
	float _lifeT = saturate(1.0f - (_particleData.life / _startLife));

	// 寿命に沿ったサイズ変化。
	// 煙は膨らみ(>1)、火花は縮む(<1)。1 なら変化しない
	float _sizeScale = lerp(1.0f, g_draw.endSizeScale, _lifeT);

	// ビルボード計算
	float3 _localPos = a_input.pos.xyz * (_particleData.size * _sizeScale);
	//float3 _localPos = a_input.pos.xyz * _debugSize;

	// 進行方向へ伸ばす(縦軸のみ)
	_localPos.y *= _stretch;

	// ワールド座標の算出
	float3 _worldPos = _particleData.pos + (_axisX * _localPos.x) + (_axisY * _localPos.y);
	//float3 _worldPos = _debugPos + (_camRight * _localPos.x) + (_camUp * _localPos.y);

	// ワールド座標から射影空間へ変換
	_out.pos = mul(float4(_worldPos, 1.0f), g_camera.viewProj);

	// UV座標のパススルー
	_out.uv = a_input.uv;
	
	// ---------------------------------------------------------------
	// 寿命に沿った色
	//
	// 爆発は「白く光る → オレンジ → くすんだ煙」のように色が変わる。
	// RGB は 1 を超えてよく、超えたぶんがブルームのしきい値を抜けて光って見える
	// ---------------------------------------------------------------
	float4 _color = lerp(g_draw.startColor, g_draw.endColor, _lifeT);

	// 出だしと終わりのフェード。
	// 割合(0〜1)で指定するので、粒ごとに寿命が違っても同じ見え方になる
	float _alpha = _color.a;
	if (g_draw.fadeInRatio > 0.0f)
	{
		_alpha *= saturate(_lifeT / g_draw.fadeInRatio);
	}
	if (g_draw.fadeOutRatio > 0.0f)
	{
		_alpha *= saturate((1.0f - _lifeT) / g_draw.fadeOutRatio);
	}

	_out.color = float4(_color.rgb, _alpha);

	return _out;
}
