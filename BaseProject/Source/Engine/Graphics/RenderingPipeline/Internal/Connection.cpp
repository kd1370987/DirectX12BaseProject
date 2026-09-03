#include "Connection.h"

#include "../../../Persistence/Archive/Archive.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	//
	// Connection
	//
	//======================================================================================
	void Connection::Archive(Persistence::Archive& a_arch)
	{
		a_arch.Field("linkID", linkID);
		a_arch.GUIDField("dstPassGUID", dstPassGUID);
		a_arch.Field("srcSlotID", srcSlotID);
		a_arch.Field("dstSlotID", dstSlotID);
	}

	void Connection::EditConnection(int a_srcOutPinID, int a_dstInPinID) const
	{
		ImNodes::Link(linkID, a_srcOutPinID, a_dstInPinID);
	}
}
