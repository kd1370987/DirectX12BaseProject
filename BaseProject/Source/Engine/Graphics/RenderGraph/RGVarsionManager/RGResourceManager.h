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
		void Register(
			const std::string& a_name,
			const DXGI_FORMAT& format,
			const UINT64& a_widht,
			const UINT& a_height,
			const Resource::TextureUsage& a_texUsage,
			bool a_isTemporal,
			const DXSM::Color& a_clerColor = { 0,0,0,1 }

		);
		void RegisterTemporal(
			const std::string& a_name,
			const DXGI_FORMAT& format,
			const UINT64& a_widht,
			const UINT& a_height,
			const Resource::TextureUsage& a_texUsage,
			const DXSM::Color& a_clerColor = { 0,0,0,1 }
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

		// ---- テンポラル用 ----
		void Swap();		// 入れ替え : コンパイル時にはforの最後に入れる : ランタイム時にはフレームに一度のみ入れる


		// ---- アクセサ ----
		Resource::ID GetID(const std::string& a_name);
		Handle<Resource::Texture> GetTexHandle(Resource::ID a_id, bool isRead = false);

		Handle<D3D12::RTV> GetRTVHandle(Resource::ID a_id);		// RTVハンドル
		Handle<D3D12::DSV> GetDSVHandle(Resource::ID a_id);		// DSVハンドル
		Handle<D3D12::DSV> GetReadOnlyDSVHandle(Resource::ID a_id);	// リードオンリーDSVハンドル

		D3D12_RESOURCE_STATES& RefCurrentState(Resource::ID a_id, bool isRead = false);	// 現在のステート
		DXGI_FORMAT GetDXGIFormat(Resource::ID a_id);				// リソースのフォーマット

		std::vector<std::string> GetResourceNameVec();				// リソース名一覧

		const std::unordered_map<std::string, Resource::Index>& GetNameMap() const;
		const Resource::Texture* GetTex(Resource::ID a_id) const;

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
			DXSM::Color clerColor = { 0,0,0,1 };

			// 実行順を決定するためのバージョン
			Resource::Generation currentVarsion = 0;

			// テンポラルかどうか
			bool isTemporal = false;

			// コンパイル時に作成される
			// コンパイル時にバリアを作るためのステート
			D3D12_RESOURCE_STATES currentState[2] = { D3D12_RESOURCE_STATE_COMMON , D3D12_RESOURCE_STATE_COMMON };
			Handle<Resource::Texture> texHandle[2] = {};
		};

		// リソース参照
		const LogicalResource& GetRes(Resource::ID a_id) const;
		LogicalResource& RefRes(Resource::ID a_id);

		// テクスチャ参照
		const Resource::Texture* GetTex(const Handle<Resource::Texture>& a_handle) const;
		Resource::Texture* RefTex(const Handle<Resource::Texture>& a_handle);

	private:

		// リソース（テクスチャ）名、リソース情報
		std::unordered_map<std::string, Resource::Index> m_stringMap = {};
		std::vector<LogicalResource> m_resourceVec = {};

		// テンポラル用フラグ
		UINT m_temporalIndex = 0;
	};
};