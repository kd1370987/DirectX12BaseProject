#pragma once

#include "../../../Engine/Editor/Helper/EditorHelper.h"
#include "../../../Engine/ECS/World/World.h"

//==========================================================================================
// FlyingSoundComponent
//
// 「飛んでいる間ずっと鳴っている音」を持つコンポーネント。ミサイルの飛翔音など。
// FlyingSoundSystem が、付いているエンティティの位置で3D再生し続ける。
//
// ・鳴らすボイス(SoundInstance)はこのコンポーネントではなく FlyingSoundResource が持つ。
//   自滅するエンティティが Release フェーズを通らなかった頃の作りで、
//   リソース側が「今フレーム見かけたか」を突き合わせて、消えたぶんを回収する。
//   (現在は自滅も ReleaseTag 経由なので、他のサウンドと同じ持ち方にもできる)
// ・3D再生なので、聞き手側に AudioListenerComponent が必要。
//==========================================================================================
struct FlyingSoundComponent
{
	Engine::GUID soundGUID = Engine::DefaultGUID;	// 鳴らすサウンド

	float vol = 1.0f;				// 音量
	float distanceScaler = 1.0f;	// 距離減衰の倍率。大きいほど遠くまで聞こえる
	bool  isLoop = true;			// 飛翔音なので基本ループ。単発の噴射音等で使うなら false
};

template<>
struct Engine::ECS::ComponentTraits<FlyingSoundComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		FlyingSoundComponent& _comp = Engine::Editor::GetValue<FlyingSoundComponent>(a_pData);
		a_ar.Field("SoundGUID", _comp.soundGUID);
		a_ar.Field("Vol", _comp.vol);
		a_ar.Field("DistanceScaler", _comp.distanceScaler);
		a_ar.Field("IsLoop", _comp.isLoop);
	}

	static void Edit(CompEditContext& a_context)
	{
		FlyingSoundComponent& _comp = Engine::Editor::GetValue<FlyingSoundComponent>(a_context.pData);

		// 鳴っているボイスはエンティティごとにリソース側が持っているので、
		// ここでの変更は「次に生成されたエンティティ」から効く
		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
			"Change Sound",
			"Sound",
			_comp.soundGUID);

		ImGui::DragFloat("Volume", &_comp.vol, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("DistanceScaler", &_comp.distanceScaler, 0.05f, 0.01f, 1000.0f);
		ImGui::Checkbox("Loop", &_comp.isLoop);
	}
};
