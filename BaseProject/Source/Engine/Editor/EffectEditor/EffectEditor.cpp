#include "EffectEditor.h"

#include "../Editor.h"
#include "../Helper/EditorHelper.h"

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

#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Utility/EffectSpawnHelper.h"

namespace Engine::Editor
{
	namespace
	{
		// ポップアップのID。OpenPopup と BeginPopupModal で同じものを使う
		constexpr const char* POPUP_ID = "Effect Editor";

		// 軌道カメラの操作感
		constexpr float ORBIT_SENSITIVITY = 0.4f;	// 度/ピクセル
		constexpr float PAN_SENSITIVITY = 0.01f;	// メートル/ピクセル(距離1あたり)
		constexpr float ZOOM_SENSITIVITY = 0.15f;	// ホイール1目盛りあたりの倍率
		constexpr float MIN_DISTANCE = 0.5f;
		constexpr float MAX_DISTANCE = 200.0f;
		constexpr float MAX_PITCH = 89.0f;

		// エフェクトを出す位置。プレビューは常に原点固定にしておく
		const Math::Vector3 EFFECT_ORIGIN = { 0.0f, 0.0f, 0.0f };
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
		m_isOpen = true;
		m_isOpenRequest = true;

		// 再生状態は開くたびに頭から
		m_isPlaying = true;
		m_isRestartRequest = false;

		// 注視点はエフェクトの発生位置(原点)へ戻す。
		// 前回パンしたまま開くと、出したものが画面の外から始まってしまう
		m_focusPos = { 0.0f, 0.0f, 0.0f };

		EnsureWorld();
		RequestSpawn();
	}

	void EffectEditor::Close()
	{
		if (!m_isOpen) return;

		DestroyEffectEntity();

		m_isOpen = false;
		m_effectGUID = Engine::DefaultGUID;
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
		m_isOpen = false;
	}

	//======================================================================================
	// プレビュー用ワールド
	//======================================================================================
	void EffectEditor::EnsureWorld()
	{
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
		// 発生位置は常に原点。注視点(m_focusPos)はカメラ側の都合で動くので、
		// そちらを発生位置に使うとパンしただけでエフェクトが移動したことになる
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
			if (m_isLoop && _pEffect && _pEffect->IsFinished(_ref.pComp->instance))
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

		// 格子はエフェクトの発生位置(原点)に敷く。注視点に付けると
		// パンしたときに一緒に動いてしまい、大きさの目安にならない
		const float _half = m_gridSize;
		const int   _count = static_cast<int>(m_gridSize);	// 1mごと

		for (int _i = -_count; _i <= _count; ++_i)
		{
			const float _p = static_cast<float>(_i);
			const bool _isAxis = (_i == 0);
			const DXSM::Color& _col = _isAxis ? _axisColor : _lineColor;

			_editor.DrawLine(
				DXSM::Vector3(_p, 0.0f, -_half),
				DXSM::Vector3(_p, 0.0f, _half), _col);
			_editor.DrawLine(
				DXSM::Vector3(-_half, 0.0f, _p),
				DXSM::Vector3(_half, 0.0f, _p), _col);
		}
	}

	//======================================================================================
	// カメラ
	//======================================================================================
	bool EffectEditor::TryGetCameraOverride(DXSM::Matrix& a_outWorldMat, DXSM::Matrix& a_outProjMat) const
	{
		if (!m_isOpen) return false;

		a_outWorldMat = m_camWorldMat;
		a_outProjMat = m_camProjMat;
		return true;
	}

	void EffectEditor::BuildCameraMatrix()
	{
		// 注視点から見た向き。左手系 +Z 前方なので、Yaw/Pitch から前方を作って
		// その逆向きへ distance だけ下がった位置にカメラを置く
		const DXSM::Quaternion _rot = DXSM::Quaternion::CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(m_yawDeg),
			DirectX::XMConvertToRadians(m_pitchDeg),
			0.0f);

		const DXSM::Vector3 _forward = DXSM::Vector3::Transform(DXSM::Vector3(0.0f, 0.0f, 1.0f), _rot);
		const DXSM::Vector3 _pos = m_focusPos - _forward * m_distance;

		m_camWorldMat =
			DXSM::Matrix::CreateFromQuaternion(_rot) *
			DXSM::Matrix::CreateTranslation(_pos);

		// アスペクトはゲームと同じ描画解像度から取る。
		// レンダーグラフを共有している以上、ここを変えると本番と画角がずれる
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _aspect = (_winOp.windowHeight > 0)
			? static_cast<float>(_winOp.windowWidth) / static_cast<float>(_winOp.windowHeight)
			: 16.0f / 9.0f;

		m_camProjMat = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_fovY), _aspect, 0.1f, 1000.0f);
	}

	//--------------------------------------------------------------------------------------
	// 画像の上でのカメラ操作
	//   左ドラッグ   : 回す
	//   中ドラッグ   : 注視点を平行移動
	//   ホイール     : 寄る / 引く
	//--------------------------------------------------------------------------------------
	void EffectEditor::UpdateCamera()
	{
		// 直前に描いた画像の上にカーソルがあるときだけ操作する
		const bool _isHovered = ImGui::IsItemHovered();

		if (_isHovered)
		{
			const float _wheel = ImGui::GetIO().MouseWheel;
			if (_wheel != 0.0f)
			{
				m_distance *= (1.0f - _wheel * ZOOM_SENSITIVITY);
				m_distance = std::clamp(m_distance, MIN_DISTANCE, MAX_DISTANCE);
			}
		}

		// ドラッグは画像の上で押し始めたときだけ始まる。
		// 一度始まれば枠の外へ出ても、離すまで続ける(シーンビューのフリーカメラと同じ)
		if (_isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))   m_isOrbiting = true;
		if (_isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) m_isPanning = true;

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))   m_isOrbiting = false;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) m_isPanning = false;

		const ImVec2 _delta = ImGui::GetIO().MouseDelta;

		if (m_isOrbiting)
		{
			m_yawDeg += _delta.x * ORBIT_SENSITIVITY;
			m_pitchDeg += _delta.y * ORBIT_SENSITIVITY;
			m_pitchDeg = std::clamp(m_pitchDeg, -MAX_PITCH, MAX_PITCH);
		}
		else if (m_isPanning)
		{
			// 画面の右方向・上方向へ、距離に比例した量だけ注視点を動かす
			const DXSM::Vector3 _right(m_camWorldMat._11, m_camWorldMat._12, m_camWorldMat._13);
			const DXSM::Vector3 _up(m_camWorldMat._21, m_camWorldMat._22, m_camWorldMat._23);

			const float _scale = PAN_SENSITIVITY * m_distance;
			m_focusPos -= _right * (_delta.x * _scale);
			m_focusPos += _up * (_delta.y * _scale);
		}

		BuildCameraMatrix();
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
		ImGui::SetNextWindowSize(ImVec2(_display.x * 0.8f, _display.y * 0.85f), ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(ImVec2(_display.x * 0.5f, _display.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		bool _isWindowOpen = true;
		if (ImGui::BeginPopupModal(POPUP_ID, &_isWindowOpen, ImGuiWindowFlags_NoCollapse))
		{
			DrawToolbar();
			ImGui::Separator();
			DrawViewport();
			DrawInfo();

			ImGui::EndPopup();
		}

		// × で閉じられた
		if (!_isWindowOpen)
		{
			ImGui::CloseCurrentPopup();
			Close();
		}
	}

	void EffectEditor::DrawToolbar()
	{
		// 何を見ているか
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
		ImGui::SetNextItemWidth(120.0f);
		ImGui::SliderFloat("FOV", &m_fovY, 20.0f, 100.0f, "%.0f deg");
		ImGui::SameLine();
		if (ImGui::Button("Reset Camera"))
		{
			m_yawDeg = 0.0f;
			m_pitchDeg = 12.0f;
			m_distance = 8.0f;
			m_focusPos = { 0.0f, 0.0f, 0.0f };
		}

		ImGui::SameLine();
		if (EditorHelper::DeleteButton("Close"))
		{
			ImGui::CloseCurrentPopup();
			Close();
		}
	}

	void EffectEditor::DrawViewport()
	{
		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) { ImGui::TextDisabled("GraphicsEngine がありません"); return; }

		auto* _pRG = _pGE->RefRenderGraph();
		if (!_pRG) { ImGui::TextDisabled("RenderGraph がありません"); return; }

		// シーンビューと同じ最終出力を出す。
		// ゲームのシーンと同じレンダーグラフを通っているので、ここで見えているものが本番の見え方
		const auto* _pTex = _pRG->GetTmepTexture("AfterTAAColor");
		if (!_pTex) { ImGui::TextDisabled("出力テクスチャがまだありません"); return; }

		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _aspect = (_winOp.windowHeight > 0)
			? static_cast<float>(_winOp.windowWidth) / static_cast<float>(_winOp.windowHeight)
			: 16.0f / 9.0f;

		// 情報欄のぶんを残して、収まる大きさへ合わせる
		const ImVec2 _avail = ImGui::GetContentRegionAvail();
		const float _reserveY = ImGui::GetTextLineHeightWithSpacing() * 3.0f;
		const float _maxH = (std::max)(64.0f, _avail.y - _reserveY);

		ImVec2 _size(_avail.x, _avail.x / _aspect);
		if (_size.y > _maxH)
		{
			_size.y = _maxH;
			_size.x = _maxH * _aspect;
		}

		auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
		ImGui::Image((ImTextureID)(_gpuHandle.ptr), _size);

		// 画像の上でのカメラ操作(ImGui::Image の直後に呼ぶこと。IsItemHovered が画像を指す)
		UpdateCamera();
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
		ImGui::TextDisabled("| Particle Parts : %d / Mesh Parts : %d | Camera %.0f deg, %.0f deg, %.1f m",
			static_cast<int>(_pEffect->GetParticleParts().size()),
			static_cast<int>(_pEffect->GetMeshParts().size()),
			m_yawDeg, m_pitchDeg, m_distance);
	}
}
