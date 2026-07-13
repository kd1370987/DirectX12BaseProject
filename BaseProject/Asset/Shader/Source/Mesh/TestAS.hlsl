#include "MeshCommon.hlsli"

[numthreads(1, 1, 1)]
void ASMain(in uint3 a_groupID : SV_GroupID)
{
	PayloadStruct _payload;
	_payload.myArbitaryData = a_groupID.x;
	DispatchMesh(1, 1, 1, _payload);
}
