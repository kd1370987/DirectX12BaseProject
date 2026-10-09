#pragma once

#include "Engine/ECS/World/World.h"
#include "Application/Game/GlobalGameContext.h"

namespace App::InstanceResource
{
	//==========================================================================================
	//
	// シーンをまたぐ記録(GlobalGameContext)への入口を置くワールドリソース。
	//
	// 実体は GameManager が1つだけ持っている(シーンを切り替えても消えない)。
	// ここに置くのは借りたポインタだけで、ワールドを作るとき(GameManager が差し込む
	// ワールドの作り手)に詰められる。
	//
	// 以前は GameManager をシングルトンにして、使う側が名指しで引いていた。
	// 引く口をワールドに寄せたので、システムは SystemContext、
	// オブジェクトは ObjectContext の pWorld から辿れる。
	//
	// 書く側 : ScoreSystem(スコア) / SceneSequence(タイム・結末・ウェーブ)
	// 読む側 : ScoreHUD、ResultSequence
	//
	//==========================================================================================
	struct GameDataResource
	{
		Game::GlobalGameContext* pGameData = nullptr;

		/// <summary>
		/// ワールドから記録を引く。置かれていなければ nullptr
		/// </summary>
		/// <remarks>
		/// エディターの確認用ワールドなど、作り手を通らずに作られたワールドには無い
		/// </remarks>
		static Game::GlobalGameContext* Find(Engine::ECS::World* a_pWorld)
		{
			if (!a_pWorld || !a_pWorld->HasResource<GameDataResource>()) return nullptr;
			return a_pWorld->RefResource<GameDataResource>().pGameData;
		}
	};
}
