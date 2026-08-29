#pragma once

#include "../../Components/Camera/ProjMatComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"
#include "Engine/Editor/Helper/EditorHelper.inl"

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	// 実行時の画角の上乗せ(度)。実際の画角は fovY + fovBoost。
	// スピードに応じた広角化(TPSSystem)のように毎フレーム動かす値をここに入れる。
	// fovY を直接書き換えると「作者が決めた基準の画角」が失われるので分けてある。
	// 派生値なので保存しない。
	float fovBoost		= 0.0f;

	//======================================================================================
	// このカメラで画面を映すかどうか
	//--------------------------------------------------------------------------------------
	// もとは ActiveCameraTag という空のタグで表していたが、
	//   ・タグの付け外しはシグネチャの張り替え(エンティティの引っ越し)になるので、
	//     カメラを切り替えるだけで1フレーム遅れる
	//   ・エディターでは「付いている/付いていない」でしか見えず、
	//     カメラの設定を見ているつもりでも切り替えは別の場所を触ることになる
	// ので、カメラ自身の持ち物にした。
	//
	// 実際にどれが映すかは MainCameraSystem が毎フレーム見て、
	// SingletonEntityResource.mainCamera へ書き込む。
	// 立っているカメラが複数あったときは先に見つかったものが勝つので、
	// 切り替えるときは前のカメラを false にすること。
	//======================================================================================
	bool isActive = true;

	//======================================================================================
	// このカメラが使う描画構成(グラフィックスパイプライン)
	//--------------------------------------------------------------------------------------
	// カメラは「何を見るか」だけを持ち、「どう描くか」はこのアセットが持つ。
	// メインカメラでもサブカメラ(モニターに映すものなど)でも同じ形で指せる。
	//
	// 空のままなら新しい描画経路には乗らない(従来のレンダーグラフだけが動く)。
	// 実行用のインスタンスはカメラごとに GraphicsEngine 側が作る
	//======================================================================================
	Engine::GUID pipelineGUID = {};
	Engine::Handle<Engine::Graphics::Pipeline::RenderingPipelineAsset> pipelineHandle = {};

	//======================================================================================
	// このカメラの描画サイズ
	//--------------------------------------------------------------------------------------
	// 0 のままなら画面の描画解像度に追従する。
	// モニターに映すカメラのように小さく描きたいときだけ指定する
	//======================================================================================
	UINT viewportWidth = 0;
	UINT viewportHeight = 0;

	// 描画する順番。小さいものから回す。
	// モニターへ映す絵を先に作ってから本編を描く、といった並べ替えに使う
	int renderOrder = 0;

	// 画角などが変わったので射影行列を作り直す必要がある
	bool isDirty = false;

	// 実効画角(度)。0 や 180 以上は射影行列が破綻するので必ず内側へ丸める
	float GetFovY() const { return std::clamp(fovY + fovBoost, 1.0f, 179.0f); }
};

template<>
struct Engine::ECS::ComponentTraits<CameraParamComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_pData);
		a_ar.Field("fovY",_comp.fovY);
		a_ar.Field("aspectRatio",_comp.aspectRatio);
		a_ar.Field("nearZ",_comp.nearZ);
		a_ar.Field("farZ",_comp.farZ);

		// 既存のシーン/プレハブにはこのキーが無い。
		// その場合は既定値(true)のまま残るので、今までどおり映るカメラとして動く
		a_ar.Field("isActive",_comp.isActive);

		// 以下は後から足したもの。古いシーン/プレハブには入っていないので、
		// 見つからなければ既定値のまま(パイプライン無し = 従来経路)になる
		a_ar.Field("pipelineGUID",_comp.pipelineGUID);
		a_ar.Field("viewportWidth",_comp.viewportWidth);
		a_ar.Field("viewportHeight",_comp.viewportHeight);
		a_ar.Field("renderOrder",_comp.renderOrder);
	}

	//----------------------------------------------------------------------------------
	// 借りているリソースを返す
	//
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事
	//----------------------------------------------------------------------------------
	static void Release(void* a_pData)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_pData);
		Engine::Resource::ResourceManager::Instance().ReleaseHandle(_comp.pipelineHandle);
	}

	static void Edit(CompEditContext& a_context)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_context.pData);

		bool _isEdit = false;

		// 映すかどうかの切り替え。射影行列には関係しないので _isEdit には混ぜない
		ImGui::Checkbox("IsActive", &_comp.isActive);

		// 描画構成 : 空なら従来のレンダーグラフだけが動く
		Engine::Editor::EditorHelper::DrawAssetSelectCombo<Engine::Graphics::Pipeline::RenderingPipelineAsset>(
			"Pipeline",
			"RenderingPipelineAsset",
			_comp.pipelineGUID,
			_comp.pipelineHandle
		);

		// 描画サイズ : 0 なら画面の描画解像度に追従する
		int _viewport[2] = { static_cast<int>(_comp.viewportWidth), static_cast<int>(_comp.viewportHeight) };
		if (ImGui::DragInt2("Viewport(0=Auto)", _viewport, 1.0f, 0, 8192))
		{
			_comp.viewportWidth = static_cast<UINT>(std::max(0, _viewport[0]));
			_comp.viewportHeight = static_cast<UINT>(std::max(0, _viewport[1]));
		}
		ImGui::DragInt("RenderOrder", &_comp.renderOrder);
		ImGui::Separator();

		_isEdit |= ImGui::DragFloat("Fov", &_comp.fovY);
		_isEdit |= ImGui::DragFloat("Aspect", &_comp.aspectRatio);
		_isEdit |= ImGui::DragFloat("NearZ", &_comp.nearZ, 0.01f, 0.1f);
		_isEdit |= ImGui::DragFloat("FarZ", &_comp.farZ,1.0f,1.0f);

		_comp.nearZ = std::max(0.1f, _comp.nearZ);
		_comp.farZ = std::max(1.0f,_comp.farZ);

		// プレハブのインスペクタは実体を持たない(entity が無効値)。
		// RefData は生存エンティティ前提で添え字を引くので、先に弾く
		if (!a_context.pWorld || a_context.entity == Engine::ECS::Limits::INVALID_ENTITY) return;

		auto* _pProjMatComp = a_context.pWorld->RefData<ProjMatComponent>(a_context.entity);
		if (!_pProjMatComp) return;

		if (_isEdit)
		{
			_pProjMatComp->projMat = Math::Matrix::CreatePerspectiveFieldOfView(
				DirectX::XMConvertToRadians(_comp.GetFovY()),
				_comp.aspectRatio,
				_comp.nearZ,
				_comp.farZ
			);
		};
	}
};