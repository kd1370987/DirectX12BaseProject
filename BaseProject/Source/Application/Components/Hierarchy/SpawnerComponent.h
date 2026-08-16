#pragma once

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
	Engine::GUID spawnerGUID = Engine::DefaultGUID;	// 生成元(GameObject)のGUID
	int          waveIndex   = -1;					// 生成元の中での区分(ウェーブ番号)
};

template<>
struct Engine::ECS::ComponentTraits<SpawnerComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SpawnerComponent& _comp = Engine::Editor::GetValue<SpawnerComponent>(a_pData);
		a_ar.GUIDField("spawnerGUID", _comp.spawnerGUID);
		a_ar.Field("waveIndex", _comp.waveIndex);
	}

	static void Edit(CompEditContext& a_context)
	{
		SpawnerComponent& _comp = Engine::Editor::GetValue<SpawnerComponent>(a_context.pData);

		// 生成時に書き込まれる値なので表示のみ
		ImGui::Text("SpawnerGUID : %s", _comp.spawnerGUID.String().c_str());
		ImGui::Text("WaveIndex   : %d", _comp.waveIndex);
	}
};
