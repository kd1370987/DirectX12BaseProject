#include "DrawList.h"

#include "Engine/ECS/World/World.h"

namespace Engine::Graphics
{
	namespace
	{
		// vector を空にしつつ、次フレームぶんの容量を確保し直す
		template<typename T>
		void ClearAndReserve(std::vector<T>& a_vec, size_t a_reserveCount)
		{
			a_vec.clear();
			a_vec.reserve(a_reserveCount);
		}
	}

	void DrawLists::Clear()
	{
		// 描画命令
		ClearAndReserve(m_lightWeightDrawItemVec, 10000);
		ClearAndReserve(m_uiDrawItemVec, 10000);
		ClearAndReserve(m_dynamicRayRequestVec, 1000);
		ClearAndReserve(m_skinningDispathItemVec, 1000);

		// ボーンパレットと、ワールドごとの土台の対応表
		ClearAndReserve(m_boneMatrixVec, 10000);
		m_boneBaseIndexMap.clear();

		// オブジェクトデータ
		ClearAndReserve(m_meshInstanceDataVec, 10000);

		// サブセット情報
		ClearAndReserve(m_meshMaterialDataVec, 10000);
	}

	//==========================================================================================
	// モデルの描画アイテム
	//==========================================================================================
	void DrawLists::AddItem(const LightWeightDrawItem& a_item)
	{
		m_lightWeightDrawItemVec.push_back(a_item);
	}

	void DrawLists::SortItems()
	{
		std::sort(
			m_lightWeightDrawItemVec.begin(), m_lightWeightDrawItemVec.end(),
			[](const LightWeightDrawItem& a, const LightWeightDrawItem& b)
			{
				return a.sortKey.value < b.sortKey.value;
			}
		);
	}

	std::span<const LightWeightDrawItem> DrawLists::GetPassItems(uint8_t a_passIndex) const
	{
		// 探したいパスのキーの最小値と最大値を求める。
		// パス番号は RenderSortKey の最上位8bit(56〜63)に置いてある
		constexpr uint32_t _kPassIndexShift = 56;
		uint64_t _minKey = static_cast<uint64_t>(a_passIndex) << _kPassIndexShift;
		uint64_t _maxKey = _minKey | ((1ull << _kPassIndexShift) - 1ull); // 下位56ビットをすべて1にする

		// ソート済み配列から開始位置を見つける
		auto _itStart = std::lower_bound(
			m_lightWeightDrawItemVec.begin(),
			m_lightWeightDrawItemVec.end(),
			_minKey,
			[](const LightWeightDrawItem& a_item, uint64_t a_value)
			{
				return a_item.sortKey.value < a_value;
			}
		);

		// ソート済み配列から終了位置を見つける
		auto _itEnd = std::upper_bound(
			_itStart,		// 開始位置から探す
			m_lightWeightDrawItemVec.end(),
			_maxKey,
			[](uint64_t a_value, const LightWeightDrawItem& a_item)
			{
				return a_value < a_item.sortKey.value;
			}
		);

		return std::span<const LightWeightDrawItem>(_itStart, _itEnd);
	}

	//==========================================================================================
	// メッシュシェーダー用
	//==========================================================================================
	UINT DrawLists::AddInstanceData(const MeshInstanceData& a_instanceData)
	{
		UINT _index = static_cast<UINT>(m_meshInstanceDataVec.size());
		m_meshInstanceDataVec.push_back(a_instanceData);
		return _index;
	}

	UINT DrawLists::AddMeshMaterial(const MeshMaterial& a_meshMaterial)
	{
		UINT _index = static_cast<UINT>(m_meshMaterialDataVec.size());
		m_meshMaterialDataVec.push_back(a_meshMaterial);
		return _index;
	}

	//==========================================================================================
	// UI
	//==========================================================================================
	void DrawLists::AddUI(const UIData& a_data)
	{
		m_uiDrawItemVec.push_back(a_data);
	}

	void DrawLists::SortUIByLayer()
	{
		// 小さいものから描く = 大きいほど手前。
		// 同じ値のものは積んだ順を崩さないよう stable_sort を使う
		// (1つのUIが持つ飾りは配列順で重なっているため、崩すと絵が入れ替わる)
		std::stable_sort(
			m_uiDrawItemVec.begin(), m_uiDrawItemVec.end(),
			[](const UIData& a, const UIData& b)
			{
				return a.layer < b.layer;
			}
		);
	}

	//==========================================================================================
	// GPUスキニング / レイトレ
	//==========================================================================================
	void DrawLists::AddSkinning(const SkinningDispatchItem& a_item)
	{
		m_skinningDispathItemVec.push_back(a_item);
	}

	void DrawLists::AddDynamicRayRequest(const Raytracing::DynamicRaytracingRequest& a_request)
	{
		m_dynamicRayRequestVec.push_back(a_request);
	}

	//======================================================================================
	// ボーンパレットへこのワールドのボーン行列を積む
	//--------------------------------------------------------------------------------------
	// ボーン行列はワールド(シーン)ごとの RangePool に入っていて、その添字も
	// ワールドごとに 0 から始まる。GPU側のボーンパレットは1本しかないので、
	// 複数のシーンを重ねて描くときは連結したうえで土台を足してやる必要がある。
	//
	// ポーズ画面はゲームのシーンへ重ねて出す(Push)ので、描くワールドが2つになる。
	// ここで両方を積んでおかないと、片方のキャラのボーンが単位行列でも他人のものでもない
	// 場所を指し、頂点が一点に潰れて消えたように見える。
	//
	// 呼ばれるのは描画フェーズ(GameManager::Draw)の中で、ボーン行列自体は
	// それより前の Animation フェーズで確定しているので、この時点の値で正しい。
	//======================================================================================
	uint32_t DrawLists::AcquireBoneBaseIndex(ECS::World& a_world)
	{
		// 同じワールドをこのフレームで既に積んでいればその位置を返す
		auto _it = m_boneBaseIndexMap.find(&a_world);
		if (_it != m_boneBaseIndexMap.end()) return _it->second;

		const uint32_t _baseIndex = static_cast<uint32_t>(m_boneMatrixVec.size());

		if (a_world.HasResource<Pool::RangePool<Resource::BoneMatrix>>())
		{
			auto& _boneMatPool = a_world.GetResource<Pool::RangePool<Resource::BoneMatrix>>();

			// プールは最初から10000要素ぶん確保されているので、丸ごと積むと
			// ワールドを2つ重ねただけでGPU側のボーンパレットが溢れる。
			// 実際に使われている末尾までで足りる(ハンドルの添字は必ずこの内側)
			const auto& _data = _boneMatPool.GetAllData();
			const size_t _usedCount = (std::min)(
				static_cast<size_t>(_boneMatPool.GetUsedCount()), _data.size());

			m_boneMatrixVec.insert(m_boneMatrixVec.end(), _data.begin(), _data.begin() + _usedCount);
		}

		m_boneBaseIndexMap.emplace(&a_world, _baseIndex);
		return _baseIndex;
	}
}
