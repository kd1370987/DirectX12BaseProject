#include "EffectEditor.h"

#include "../Editor.h"
#include "../Helper/EditorHelper.h"
#include "../EditorCamera/EditorCamera.h"

#include "../Panel/InspectorPanel/AssetInspector/ResourceDraw/EffectAssetEdit/EffectAssetEdit.h"
#include "../Panel/InspectorPanel/AssetInspector/ResourceDraw/ParticleEdit/ParticleEdit.h"

#include "../../MainEngine.h"
#include "../../ECS/World/World.h"
#include "../../Scene/BaseScene/BaseScene.h"
#include "../../Graphics/GraphicEngine.h"
#include "../../Graphics/RenderGraph/RenderGraph.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Option/OptionManager.h"
#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../Resource/Data/EffectAsset/EffectAsset.h"
#include "../../Resource/Data/Particles/ParticlesAsset.h"
#include "../../Audio/AudioManager.h"

#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Utility/EffectSpawnHelper.h"

namespace Engine::Editor
{
	namespace
	{
		// ポップアップのID。OpenPopup と BeginPopupModal で同じものを使う
		constexpr const char* POPUP_ID = "Effect Editor";

		// エフェクトを出す位置。プレビューは常に原点固定にしておく
		const Math::Vector3 EFFECT_ORIGIN = { 0.0f, 0.0f, 0.0f };

		// カメラの定位置。原点に出るエフェクトが正面に収まる位置
		const DXSM::Vector3 CAMERA_HOME_POS = { 0.0f, 2.5f, -8.0f };
		constexpr float CAMERA_HOME_YAW = 0.0f;
		constexpr float CAMERA_HOME_PITCH = 10.0f;
	}

	//======================================================================================
	// プレビュー中のエフェクトへの参照
	//
	// 実体はワールドの中にあり、生成は遅延(次の BeginFrame)なので、
	// 「まだ居ない」状態を素直に扱えるようにまとめてある。
	//======================================================================================
	struct EffectEditor::EffectRef
	{
		ECS::Entity entity = ECS::Limits::INVALID_ENTITY;
		EffectAssetComponent* pComp = nullptr;

		bool IsValid() const { return pComp != nullptr; }
	};

	EffectEditor::EffectEditor() = default;
	EffectEditor::~EffectEditor() = default;

	//======================================================================================
	// 開く / 閉じる
	//======================================================================================
	void EffectEditor::Open(const Engine::GUID& a_effectGUID)
	{
		if (a_effectGUID == Engine::DefaultGUID) return;

		// 開き直しでも中身は作り直す(別のエフェクトを選んだ場合があるため)
		DestroyEffectEntity();

		m_effectGUID = a_effectGUID;
		m_effectHandle =
			Resource::ResourceManager::Instance().LoadImmediate<Resource::EffectAsset>(a_effectGUID);

		m_isOpen = true;
		m_isOpenRequest = true;

		// 再生状態は開くたびに頭から
		m_isPlaying = true;
		m_isRestartRequest = false;
		m_selectedParticlePart = 0;

		EnsureWorld();

		// カメラは開くたびに定位置へ。
		// 前回どこかへ飛ばしたまま開くと、出したものが画面の外から始まってしまう
		if (m_upCamera) m_upCamera->SetPose(CAMERA_HOME_POS, CAMERA_HOME_YAW, CAMERA_HOME_PITCH);

		RequestSpawn();
	}

	void EffectEditor::Close()
	{
		if (!m_isOpen) return;

		DestroyEffectEntity();

		m_isOpen = false;
		m_effectGUID = Engine::DefaultGUID;
		m_effectHandle = {};

		// 押しっぱなし扱いを閉じたあとへ持ち越さない
		if (m_upCamera) m_upCamera->CancelControl();
	}

	void EffectEditor::Release()
	{
		// World::Release() は呼ばない。
		// あちらは「ECS参照カウントを全部リセット → GCで数え直し → 未参照を解放」まで走るので、
		// 数えられるのはこのワールドの中身だけになり、ゲームのシーンが使っている
		// モデルやテクスチャまで巻き添えで解放されてしまう。
		// ここが呼ばれるのはアプリ終了時(ResourceManager::Release のあと)なので、
		// 持ち物を捨てるだけでよい。
		m_upWorld.reset();
		m_upCamera.reset();
		m_isOpen = false;
	}

	//======================================================================================
	// プレビュー用ワールド
	//======================================================================================
	void EffectEditor::EnsureWorld()
	{
		if (!m_upCamera)
		{
			m_upCamera = std::make_unique<EditorCamera>();
			m_upCamera->Init();
		}

		if (m_upWorld) return;

		// ゲームのシーンとまったく同じ構成(コンポーネント・システム・ワールドリソース)で作る。
		// 描画のされ方を本番と揃えるのが目的なので、ここで簡易版を組んではいけない
		m_upWorld = Scene::CreateSceneWorld();
	}

	void EffectEditor::RequestSpawn()
	{
		if (!m_upWorld) return;
		if (m_effectGUID == Engine::DefaultGUID) return;

		// 実体化は次の BeginFrame。
		// 出し切っても消えないようにしておく(何度も再生し直したいので寿命はこちらが握る)。
		// 発生位置は常に原点。カメラは自由に動かせるので、見る位置と出す位置は分けておく
		App::Utility::SpawnEffectAt(*m_upWorld, m_effectGUID, EFFECT_ORIGIN, false);
	}

	void EffectEditor::DestroyEffectEntity()
	{
		if (!m_upWorld) return;

		// プレビュー用ワールドにはエフェクトしか居ないので、見つけたものを全部片付ける
		std::vector<ECS::Entity> _targets = {};
		m_upWorld->ForEach<EffectAssetComponent>(
			[&](ECS::ArchetypeChunk* a_pChunk, uint32_t a_count, EffectAssetComponent*)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					_targets.push_back(a_pChunk->entityData[_i]);
				}
			}
		);

		for (const ECS::Entity& _entity : _targets)
		{
			m_upWorld->AddReleaseEntity(_entity);
		}

		// 解放予約は次の BeginFrame で消化される。
		// 閉じたあとはこのワールドを回さないので、ここで1回だけ空回しして始末しておく
		if (!_targets.empty())
		{
			m_upWorld->BeginFrame();
		}
	}

	EffectEditor::EffectRef EffectEditor::FindEffect() const
	{
		EffectRef _ref = {};
		if (!m_upWorld) return _ref;

		m_upWorld->ForEach<EffectAssetComponent>(
			[&](ECS::ArchetypeChunk* a_pChunk, uint32_t a_count, EffectAssetComponent* a_effectArray)
			{
				if (_ref.pComp) return;		// 先に見つけたものを使う(プレビューは常に1つ)
				if (a_count == 0) return;

				_ref.entity = a_pChunk->entityData[0];
				_ref.pComp = &a_effectArray[0];
			}
		);

		return _ref;
	}

	Resource::EffectAsset* EffectEditor::RefEffectAsset() const
	{
		return Resource::ResourceManager::Instance().Ref(m_effectHandle);
	}

	Resource::ParticlesAsset* EffectEditor::RefSelectedParticleAsset() const
	{
		const auto* _pEffect = RefEffectAsset();
		if (!_pEffect) return nullptr;

		const auto& _parts = _pEffect->GetParticleParts();
		if (m_selectedParticlePart < 0) return nullptr;
		if (static_cast<size_t>(m_selectedParticlePart) >= _parts.size()) return nullptr;

		return Resource::ResourceManager::Instance().Ref(_parts[m_selectedParticlePart].particleHandle);
	}

	//======================================================================================
	// 更新 : ゲームのシーンの代わりに、このワールドだけを回す
	//======================================================================================
	void EffectEditor::UpdateScene(float a_dt)
	{
		if (!m_isOpen || !m_upWorld) return;

		// 止めているあいだは時間を進めない。
		// エフェクトの経過時間もパーティクルの発生もここの dt で決まるので、
		// 0 を流すだけで「その瞬間で固まる」
		const float _dt = m_isPlaying ? (a_dt * m_playSpeed) : 0.0f;

		// ---- 再生の指示をコンポーネントへ書いてから回す ----
		if (EffectRef _ref = FindEffect(); _ref.IsValid())
		{
			auto* _pEffect = Resource::ResourceManager::Instance().Ref(_ref.pComp->effectHandle);

			// 頭から再生し直す。
			// isPlay を落として立ち上げ直すと2フレームかかるので、実体を直接叩く
			if (m_isRestartRequest && _pEffect)
			{
				_pEffect->Play(_ref.pComp->instance);
				m_isRestartRequest = false;
			}

			_ref.pComp->isPlay = true;

			// 出し切ったら頭から。
			// 出しっぱなしのパーツを含むエフェクトは IsFinished が立たないので、
			// ループ指定でも何も起きない(それでよい : もともと終わらない演出のため)
			//
			// 音も見るのはゲーム側(EffectUpdateSystem)と揃えるため。
			// 見ないと、絵が終わった時点で頭出しされて音が毎回途中で切れる
			if (m_isLoop && _pEffect &&
				_pEffect->IsFinished(_ref.pComp->instance, &Audio::AudioManager::Instance()))
			{
				m_isRestartRequest = true;
			}
		}

		// ---- ゲームのシーンと同じ順でフェーズを回す ----
		// 当たり判定(CollisionWorld)はゲームのシーンと共用のグローバルなので、
		// 動的ワールドの構築はここでは行わない。プレビューにコライダーは居ないため
		// Physics 帯は空振りする
		m_upWorld->BeginFrame();

		m_upWorld->RunSystem(ECS::ESystemType::Input, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::PreUpdate, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::Update, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::Physics, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::Animation, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::Camera, _dt);
		m_upWorld->RunSystem(ECS::ESystemType::PostUpdate, _dt);
	}

	//======================================================================================
	// 描画 : ゲームのシーンの代わりに、このワールドの命令をレンダーグラフへ流す
	//======================================================================================
	void EffectEditor::DrawScene()
	{
		if (!m_isOpen || !m_upWorld) return;

		m_upWorld->RunSystem(ECS::ESystemType::PreDraw, 0.0f);
		m_upWorld->RunSystem(ECS::ESystemType::Draw, 0.0f);
		m_upWorld->RunSystem(ECS::ESystemType::PostDraw, 0.0f);

		// 大きさの目安になる格子
		if (m_isDrawGrid) DrawGrid();
	}

	void EffectEditor::DrawGrid() const
	{
		const DXSM::Color _lineColor(0.30f, 0.32f, 0.36f, 1.0f);
		const DXSM::Color _axisColor(0.55f, 0.58f, 0.65f, 1.0f);

		auto& _editor = MainEditor::Instance();

		// 格子はエフェクトの発生位置(原点)に敷く
		const float _half = m_gridSize;
		const int   _count = static_cast<int>(m_gridSize);	// 1mごと

		for (int _i = -_count; _i <= _count; ++_i)
		{
			const float _p = static_cast<float>(_i);
			const DXSM::Color& _col = (_i == 0) ? _axisColor : _lineColor;

			_editor.DrawLine(DXSM::Vector3(_p, 0.0f, -_half), DXSM::Vector3(_p, 0.0f, _half), _col);
			_editor.DrawLine(DXSM::Vector3(-_half, 0.0f, _p), DXSM::Vector3(_half, 0.0f, _p), _col);
		}
	}

	//======================================================================================
	// カメラ : シーンビューのフリーカメラと同じもの
	//======================================================================================
	void EffectEditor::UpdateCamera(float a_dt)
	{
		if (!m_isOpen || !m_upCamera) return;

		m_upCamera->Update(a_dt);
	}

	bool EffectEditor::TryGetCameraOverride(DXSM::Matrix& a_outWorldMat, DXSM::Matrix& a_outProjMat) const
	{
		if (!m_isOpen || !m_upCamera) return false;

		a_outWorldMat = m_upCamera->GetWorldMatrix();
		a_outProjMat = m_upCamera->GetProjMatrix();
		return true;
	}

	//======================================================================================
	// UI
	//======================================================================================
	void EffectEditor::OnDrawImGui()
	{
		if (!m_isOpen) return;

		// 開いた最初のフレームだけ ImGui へ知らせる。
		// Open() はインスペクターの描画中(ウィンドウの中)から呼ばれるので、
		// ID スタックが素の状態になるここまで待ってから開く
		if (m_isOpenRequest)
		{
			ImGui::OpenPopup(POPUP_ID);
			m_isOpenRequest = false;
		}

		// 画面の大部分を使う。モーダルなので後ろのパネルは触れない
		const ImVec2 _display = ImGui::GetIO().DisplaySize;
		ImGui::SetNextWindowSize(ImVec2(_display.x * 0.9f, _display.y * 0.9f), ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(ImVec2(_display.x * 0.5f, _display.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		bool _isWindowOpen = true;
		if (ImGui::BeginPopupModal(POPUP_ID, &_isWindowOpen, ImGuiWindowFlags_NoCollapse))
		{
			DrawToolbar();
			ImGui::Separator();

			// 左 : 見る / 右 : 組む
			const float _paneWidth = (std::min)(m_editPaneWidth, ImGui::GetContentRegionAvail().x * 0.6f);
			const float _viewWidth =
				ImGui::GetContentRegionAvail().x - _paneWidth - ImGui::GetStyle().ItemSpacing.x;

			if (ImGui::BeginChild("EffectEditorViewport", ImVec2((std::max)(64.0f, _viewWidth), 0.0f)))
			{
				DrawViewport();
				DrawInfo();
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("EffectEditorEditPane", ImVec2(_paneWidth, 0.0f), true))
			{
				DrawEditPane();
			}
			ImGui::EndChild();

			ImGui::EndPopup();
		}

		// × で閉じられた
		if (!_isWindowOpen)
		{
			ImGui::CloseCurrentPopup();
			Close();
		}
	}

	//--------------------------------------------------------------------------------------
	// 上段 : 再生と表示
	//--------------------------------------------------------------------------------------
	void EffectEditor::DrawToolbar()
	{
		const auto _fileName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(m_effectGUID);
		ImGui::Text("Effect : %s", _fileName.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("(%s)", m_effectGUID.String().c_str());

		// ---- 再生 ----
		if (ImGui::Button(m_isPlaying ? "Pause" : "Play", ImVec2(80.0f, 0.0f)))
		{
			m_isPlaying = !m_isPlaying;
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart", ImVec2(80.0f, 0.0f)))
		{
			m_isRestartRequest = true;
			m_isPlaying = true;
		}
		ImGui::SameLine();
		ImGui::Checkbox("Loop", &m_isLoop);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);
		ImGui::SliderFloat("Speed", &m_playSpeed, 0.05f, 3.0f, "x%.2f");

		// ---- 表示 ----
		ImGui::Checkbox("Grid", &m_isDrawGrid);
		if (m_isDrawGrid && !Option::OptionManager::GetInstance().GetDebugDrawOption().drawWire)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(Option の Draw Debug Wire が off のため出ません)");
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Camera"))
		{
			if (m_upCamera) m_upCamera->SetPose(CAMERA_HOME_POS, CAMERA_HOME_YAW, CAMERA_HOME_PITCH);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("右ドラッグ中のみ視点操作 / WASD・EQ移動 / Shift加速");

		ImGui::SameLine();
		if (EditorHelper::DeleteButton("Close"))
		{
			ImGui::CloseCurrentPopup();
			Close();
		}
	}

	//--------------------------------------------------------------------------------------
	// 左 : ゲームと同じレンダーグラフの出力
	//--------------------------------------------------------------------------------------
	void EffectEditor::DrawViewport()
	{
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) { ImGui::TextDisabled("GraphicsEngine がありません"); return; }

		auto* _pRG = _pGE->RefRenderGraph();
		if (!_pRG) { ImGui::TextDisabled("RenderGraph がありません"); return; }

		// シーンビューと同じ最終出力を出す。
		// ゲームのシーンと同じレンダーグラフを通っているので、ここで見えているものが本番の見え方
		const auto* _pTex = _pRG->GetTmepTexture("FinalColor");
		if (!_pTex) { ImGui::TextDisabled("出力テクスチャがまだありません"); return; }

		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _aspect = (_winOp.windowHeight > 0)
			? static_cast<float>(_winOp.windowWidth) / static_cast<float>(_winOp.windowHeight)
			: 16.0f / 9.0f;

		// 情報欄のぶんを残して、収まる大きさへ合わせる
		const ImVec2 _avail = ImGui::GetContentRegionAvail();
		const float _reserveY = ImGui::GetTextLineHeightWithSpacing() * 3.0f;
		const float _maxH = (std::max)(64.0f, _avail.y - _reserveY);

		ImVec2 _size((std::max)(64.0f, _avail.x), 0.0f);
		_size.y = _size.x / _aspect;
		if (_size.y > _maxH)
		{
			_size.y = _maxH;
			_size.x = _maxH * _aspect;
		}

		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
		ImGui::Image((ImTextureID)(_gpuHandle.ptr), _size);

		// フリーカメラへホバー状態を渡す。
		// 右クリックの開始位置がこの画像の上の時だけ操作を始めるための判定(シーンビューと同じ)
		if (m_upCamera) m_upCamera->SetViewportHovered(ImGui::IsItemHovered());
	}

	void EffectEditor::DrawInfo()
	{
		EffectRef _ref = FindEffect();
		if (!_ref.IsValid())
		{
			// 生成は遅延なので、開いた直後の1〜2フレームはここを通る
			ImGui::TextDisabled("エフェクトを生成中...");
			return;
		}

		const auto* _pEffect = Resource::ResourceManager::Instance().Get(_ref.pComp->effectHandle);
		if (!_pEffect)
		{
			ImGui::TextDisabled("アセットを読み込めませんでした");
			return;
		}

		ImGui::Text("Elapsed : %.2f s", _ref.pComp->instance.elapsed);
		ImGui::SameLine();
		ImGui::TextDisabled("| Particle Parts : %d / Mesh Parts : %d",
			static_cast<int>(_pEffect->GetParticleParts().size()),
			static_cast<int>(_pEffect->GetMeshParts().size()));
	}

	//--------------------------------------------------------------------------------------
	// 右 : 組む
	//
	// 中身はアセットインスペクターと同じ関数を呼ぶだけ。
	// ここで独自のUIを書くと、パーツにフィールドを足したときに片方だけ直し忘れる
	//--------------------------------------------------------------------------------------
	void EffectEditor::DrawEditPane()
	{
		auto* _pEffect = RefEffectAsset();
		if (!_pEffect)
		{
			ImGui::TextDisabled("エフェクトアセットを読み込めませんでした");
			return;
		}

		if (!ImGui::BeginTabBar("EffectEditorTabs")) return;

		if (ImGui::BeginTabItem("Effect"))
		{
			// 自分自身を開くボタンは要らないので出さない
			Inspector::EffectAssetEdit(m_effectGUID, _pEffect, false);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Particle"))
		{
			DrawParticleTab();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	//--------------------------------------------------------------------------------------
	// Particle タブ : パーツが使っている粒そのものを触る
	//
	// 「どこから・どれだけ出すか」は Effect タブ(パーツ)、
	// 「1粒がどう飛んでどう消えるか」はこちら(パーティクルアセット)。
	// 同じ粒を別のエフェクトも使っている場合、ここでの変更はそちらにも効く
	//--------------------------------------------------------------------------------------
	void EffectEditor::DrawParticleTab()
	{
		const auto* _pEffect = RefEffectAsset();
		if (!_pEffect) return;

		const auto& _parts = _pEffect->GetParticleParts();
		if (_parts.empty())
		{
			ImGui::TextDisabled("パーティクルパーツがありません");
			ImGui::TextDisabled("Effect タブの Add Particle Part から足してください");
			return;
		}

		// どのパーツの粒を触るか
		m_selectedParticlePart = std::clamp(m_selectedParticlePart, 0, static_cast<int>(_parts.size()) - 1);

		const std::string _preview = "Particle " + std::to_string(m_selectedParticlePart);
		if (ImGui::BeginCombo("Part", _preview.c_str()))
		{
			for (size_t _i = 0; _i < _parts.size(); ++_i)
			{
				const std::string _name =
					"Particle " + std::to_string(_i) + " : " +
					Resource::AssetDatabase::Instance().GetFileNameFromGUID(_parts[_i].particleGUID);

				const bool _isSelected = (static_cast<int>(_i) == m_selectedParticlePart);
				if (ImGui::Selectable(_name.c_str(), _isSelected))
				{
					m_selectedParticlePart = static_cast<int>(_i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();

		auto* _pParticles = RefSelectedParticleAsset();
		if (!_pParticles)
		{
			ImGui::TextDisabled("このパーツにはパーティクルが割り当てられていません");
			ImGui::TextDisabled("Effect タブでアセットを選ぶと、ここで中身を触れます");
			return;
		}

		ImGui::TextDisabled("この粒を使っている他のエフェクトにも変更が効きます");
		Inspector::ParticleEdit(_pParticles);
	}
}
