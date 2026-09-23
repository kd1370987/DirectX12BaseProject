#pragma once
#include "DrawCommand.h"

namespace Engine::Graphics
{
	/// <summary>
	/// 描画要求と受付の配列を保持するクラス
	/// CPU側のデータなのでバックバッファ分保持はしない
	///
	/// 積むのは描画フェーズ(GraphicsEngine の Submit 系)、読むのは Execute の中
	/// (レンダーコンテキストとフレーム計算)。そのフレームのうちに使い切るので、
	/// GPUへ上げたコピーだけがフレーム数ぶんあればよい(レンダーコンテキスト側が持つ)。
	/// 中身は GraphicsEngine::EndFrame で Clear() される
	/// </summary>
	class DrawLists
	{
	public:

		// フレームの終わりに呼ぶ : 空にしつつ、次フレームぶんの容量を確保し直す
		void Clear();

		//--------------------------------------------------------------------------------------------
		// モデルの描画アイテム
		//--------------------------------------------------------------------------------------------
		void AddItem(const LightWeightDrawItem& a_item);

		// ソートキー順に並べる。GetPassItems() はこの後でしか引けない
		void SortItems();

		// 指定したパス番号のアイテムだけを返す(ソート済みであること)
		std::span<const LightWeightDrawItem> GetPassItems(uint8_t a_passIndex) const;

		//--------------------------------------------------------------------------------------------
		// メッシュシェーダー用 : 追加した位置(添字)を返す
		//--------------------------------------------------------------------------------------------
		UINT AddInstanceData(const MeshInstanceData& a_instanceData);
		UINT AddMeshMaterial(const MeshMaterial& a_meshMaterial);

		const std::vector<MeshInstanceData>& GetInstanceDataVec() const { return m_meshInstanceDataVec; }
		const std::vector<MeshMaterial>& GetMeshMaterialVec() const { return m_meshMaterialDataVec; }

		//--------------------------------------------------------------------------------------------
		// UI
		//--------------------------------------------------------------------------------------------
		void AddUI(const UIData& a_data);

		// レイヤー順に並べる。
		// UIパスは深度を持たないので、重なりを決めるのは描く順そのもの
		void SortUIByLayer();

		const std::vector<UIData>& GetUIDataVec() const { return m_uiDrawItemVec; }

		//--------------------------------------------------------------------------------------------
		// GPUスキニング
		//--------------------------------------------------------------------------------------------
		void AddSkinning(const SkinningDispatchItem& a_item);
		const std::vector<SkinningDispatchItem>& GetSkinningItems() const { return m_skinningDispathItemVec; }

		//--------------------------------------------------------------------------------------------
		// アニメーション用レイトレインスタンス作成命令
		//--------------------------------------------------------------------------------------------
		void AddDynamicRayRequest(const Raytracing::DynamicRaytracingRequest& a_request);
		const std::vector<Raytracing::DynamicRaytracingRequest>& GetDynamicRayRequests() const { return m_dynamicRayRequestVec; }

		//--------------------------------------------------------------------------------------------
		// ボーンパレット
		//
		// このワールドのボーン行列をパレットへ積み、GPU上の開始位置を返す。
		// 同じフレームで同じワールドを二度呼んでも積み直さず、最初に積んだ位置を返す
		//--------------------------------------------------------------------------------------------
		uint32_t AcquireBoneBaseIndex(ECS::World& a_world);
		const std::vector<Resource::BoneMatrix>& GetBoneMatrixVec() const { return m_boneMatrixVec; }

	private:

		// ソートキー持ち描画コマンドリスト
		std::vector<LightWeightDrawItem> m_lightWeightDrawItemVec = {};

		// オブジェクト単位データ
		std::vector<MeshInstanceData> m_meshInstanceDataVec = {};

		// サブセット単位データ
		std::vector<MeshMaterial> m_meshMaterialDataVec = {};

		// UI用アイテム配列
		std::vector<UIData> m_uiDrawItemVec = {};

		// GPUスキニング配列
		std::vector<SkinningDispatchItem> m_skinningDispathItemVec = {};

		// アニメーション用レイトレインスタンス作成命令
		std::vector<Raytracing::DynamicRaytracingRequest> m_dynamicRayRequestVec = {};

		//--------------------------------------------------------------------------------------------
		// ボーンパレット
		//--------------------------------------------------------------------------------------------
		// ボーン行列はシーン(ワールド)ごとのプールに入っていて、添字も 0 から振り直される。
		// ポーズ画面のようにシーンを重ねて描くときは複数のワールドを1フレームで描くので、
		// ここで全ワールド分を1本に連結し、各ワールドの土台(開始位置)を覚えておく。
		std::vector<Resource::BoneMatrix> m_boneMatrixVec = {};
		std::unordered_map<const ECS::World*, uint32_t> m_boneBaseIndexMap = {};
	};
}
