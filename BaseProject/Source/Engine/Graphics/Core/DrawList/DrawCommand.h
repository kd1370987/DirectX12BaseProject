#pragma once

namespace Engine::ECS
{
	class World;
}

namespace Engine::Graphics
{
	// 64ビットのソートキー
	//
	// ここに詰まっている ID は「並べ替えと、同じ状態をまとめるため」だけのもの。
	// 世代を持たないので、ここからリソースを引かないこと。
	// 実体が必要なときは LightWeightDrawItem のハンドルから取得する
	union RenderSortKey
	{
		uint64_t value;
		struct {
			// 下位ビットから順に判断優先度が低くなるように配置する
			//
			// 幅の配り方について:
			//   psoID は溢れると「描くときに別のPSOを引く」か「アイテムごと捨てる」しかなく、
			//   絵が消える。一方 meshID / materialID は並べ替えのための値でしかなく
			//   (ここからリソースは引かない)、被っても並び順が少し甘くなるだけ。
			//   なので psoID にはハンドルのインデックスと同じ16bitを渡し切って、
			//   切り捨てが起こりえない形にしてある。
			//   足りないぶんは、まだ誰も書いていない depth から回している
			uint64_t depth : 12;			// 深度 (未使用)
			uint64_t meshID : 14;			// メッシュ
			uint64_t materialID : 14;		// マテリアル
			uint64_t psoID : 16;			// PSOID : Handle::GetIndex() と同じ幅
			uint64_t passIndex : 8;			// パスインデックス
		} bits;
	};

	struct LightWeightDrawItem
	{
		// 描画順序と各種IDの情報すべてを持つ
		RenderSortKey sortKey;
		UINT subIndex = 0;

		// 描画に使うリソース。
		// 実体はリソースマネージャーに置いたままにして、ここではハンドルだけを持つ。
		// 描画する瞬間に引き直すこと
		Handle<Resource::Mesh>		meshHandle = {};
		Handle<Resource::Material>	materialHandle = {};

		// インスタンスデータ
		bool isAnimation = false;

		// メッシュシェーダー用インデックス
		UINT meshInstanceIndex = 0;
		UINT meshMaterialIndex = 0;

		// このサブセットを描画するためのメッシュレット数
		UINT subsetMeshletCount = 0;

		// ヘルパー関数 : ビット位置を直に書くと幅を変えたときに追従し損ねるので、
		// 取り出しはビットフィールド越しにする
		uint8_t GetPassIndex()		const { return static_cast<uint8_t>(sortKey.bits.passIndex); }
		uint16_t GetPSOID()			const { return static_cast<uint16_t>(sortKey.bits.psoID); }
	};

	/// <summary>
	/// GPUスキニングするエンティティの命令
	/// </summary>
	struct SkinningDispatchItem
	{
		RangeHandle<Resource::MeshVertexFloat> staticVertexHandle;		// アセット側の頂点データ
		RangeHandle<uint32_t> staticIndexHandle;						// アセット側のインデックスデータ
		RangeHandle<Resource::NodePoseMatrix> nodePoseMat;				// CPUで更新されたボーンノード行列
		RangeHandle<Resource::MeshVertexFloat> animatedHandle;
		RangeHandle<Resource::BoneMatrix> boneHandle;					// ボーン行列(ワールド内のプール添字)

		// ボーンパレット(GPU)での開始位置。
		// プールの添字はワールドごとに 0 から振り直されるので、
		// 複数のシーンを重ねて描くとそのままでは他シーンのボーンを踏む。
		// アップロード時に付く土台を足した「GPU上の」位置をここに持つ
		uint32_t boneBufferStart = 0;

		// 自身のBLASと変形後頂点を入れるメガバッファのハンドルを保持しているインスタンスのハンドル
		Handle<Raytracing::DynamicRaytracingData> animHandle;

		// この命令を出したワールド。
		// animHandle はワールドごとのプールの鍵なので、引くときは必ずこのワールドから引く。
		// シーンを重ねて描くと命令配列に複数のワールドのものが混ざるため、
		// 「今の一番上のシーン」から引くと他人のプールを鍵違いで探すことになる
		ECS::World* pWorld = nullptr;
	};
}