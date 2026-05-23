#pragma once

namespace Engine::Resource
{
	//==========================================================
	// 
	// ノード：モデルを形成するメッシュを扱うための最小単位
	// 
	//==========================================================
	struct Node
	{
		// ノードのセーブ
		void Save(std::ofstream& a_ofs);

		std::string				name;						//ノード名
		UINT					nodeNameHash = 0;			// ノード名ハッシュ

		std::vector<int>		meshIndices;				// メッシュのインデックスリスト

		DirectX::XMFLOAT4X4		localTransform = {};				// 直属の親ボーンからの行列
		DirectX::XMFLOAT4X4		worldTransform = {};				// 原点からの行列
		DirectX::XMFLOAT4X4		boneInverseWorldMatrix = {};		// 原点からの逆行列

		int						parent = -1;				// 親インデックス
		std::vector<int>		children;					// 子供へのインデックスリスト

		int						boneIndex = -1;				// ボーンノードのとき、先頭から何番目のボーンか

		bool					isSkinMesh = false;			// スキンメッシュ持ちかどうか
	};
}