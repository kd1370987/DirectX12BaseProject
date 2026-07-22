#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
#include "../../../Engine/Editor/Drawers/ComponentEdit/ComponentEdit.h"	// DrawEnumCombo

// パーティクルの発生源(位置・方向)をどこから取るか
enum class EEmitSpace : uint32_t
{
	WorldMatrix,	// 付いているオブジェクトの worldMat をそのまま使う(追従)
	LocalOffset,	// worldMat を基準に posOffset / emitDir を合成する(ノズル位置調整など)
	FixedWorld,		// コンポーネントの絶対 worldPos / emitDir を使う(行列を使わない単発など)
};

struct ParticlesComponent
{
	// ---- 参照データ ----
	Engine::GUID particleGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> particlesAssetHandle;

	// ---- 発生源 ----
	EEmitSpace			emitSpace = EEmitSpace::WorldMatrix;
	DirectX::XMFLOAT3	posOffset = { 0,0,0 };		// LocalOffset時: worldMat基準の追加移動(ローカル座標)
	DirectX::XMFLOAT3	emitDir   = { 0,0,1 };		// Local/Fixed時の発生方向
	DirectX::XMFLOAT3	worldPos  = { 0,0,0 };		// FixedWorld時の絶対ワールド座標

	// ---- 発生量 ----
	int   emitCount = 8;		// 1回の発生数
	float emitRate  = 0.0f;		// >0: 毎秒 emitRate 回の連続発生 / 0: isPlay立ち上がりで1回だけバースト

	// ---- 形状(スケール/拡散) : アセットに持たせていないのでインスタンス側で持つ ----
	float baseScale      = 1.0f;	// 全体スケール
	float minScale       = 0.1f;	// 個々のスケール下限
	float maxScale       = 1.0f;	// 個々のスケール上限
	float positionRadius = 0.5f;	// 発生位置の半径(ばらつき)
	float directionAngle = 10.0f;	// 方向のばらつき(度)

	// ---- ランタイム(保存しない) ----
	bool  isPlay = false;			// 発生させたいか(AttachmentDispatchSystem 等が設定)
	float time = 0.0f;				// レート発生の小数繰り越し用アキュムレータ
	int   pendingEmitCount = 0;		// このフレームの発生数(ParticleEmitSystemが計算 / EmittParticleSystemが消費)
	bool  wasPlaying = false;		// バーストの立ち上がり検出用
};


template<>
struct Engine::ECS::ComponentTraits<ParticlesComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_pData);
		a_ar.Field("particleGUID",   _comp.particleGUID);

		a_ar.Field("emitSpace",      _comp.emitSpace);
		a_ar.Field("posOffset",      _comp.posOffset);
		a_ar.Field("emitDir",        _comp.emitDir);
		a_ar.Field("worldPos",       _comp.worldPos);

		a_ar.Field("emitCount",      _comp.emitCount);
		a_ar.Field("emitRate",       _comp.emitRate);

		a_ar.Field("baseScale",      _comp.baseScale);
		a_ar.Field("minScale",       _comp.minScale);
		a_ar.Field("maxScale",       _comp.maxScale);
		a_ar.Field("positionRadius", _comp.positionRadius);
		a_ar.Field("directionAngle", _comp.directionAngle);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_context.pData);

		// ---- 発生源 ----
		ImGui::Text("Emit Source");
		Editor::DrawEnumCombo("EmitSpace", _comp.emitSpace);
		if (_comp.emitSpace == EEmitSpace::LocalOffset)
		{
			ImGui::DragFloat3("PosOffset", &_comp.posOffset.x, 0.05f);
			ImGui::DragFloat3("EmitDir (local)", &_comp.emitDir.x, 0.05f);
		}
		else if (_comp.emitSpace == EEmitSpace::FixedWorld)
		{
			ImGui::DragFloat3("WorldPos", &_comp.worldPos.x, 0.05f);
			ImGui::DragFloat3("EmitDir", &_comp.emitDir.x, 0.05f);
		}

		ImGui::Separator();

		// ---- 発生量 ----
		ImGui::Text("Emission");
		ImGui::DragInt("EmitCount", &_comp.emitCount, 1, 0);
		ImGui::DragFloat("EmitRate (/s, 0=Burst)", &_comp.emitRate, 0.5f, 0.0f);

		ImGui::Separator();

		// ---- 形状 ----
		ImGui::Text("Shape");
		ImGui::DragFloat("BaseScale", &_comp.baseScale, 0.05f, 0.0f);
		ImGui::DragFloat("MinScale", &_comp.minScale, 0.01f, 0.0f);
		ImGui::DragFloat("MaxScale", &_comp.maxScale, 0.01f, 0.0f);
		ImGui::DragFloat("PositionRadius", &_comp.positionRadius, 0.05f, 0.0f);
		ImGui::DragFloat("DirectionAngle (deg)", &_comp.directionAngle, 0.5f, 0.0f);

		ImGui::Separator();

		// ---- アセット選択(既存踏襲) ----
		Editor::Helper::DrawHandle(_comp.particlesAssetHandle);
		if (ImGui::BeginCombo("Change Particle", "Select..."))
		{
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("ParticlesAsset"))
			{
				bool _selected = (_comp.particleGUID == _prop.guid);
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					_comp.particlesAssetHandle = Resource::ResourceManager::Instance().GetCache<Resource::ParticlesAsset>(_prop.guid);
					_comp.particleGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}

		// ---- ランタイム状態(参考) ----
		ImGui::Separator();
		ImGui::TextDisabled("isPlay:%d  pending:%d  time:%.2f",
			_comp.isPlay ? 1 : 0, _comp.pendingEmitCount, _comp.time);
	}
};
