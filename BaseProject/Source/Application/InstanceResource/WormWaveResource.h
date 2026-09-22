#pragma once

#include "../Components/Character/Boss/SwarmBossWave.h"

//==========================================================================================
//
// ワームの体を走る発光のウェーブ。
//
// 出すのは SwarmBossController(周期・速さ・見た目の調整値も持つ)。
// 受けて 4000 体のボイドへ書き込むのは BoidWaveSystem。
// 間にこのリソースを挟んでいるのは、オブジェクト側の更新で ECS を全走査すると
// チャンク単位で回れず、体数ぶんそのまま重くなるため。
//
// 位置はすべて「ワームの頭からの1次元距離(m)」。実際に体が曲がっていても、
// 伸ばした一本の紐の上での距離として扱う。
//   小隊長 … PlatoonLeaderComponent.distanceAlongWorm(生成時に決まる)
//   ボイド … 上に BoidComponent.distanceFromPlatoonLeader を足したもの(毎フレーム計算)
//
//==========================================================================================

struct WormWaveResource
{
	// 今フレームで走っているウェーブ(1本ぶんの中身は SwarmBossWave)。
	// 周期が通過時間より短ければ複数本が同時に並ぶ
	std::vector<SwarmBossWave> waves = {};

	//--------------------------------------------------------------------------
	// 見た目の調整値(SwarmBossController が毎フレーム書き写す)
	//--------------------------------------------------------------------------
	float width = 12.0f;	// 帯の幅(m)。ウェーブからこの距離でベース値に戻る

	float baseIntensity = 0.5f;		// ウェーブが来ていないときの発光の強さ
	float peakIntensity = 8.0f;		// ウェーブの中心での発光の強さ

	Math::Vector3 baseColor = { 1.0f, 0.3f, 0.1f };	// ベースの色(0〜1)
	Math::Vector3 peakColor = { 1.0f, 1.0f, 0.9f };	// ピークの色(0〜1)

	// ボスが居てウェーブを回しているか。
	// false の間は書き込む側(BoidWaveSystem)が何もしないので、
	// ボスが居ないシーンのボイドはプレハブの発光のままになる
	bool isActive = false;

	void Clear()
	{
		waves.clear();
		isActive = false;
	}

	// 事前確保(初期化時に一度だけ呼ぶ想定)
	void Reserve(size_t a_capacity)
	{
		waves.reserve(a_capacity);
	}
};
