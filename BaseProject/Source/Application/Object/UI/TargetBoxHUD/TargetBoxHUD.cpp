#include "TargetBoxHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/ActiveCameraTag.h"
#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/EnemyTag.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

namespace App::Object
{
	namespace
	{
		// 既定のターゲットボックステクスチャのパス
		constexpr const char* TARGET_BOX_TEXTURE_PATH = "Asset/Texture/Reticle/Robot01.png";

		// 新規追加時の既定表示サイズ(px)
		constexpr float DEFAULT_BOX_SIZE = 96.0f;

		// 同時に描く最大数 : 敵が増えすぎたときに画面とバッファを守る
		constexpr size_t MAX_TARGET_COUNT = 128;
	}

	void TargetBoxHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// リソース周りはコンテキストが運んできたサービスを使う
		if (!a_context.pServices) return;
		if (!a_context.pServices->pAssetDatabase || !a_context.pServices->pResourceManager) return;

		// GUID未設定(新規追加)なら既定テクスチャを引く。
		// シーン読み込み時は Archive で復元済みのGUIDを尊重する。
		if (!m_texGUID.IsValid())
		{
			m_texGUID = a_context.pServices->pAssetDatabase->GetGUIDFromFilePath(TARGET_BOX_TEXTURE_PATH);
		}

		// 新規追加直後はサイズが0で何も見えないので、既定サイズを入れておく。
		// シーン読み込み時はこの後の Archive で保存値に上書きされる。
		if (m_pixelSize.x <= 0.0f || m_pixelSize.y <= 0.0f)
		{
			m_pixelSize = { DEFAULT_BOX_SIZE, DEFAULT_BOX_SIZE };
			m_editSize = m_pixelSize;
		}

		if (!m_texGUID.IsValid()) return;

		// 実体の到着は待たない。描画側が IsReady を見てスキップする
		m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
	}

	void TargetBoxHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// このフレームぶんを作り直す
		m_targetScreenPosVec.clear();

		// ワールドもサービスもコンテキスト経由で受け取る(シングルトンは触らない)
		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return;
		if (!a_context.pServices || !a_context.pServices->pOptionManager) return;

		// スクリーン解像度(px) : SubmitUI がピクセル座標を解釈する基準と合わせる
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return;

		//==================================================================
		// アクティブカメラから ViewProj を作る
		//------------------------------------------------------------------
		// ビュー行列はカメラのワールド行列の逆行列。
		// (GraphicsEngine::SetCameraMat と同じ作り方に合わせてある)
		//
		// GameObjectManager::Update は PostUpdate の後に回るので、
		// カメラも敵も同じフレームの確定済みワールド行列が読める。
		//==================================================================
		DXSM::Matrix _viewProjMat = DXSM::Matrix::Identity;
		bool _hasCamera = false;

		_pWorld->ForEach<const ActiveCameraTag, const CameraTag, const ProjMatComponent, const WorldMatrixComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveCameraTag* a_activeCamTagArray,
				const CameraTag* a_camTagArray,
				const ProjMatComponent* a_projMatArray,
				const WorldMatrixComponent* a_worldMatArray
			)
			{
				// アクティブカメラは1台の想定。先に見つかったものを使う
				if (_hasCamera || a_count == 0) return;

				const DXSM::Matrix _camWorldMat = a_worldMatArray[0].worldMat;
				const DXSM::Matrix _projMat = a_projMatArray[0].projMat;

				_viewProjMat = _camWorldMat.Invert() * _projMat;
				_hasCamera = true;
			}
		);

		// カメラがいないフレームは何も描かない
		if (!_hasCamera) return;

		//==================================================================
		// 敵をすべて集めてスクリーン座標へ射影する
		//==================================================================

		// 画面外判定の余白 : ボックスの半分ぶんはみ出すまでは描く
		const DXSM::Vector2 _halfSize = m_pixelSize * 0.5f;

		_pWorld->ForEach<const ActiveTag, const EnemyTag, const WorldMatrixComponent>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const EnemyTag* a_enemyTagArray,
				const WorldMatrixComponent* a_worldMatArray
			)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					if (m_targetScreenPosVec.size() >= MAX_TARGET_COUNT) return;

					// ワールド行列の平行移動成分がその敵の位置
					const DirectX::XMFLOAT4X4& _worldMat = a_worldMatArray[_i].worldMat;
					DXSM::Vector3 _worldPos = { _worldMat._41, _worldMat._42, _worldMat._43 };
					_worldPos.y += m_worldOffsetY;

					// ワールド → クリップ空間。
					// Vector4 で受ける版を使うと w が残るので、後の透視除算に使える
					DXSM::Vector4 _clipPos = {};
					DXSM::Vector3::Transform(_worldPos, _viewProjMat, _clipPos);

					// w <= 0 はカメラ後方。ここで割ると画面内へ折り返して映るので必ず弾く
					if (_clipPos.w <= 1e-4f) continue;

					const float _invW = 1.0f / _clipPos.w;
					const DXSM::Vector3 _ndc = { _clipPos.x * _invW, _clipPos.y * _invW, _clipPos.z * _invW };

					// ニア/ファーの外は描かない(DirectXの深度は0..1)
					if (_ndc.z < 0.0f || _ndc.z > 1.0f) continue;

					// NDC(中心原点/Y上向き) → ピクセル(左上原点/Y下向き)
					const DXSM::Vector2 _screenPos = {
						(_ndc.x * 0.5f + 0.5f) * _w,
						(0.5f - _ndc.y * 0.5f) * _h
					};

					// 完全に画面外へ出たものは積まない
					if (_screenPos.x < -_halfSize.x || _screenPos.x > _w + _halfSize.x) continue;
					if (_screenPos.y < -_halfSize.y || _screenPos.y > _h + _halfSize.y) continue;

					m_targetScreenPosVec.push_back(_screenPos);
				}
			}
		);
	}

	void TargetBoxHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_targetScreenPosVec.empty()) return;
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		// 見た目(サイズ・色・回転など)は全ボックス共通。位置だけ敵ごとに差し替える
		for (const DXSM::Vector2& _screenPos : m_targetScreenPosVec)
		{
			_pGE->SubmitUI(
				m_texRef,
				_screenPos,
				m_pixelSize,
				m_color,
				m_rotation,
				m_layer,
				m_uvOffset,
				m_pivot
			);
		}
	}

	void TargetBoxHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.Field("WorldOffsetY", m_worldOffsetY);
	}

	void TargetBoxHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("TargetBox");

		// 敵の原点から上下にずらす量(ワールド単位)
		ImGui::DragFloat("WorldOffsetY", &m_worldOffsetY, 0.01f);

		// 位置は敵から作るため PixelPos は効かない旨をここで明示しておく
		ImGui::TextDisabled("PixelPos is unused (follows enemies)");
		ImGui::Text("Targets : %d", static_cast<int>(m_targetScreenPosVec.size()));
	}
}
