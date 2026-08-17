#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/Helper/EditorHelper.h"	// DrawEnumCombo

// パーティクルの発生源(位置・方向)をどこから取るか
// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
enum class EEmitSpace : uint32_t
{
	WorldMatrix,		// 付いているオブジェクトの worldMat をそのまま使う(追従)
	LocalOffset,		// worldMat を基準に posOffset / emitDir を合成する(ノズル位置調整など)
	FixedWorld,			// コンポーネントの絶対 worldPos / emitDir を使う(行列を使わない単発など)
	ReverseVelocity,	// 進行方向(VelocityComponent)の逆へ吹く。位置は worldMat 基準 + posOffset。
						// 弾やミサイルのように「見た目の姿勢が進行方向と一致しない」ものの
						// 噴射・排気向け。速度が無いときは行列の +Z の逆を使う
};

struct ParticlesComponent
{
	// ---- 参照データ ----
	Engine::GUID particleGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> particlesAssetHandle;

	// ---- 発生源 ----
	EEmitSpace			emitSpace = EEmitSpace::WorldMatrix;
	Math::Vector3	posOffset = { 0,0,0 };		// LocalOffset時: worldMat基準の追加移動(ローカル座標)
	Math::Vector3	emitDir   = { 0,0,1 };		// Local/Fixed時の発生方向
	Math::Vector3	worldPos  = { 0,0,0 };		// FixedWorld時の絶対ワールド座標

	// ---- 発生量 ----
	int   emitCount = 8;		// 1回の発生数
	float emitRate  = 0.0f;		// >0: 毎秒 emitRate 回の連続発生 / 0: isPlay立ち上がりで1回だけバースト

	// 生成された時点から発生させるか(ParticleFixupSystem が isPlay に反映する)。
	// ミサイルの噴煙のように「出っぱなし」のものはこれを立てる。
	// ブースターのように状況で入り切りするものは false のままにして、
	// 制御側のシステム(ThrusterEffectSystem 等)に isPlay を任せる。
	bool  playOnStart = false;

	// ---- 形状(スケール/拡散) : アセットに持たせていないのでインスタンス側で持つ ----
	float baseScale      = 1.0f;	// 全体スケール
	float minScale       = 0.1f;	// 個々のスケール下限
	float maxScale       = 1.0f;	// 個々のスケール上限
	float positionRadius = 0.5f;	// 発生位置の半径(ばらつき)
	float directionAngle = 10.0f;	// 方向のばらつき(度)

	// ---- 火花(サブパーティクル) ----
	// isPlay の立ち上がり / 立ち下がりで一度だけ出す別アセット。
	// ブースターの点火・消火の「バチッ」とした表現用。
	// 発生源(emitSpace / posOffset / emitDir)は本体と共有し、
	// 同じフレームに本体と2つ同時に emit される。
	Engine::GUID sparkGUID;
	Engine::Handle<Engine::Resource::ParticlesAsset> sparkAssetHandle;

	bool  emitSparkOnStart = false;	// 発動時(isPlay false→true)に出す
	bool  emitSparkOnEnd   = false;	// 終了時(isPlay true→false)に出す
	int   sparkEmitCount   = 16;	// 1回の発生数

	// 火花は本体より大きく・広く散らしたいことが多いので、形状は別に持つ
	float sparkBaseScale      = 1.0f;
	float sparkMinScale       = 0.1f;
	float sparkMaxScale       = 1.0f;
	float sparkPositionRadius = 0.5f;
	float sparkDirectionAngle = 45.0f;

	// ---- ランタイム(保存しない) ----
	bool  isPlay = false;			// 発生させたいか(AttachmentDispatchSystem 等が設定)
	float time = 0.0f;				// レート発生の小数繰り越し用アキュムレータ
	int   pendingEmitCount = 0;		// このフレームの発生数(ParticleEmitSystemが計算 / EmitParticleSystemが消費)
	int   pendingSparkEmitCount = 0;// このフレームの火花発生数(同上)
	bool  wasPlaying = false;		// バーストの立ち上がり / 立ち下がり検出用
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
		a_ar.Field("playOnStart",    _comp.playOnStart);

		a_ar.Field("baseScale",      _comp.baseScale);
		a_ar.Field("minScale",       _comp.minScale);
		a_ar.Field("maxScale",       _comp.maxScale);
		a_ar.Field("positionRadius", _comp.positionRadius);
		a_ar.Field("directionAngle", _comp.directionAngle);

		// ※ 追加は末尾に。バイナリは順次読みなので途中に挿すと既存データが全部ずれる
		a_ar.Field("sparkGUID",            _comp.sparkGUID);
		a_ar.Field("emitSparkOnStart",     _comp.emitSparkOnStart);
		a_ar.Field("emitSparkOnEnd",       _comp.emitSparkOnEnd);
		a_ar.Field("sparkEmitCount",       _comp.sparkEmitCount);
		a_ar.Field("sparkBaseScale",       _comp.sparkBaseScale);
		a_ar.Field("sparkMinScale",        _comp.sparkMinScale);
		a_ar.Field("sparkMaxScale",        _comp.sparkMaxScale);
		a_ar.Field("sparkPositionRadius",  _comp.sparkPositionRadius);
		a_ar.Field("sparkDirectionAngle",  _comp.sparkDirectionAngle);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		ParticlesComponent& _comp = Engine::Editor::GetValue<ParticlesComponent>(a_context.pData);

		// ---- 発生源 ----
		ImGui::Text("Emit Source");
		Editor::EditorHelper::DrawEnumCombo("EmitSpace", _comp.emitSpace);
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
		else if (_comp.emitSpace == EEmitSpace::ReverseVelocity)
		{
			// 向きは速度から決まるので EmitDir は使わない
			ImGui::DragFloat3("PosOffset", &_comp.posOffset.x, 0.05f);
			ImGui::TextDisabled("Dir : -Velocity (fallback : -Forward)");
		}

		ImGui::Separator();

		// ---- 発生量 ----
		ImGui::Text("Emission");
		ImGui::DragInt("EmitCount", &_comp.emitCount, 1, 0);
		ImGui::DragFloat("EmitRate (/s, 0=Burst)", &_comp.emitRate, 0.5f, 0.0f);

		// 出っぱなしにするか。切り替えは即座に反映して、エディタで確認できるようにする
		// (生成時の反映は ParticleFixupSystem が行う)
		if (ImGui::Checkbox("PlayOnStart", &_comp.playOnStart))
		{
			_comp.isPlay = _comp.playOnStart;
		}

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
		// ロードではなくキャッシュ参照で解決したいので、選択だけを共通ヘルパーに任せる
		Editor::EditorHelper::DrawHandle(_comp.particlesAssetHandle);
		GUID _selectedGUID = {};
		if (Editor::EditorHelper::DrawAssetGUIDCombo(
			"Change Particle",
			"ParticlesAsset",
			_comp.particleGUID,
			_selectedGUID))
		{
			_comp.particlesAssetHandle = Resource::ResourceManager::Instance().GetCache<Resource::ParticlesAsset>(_selectedGUID);
			_comp.particleGUID = _selectedGUID;
		}

		ImGui::Separator();

		// ---- 火花(発動時 / 終了時のワンショット) ----
		// 本体と同じ発生源から、同じフレームに同時に出る
		if (ImGui::CollapsingHeader("Spark (Start / End)"))
		{
			ImGui::Checkbox("EmitSparkOnStart", &_comp.emitSparkOnStart);
			ImGui::Checkbox("EmitSparkOnEnd", &_comp.emitSparkOnEnd);

			ImGui::DragInt("SparkEmitCount", &_comp.sparkEmitCount, 1, 0);

			ImGui::DragFloat("SparkBaseScale", &_comp.sparkBaseScale, 0.05f, 0.0f);
			ImGui::DragFloat("SparkMinScale", &_comp.sparkMinScale, 0.01f, 0.0f);
			ImGui::DragFloat("SparkMaxScale", &_comp.sparkMaxScale, 0.01f, 0.0f);
			ImGui::DragFloat("SparkPositionRadius", &_comp.sparkPositionRadius, 0.05f, 0.0f);
			ImGui::DragFloat("SparkDirectionAngle (deg)", &_comp.sparkDirectionAngle, 0.5f, 0.0f);

			Editor::EditorHelper::DrawHandle(_comp.sparkAssetHandle);
			GUID _selectedSparkGUID = {};
			if (Editor::EditorHelper::DrawAssetGUIDCombo(
				"Change Spark Particle",
				"ParticlesAsset",
				_comp.sparkGUID,
				_selectedSparkGUID))
			{
				_comp.sparkAssetHandle = Resource::ResourceManager::Instance().GetCache<Resource::ParticlesAsset>(_selectedSparkGUID);
				_comp.sparkGUID = _selectedSparkGUID;
			}
		}

		// ---- ランタイム状態(参考) ----
		ImGui::Separator();
		ImGui::TextDisabled("isPlay:%d  pending:%d  spark:%d  time:%.2f",
			_comp.isPlay ? 1 : 0, _comp.pendingEmitCount, _comp.pendingSparkEmitCount, _comp.time);
	}
};
