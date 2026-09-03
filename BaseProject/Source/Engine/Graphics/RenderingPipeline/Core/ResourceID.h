#pragma once
//==========================================================================================
//
// ResourceID / ResourceHandle (Engine::Graphics::Pipeline)
//
// 仮想リソースの「同一性」と「参照」。
//   ResourceID     : どのパスのどの出力ピンが作ったリソースか(同一性)
//   ResourceHandle : RenderGraph が持つ仮想リソース配列の添字(参照)
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	// 仮想リソースの参照
	//
	// 中身は RenderGraph が持つ仮想リソース配列の添字だけ。
	// 実行順はピンの接続から決まるので、既存の RGResourceHandle のような
	// 世代(version)は持たせていない
	struct ResourceHandle
	{
		static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);

		uint32_t index = INVALID_INDEX;

		bool IsValid() const { return index != INVALID_INDEX; }

		bool operator==(const ResourceHandle& a_other) const { return index == a_other.index; }
		bool operator!=(const ResourceHandle& a_other) const { return !(*this == a_other); }
	};

	//======================================================================================
	// 仮想リソースの識別子
	//
	// 「どのパスのどの出力ピンが作ったリソースか」。
	//
	// 以前はリソース名(文字列)が同一性そのもので、「同じ名前 = 同じリソース」だった。
	// そのため同じパスクラスを2つ置くと出力名がぶつかり、黙って1枚のテクスチャへ
	// 相乗りしていた(ブラーの縮小段を並べるのに、エディターで名前を打ち分ける必要があった)。
	//
	// 作り手のインスタンスと出力ピンで決めれば、何個並べても勝手に別物になる。
	// パスの型ID(ID<Pass>)ではなくインスタンスのGUIDなのはそのため。
	// 型IDは同じクラスを2つ置くと被るので、インスタンスの識別には使えない。
	//
	// 名前は表示用のラベルとして残っているだけで、同一性には関わらない。
	// 例外はグラフの外から差し込まれるリソースで、
	// 差し込む側とパス側が名前で待ち合わせるしかないため、そこだけ名前から起こす
	//======================================================================================
	struct ResourceID
	{
		static constexpr uint32_t INVALID_SLOT_ID = static_cast<uint32_t>(-1);

		Engine::GUID passGUID = {};					// 作ったパス : 外部リソースなら空のまま
		uint32_t slotID = INVALID_SLOT_ID;			// その出力スロットID : 外部リソースなら名前のハッシュ

		bool IsValid() const { return slotID != INVALID_SLOT_ID; }

		bool operator==(const ResourceID& a_other) const
		{
			return slotID == a_other.slotID && passGUID == a_other.passGUID;
		}
		bool operator!=(const ResourceID& a_other) const { return !(*this == a_other); }

		// パスの出力ピンから起こす : 同じクラスを何個置いても別物になる
		static ResourceID FromOutputSlot(const Engine::GUID& a_passGUID, uint32_t a_slotID);

		// グラフの外から差し込まれるリソースから起こす。
		// 作り手のパスが居ないので GUID は空のまま。
		// パスの出力は必ず有効なGUIDを持つので、こちらとぶつかることはない
		static ResourceID FromImportName(const std::string& a_name);
	};
}

namespace std
{
	// 仮想リソースの対応表の鍵に使う
	template<>
	struct hash<Engine::Graphics::Pipeline::ResourceID>
	{
		size_t operator()(const Engine::Graphics::Pipeline::ResourceID& a_id) const noexcept
		{
			const size_t _hash = a_id.passGUID.Hash();
			return _hash ^ (static_cast<size_t>(a_id.slotID)
				+ 0x9e3779b97f4a7c15ull + (_hash << 6) + (_hash >> 2));
		}
	};
}
