#pragma once
/// <summary>
/// ボイドスポーンコンポーネント
/// 生成数は外部で決める
/// </summary>
struct BoidSpownerComopnent
{
	Engine::GUID boidPrefabGUID = {};					// 出すボイド
	Engine::Handle<Engine::Resource::Prefab> prefab;	// ランタイム用(初回生成時に解決)
	float spawnRadius = 5.0f;							// 自分を中心にこの半径の球内へばらまく
};