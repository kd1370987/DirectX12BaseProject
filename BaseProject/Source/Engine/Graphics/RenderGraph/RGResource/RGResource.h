#pragma once

namespace Engine::Graphics
{
	// パスの巡回を解消するための倫理リソース
	struct LogicalResource
	{
		// 参照するリソース
		Resource::ID m_id = Resource::Limits::INVALID_ID;

		// リソース設定
		uint32_t m_version = 0;								// このリソースが書かれたときのバージョン
	};

	// レンダーグラフで使うリソースクラス
	struct RGResource
	{
		// リソース名
		std::string m_name = "RGResource";

		// 識別ID
		Engine::Resource::ID m_id = Engine::Resource::Limits::INVALID_ID;

		// リソースが参照するテクスチャ
		Engine::Resource::Handle<Engine::Resource::Texture> m_texHandle = {};

		// 実行中のステータス
		D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;
	};
}