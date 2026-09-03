#pragma once
//==========================================================================================
//
// Connection (Engine::Graphics::Pipeline)
//
// パスとパスをつなぐ線1本。
// 線を張るのも保存するのも RenderGraph とアセットの編集UIだけなので、
// この階層の外へは出さない
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	// 接続線 : 条件はなし
	// どちら側のピンにつないだかまで持たないと、パスが複数ピンを持ったときに線を引き直せない
	struct Connection
	{
		int linkID = 0;							// この接続線のID
		Engine::GUID dstPassGUID = {};			// 接続先パスのGUID
		uint32_t srcSlotID = 0;					// 出力側(線の根本)のスロットID
		uint32_t dstSlotID = 0;					// 入力側(線の先)のスロットID

		void Archive(Persistence::Archive& a_arch);						// 保存
		void EditConnection(int a_srcOutPinID, int a_dstInPinID) const;		// ImNodeへ線を登録する
	};
}
