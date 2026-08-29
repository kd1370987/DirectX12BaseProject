#pragma once

namespace Engine::Graphics
{
	class MeshBufferAllocator;

	namespace Pipeline
	{
		class PassMetaRegistry;
	}
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
		D3D12::Device* pDevice = nullptr;

		// ---- コマンドリスト ----
		// モデル1体につき1本ずつ確保され、ビルドが終わったところで呼び出し側がsubmitする
		D3D12::GraphicsCommandList* pDirectCmdList = nullptr;
		D3D12::GraphicsCommandList* pCopyCmdList = nullptr;		// アップロード転送用
		D3D12::GraphicsCommandList* pComputeCmdList = nullptr;	// BLAS構築用

		// ---- 作成時に使うマネージャー ----
		Graphics::MeshBufferAllocator* pMeshBufferAllocator = nullptr;

		// レンダリングパイプラインを読むときに使う。
		// 保存されているのはパスの型IDだけなので、実体を作り直すのに一覧が要る
		Graphics::Pipeline::PassMetaRegistry* pPassMetaRegistry = nullptr;
		ResourceManager* pResourceManager = nullptr;
		AssetDatabase* pAssetDatabase = nullptr;

		// ---- GPUへの転送が終わるまで生かしておく中間バッファ ----
		// コマンドリストの実行完了時にまとめて解放される
		std::vector<ComPtr<ID3D12Resource>>* pKeepAliveUploads = nullptr;

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
}
