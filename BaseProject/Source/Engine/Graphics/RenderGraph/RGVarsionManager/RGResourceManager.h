#pragma once

namespace Engine::Graphics
{
	// レンダーグラフで使うリソースの抽象化IDを管理する
	class RGResourceManager
	{
	public:

		// 実態を登録
		void Register(
			const std::string& a_name,
			const DXGI_FORMAT& format,
			const UINT64& a_widht,
			const UINT& a_height,
			const Resource::TextureUsage& a_texUsage,
			const DXSM::Color& a_clerColor = {0,0,0,1}
		);

		// 既存の最新バージョンを取得する
		// 読み込み用 : テクスチャ名、読み込み時の使用方法
		Resource::ID Read(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage);

		// バージョンをインクリメントして新しいハンドルとして返す
		// 書き込み用 : テクスチャ名、書き込み時の使用方法
		Resource::ID Write(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage);

		// 今まで登録された情報でテクスチャを作成
		void CreateAllTexture();

		// 実行に移るためステートをテクスチャに合わせる
		void StateReset();

		// アクセサ
		Resource::ID GetID(const std::string& a_name);
		Resource::Handle<Resource::Texture> GetTexHandle(Resource::ID a_id);

		Resource::Handle<D3D12::RTV> GetRTVHandle(Resource::ID a_id);		// RTVハンドル
		Resource::Handle<D3D12::DSV> GetDSVHandle(Resource::ID a_id);		// DSVハンドル

		D3D12_RESOURCE_STATES& RefCurrentState(Resource::ID a_id);	// 現在のステート
		DXGI_FORMAT GetDXGIFormat(Resource::ID a_id);				// リソースのフォーマット

		std::vector<std::string> GetResourceNameVec();				// リソース名一覧

	private:

		// 論理リソース
		struct LogicalResource
		{
			// リソーステクスチャの作成情報
			std::string name = "none";
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			uint32_t widht = 0;
			uint32_t height = 0;
			Resource::TextureUsage usage = Resource::TextureUsage::None;
			DXSM::Color clerColor = {0,0,0,1};

			// 実行順を決定するためのバージョン
			Resource::Generation currentVarsion = 0;

			// コンパイル時にバリアを作るためのステート
			D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

			// コンパイル時に作成される
			Resource::Handle<Resource::Texture> texHandle = {};
		};


		// リソース（テクスチャ）名、リソース情報
		std::unordered_map<std::string, Resource::Index> m_stringMap = {};
		std::vector<LogicalResource> m_resourceVec = {};
	};
};