#pragma once

namespace Engine::Graphics
{
	class GraphicsEngine;
	class MeshBufferAllocator;

	namespace Pipeline
	{
		class PassMetaRegistry;
	}
}

namespace Engine::Graphics::D3D12
{
	class DescriptorHeapManager;
}

namespace DirectX
{
	class AudioEngine;
}

namespace Engine::Scene
{
	class SceneManager;
}

namespace Engine::Resource
{
	class ResourceManager;
	class AssetDatabase;

	/// <summary>
	/// リソースビルド時に必要なクラスの参照、コマンドリストなどを橋渡しする
	///
	/// ビルド処理(Mesh/Material/Textureの実体化)はシングルトンを直接引かず、
	/// このコンテキスト経由で必要なものを受け取ること。
	/// こうしておくと「誰のコマンドリストに積むのか」が呼び出し側の責任になり、
	/// モデル1体分をまとめて1回のsubmitに集約できる。
	/// </summary>
	struct ResourceBuildContext
	{
		// ---- デバイス ----
		Graphics::D3D12::Device* pDevice = nullptr;

		// ---- ビューの置き場 ----
		// 実体は GraphicsEngine が持っている。
		// ビュー(SRV/RTV/DSV/UAV)を取るものは、シングルトンではなくここから借りること
		Graphics::D3D12::DescriptorHeapManager* pHeapManager = nullptr;

		// ---- コマンドリスト ----
		// モデル1体につき1本ずつ確保され、ビルドが終わったところで呼び出し側がsubmitする
		Graphics::D3D12::GraphicsCommandList* pDirectCmdList = nullptr;
		Graphics::D3D12::GraphicsCommandList* pCopyCmdList = nullptr;		// アップロード転送用
		Graphics::D3D12::GraphicsCommandList* pComputeCmdList = nullptr;	// BLAS構築用

		// ---- 作成時に使うマネージャー ----
		// バッチの外で単発の転送を流すもの(テクスチャの読み込みなど)は、ここへ依頼する
		Graphics::GraphicsEngine* pGraphicsEngine = nullptr;
		Graphics::MeshBufferAllocator* pMeshBufferAllocator = nullptr;

		// レンダリングパイプラインを読むときに使う。
		// 保存されているのはパスの型IDだけなので、実体を作り直すのに一覧が要る
		Graphics::Pipeline::PassMetaRegistry* pPassMetaRegistry = nullptr;
		ResourceManager* pResourceManager = nullptr;
		AssetDatabase* pAssetDatabase = nullptr;

		// サウンド(SoundEffect)を作る先。持ち主は AudioManager
		DirectX::AudioEngine* pAudioEngine = nullptr;

		// プレハブがコンポーネントの型を引くワールド(今のシーンのもの)の持ち主
		Scene::SceneManager* pSceneManager = nullptr;

		// ---- GPUへの転送が終わるまで生かしておく中間バッファ ----
		// コマンドリスト(コピーとコンピュートの両方)の実行完了時にまとめて解放される。
		// BLAS をビルドするときのスクラッチもここへ預ける
		std::vector<ComPtr<ID3D12Resource>>* pKeepAliveUploads = nullptr;

		// ---- GPUの処理が終わったら呼ぶもの ----
		// 完了を見張るワーカースレッドから呼ばれる。BLAS の圧縮の依頼などに使う
		std::vector<std::function<void()>>* pOnBuildComplete = nullptr;

		// ---- 判定 ----
		bool CanRecordCopy() const { return pCopyCmdList != nullptr && pKeepAliveUploads != nullptr; }
		bool CanRecordCompute() const { return pComputeCmdList != nullptr; }

		/// <summary>
		/// 転送完了まで寿命を延ばしたいリソースを預ける
		/// </summary>
		void KeepAlive(const ComPtr<ID3D12Resource>& a_cpResource) const
		{
			if (!pKeepAliveUploads) return;
			pKeepAliveUploads->push_back(a_cpResource);
		}
	};

	/// <summary>
	/// バッチを持たない呼び出し元用 : マネージャーだけを載せたコンテキストを作る
	///
	/// これを受け取ったローダーは、GPUへ積む必要があれば自分でバッチを開く(ResourceBuildScope)。
	/// エディターから単発で読むときなど、手元にサービスしか無い場所で使う
	/// </summary>
	inline ResourceBuildContext MakeManagerOnlyContext(ResourceManager* a_pResourceManager, AssetDatabase* a_pAssetDatabase)
	{
		ResourceBuildContext _context = {};
		_context.pResourceManager = a_pResourceManager;
		_context.pAssetDatabase = a_pAssetDatabase;
		return _context;
	}
}
