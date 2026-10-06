#pragma once

namespace App::Component
{
	//==========================================================================================
	// SpawnerComponent
	//
	// 「誰に出されたエンティティか」を出された側に持たせる印。SceneSequence のウェーブ管理が使う。
	//
	// 生成元がエンティティIDのリストを持つ形にはしていない。プレハブの実体化は遅延コマンド
	// (次の BeginFrame で生成)なので出した直後はIDが取れず、撃破や解放でIDが無効になった
	// ときの後始末も生成元の仕事になってしまう。
	// 印を出される側に持たせておけば、生成元は毎フレーム「自分の印が付いた生存エンティティ」を
	// 数えるだけで全滅判定ができる。
	//==========================================================================================
	struct SpawnerComponent
	{
		Core::GUID spawnerGUID = Core::DEFAULT_GUID;	// 生成元(GameObject)のGUID
		int          waveIndex   = -1;					// 生成元の中での区分(ウェーブ番号)
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::SpawnerComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::SpawnerComponent& _comp = Engine::EditorField::RefValue<App::Component::SpawnerComponent>(a_pData);
		a_ar.GUIDField("spawnerGUID", _comp.spawnerGUID);
		a_ar.Field("waveIndex", _comp.waveIndex);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::SpawnerComponent& _comp = Engine::EditorField::RefValue<App::Component::SpawnerComponent>(a_context.pData);

		// 生成時に書き込まれる値なので表示のみ
		Engine::EditorField::Value("SpawnerGUID", "%s", _comp.spawnerGUID.String().c_str());
		Engine::EditorField::Value("WaveIndex", "%d", _comp.waveIndex);
	}
};
