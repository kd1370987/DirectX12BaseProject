#pragma once

struct ProjMatComponent
{
	DirectX::XMFLOAT4X4 projMat = {};     // 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};  // 射影逆行列
};