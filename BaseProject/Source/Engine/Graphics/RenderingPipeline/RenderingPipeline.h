#pragma once
namespace Engine::Graphics
{
	// リソースのタイプ
	enum class EPassSlotType : UINT
	{
		Texture,
		Buffer
	};

	// レンダーターゲットをクリアするかどうか
	enum class ELoadOp : UINT
	{
		Load,			// クリアしない
		Clear			// クリアする
	};

	// レンダーターゲットがこのパス以降にしようされるかどうか
	// これは外部から設定はしない。基本的にDontCareで次に使われるパスがつながった際に動的にStorになる
	// まだ使用するかは未定だが、エイリアシング用に置いているため未使用
	enum class EStoreOp : UINT
	{
		Store,			// この先で使うことがある
		DontCare		// これ以上後ろで使われることがない
	};

	// リソース、パスの入出力データ
	struct Slot
	{
		// リソースのステート
		std::string name;		// 名前
		EPassSlotType type;		// タイプ
		DXGI_FORMAT format;		// リソースフォーマット
		
		// リソースサイズ
		UINT64 width = 0;		// 横指定
		UINT height = 0;		// 縦指定
		float scale = 1.f;		// スケール指定 : 上記のサイズに対してかかる

		// リソースのパス間の設定
		ELoadOp loadOp = ELoadOp::Load;				// クリアは明示的に設定する必要がある
		EStoreOp storeOp = EStoreOp::DontCare;		// 後続が出れば自動で Store に変換

		// フレーム間でテンポラルするかどうか : パス間ではパスのつなぎ方で判別するためいらない
		bool isTemporal = false;

		// ランタイム時用
		Handle<Slot> handle = {};		// 自身に割り当てられたメモリ領域参照用
	};

	// グラフの構成要素 : 一つのシェーダーで一回のみの処理となる最小単位
	class Pass
	{
	public:



	private:

		std::vector<Slot> m_inputSlots = {};		// 入力
		std::vector<Slot> m_outputSlots = {};		// 出力
	};
}