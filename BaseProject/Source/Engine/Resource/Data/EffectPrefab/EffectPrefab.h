#pragma once
//==========================================================================================
//
// EffectPrefab
//
// 「炊いたら時間で必ず消える」大きな演出のためのエンティティテンプレート。
// 中身は Prefab と同じ(シグネチャ + コンポーネントの初期値 + 子)で、
// そこへ演出全体の寿命(lifeTime)を必ず1つ持たせたもの。
//
// ・使い分け
//     EffectPrefab … 地面から飛び出す砂柱・破片が飛び散る爆発のように、
//                    エンティティを何体も組み合わせて一度だけ炊き、終わったら消えるもの
//     EffectAsset  … ブースターの噴射のように、持ち主に付いて出し続ける(消えない)もの
//
// ・自滅は型で保証する。生成するとき(App::Utility::SpawnEffectPrefab)に全ノードへ
//   寿命を付けるので、プレハブに LifeTimeComponent を入れ忘れても、負(無期限)を
//   入れていても、lifeTime を過ぎて残るものは無い。
//   ノード側にこれより短い寿命が入っていれば、そちらが優先される。
// ・破片のように「出したものがさらに別のものを出す」ときも、出される側を
//   EffectPrefab にしておけば同じく自滅する。
// ・保存形式は Prefab と同じ Archive に lifeTime を足したもの(.ojefprfb / .obefprfb)。
//
//==========================================================================================

#include "../Prefab/Prefab.h"

namespace Engine::Resource
{
	class AssetDatabase;

	class EffectPrefab
	{
	public:
		// 寿命の下限(秒)。0 以下を許すと出た瞬間に消えて何も見えないので、ここで止める
		static constexpr float MIN_LIFE_TIME = 0.1f;

		EffectPrefab() = default;
		~EffectPrefab() = default;
		NON_COPYABLE_MOVABLE(EffectPrefab);

		//----------------------------------------------------------------------------------
		// リソースローダー / 生成(他リソースと同じ入口)
		//----------------------------------------------------------------------------------
		// ファイルパスから読み込んで実体を返す(ResourceManager の Load から使用)
		static EffectPrefab LoadFromFile(const std::string& a_path);
		// 空のエフェクトプレハブを新規作成(AssetDataBasePanel から使用)
		static void Create(AssetDatabase& a_assetDB, const std::string& a_path, const std::string& a_name);

		//----------------------------------------------------------------------------------
		// 保存 / 読み込み(コンポーネントのメタ情報が必要なので World を受け取る)
		//----------------------------------------------------------------------------------
		void Save(ECS::World* a_pWorld, const std::string& a_savePath);
		void Load(ECS::World* a_pWorld, const std::string& a_filePath);

		void Archive(Persistence::Archive& a_ar, ECS::World* a_pWorld);

		//----------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------
		// 中身のテンプレート(コンポーネントの編集・実体化の材料作りに使う)
		const Prefab& GetPrefab() const { return m_prefab; }
		Prefab& RefPrefab() { return m_prefab; }

		// 演出全体の寿命(秒)。これを過ぎて残るノードは無い
		float GetLifeTime() const { return m_lifeTime; }
		void SetLifeTime(float a_lifeTime) { m_lifeTime = (std::max)(a_lifeTime, MIN_LIFE_TIME); }

	private:
		Prefab m_prefab;			// エンティティテンプレート(ルート + 子)
		float m_lifeTime = 3.0f;	// 演出全体の寿命(秒)
	};
}
