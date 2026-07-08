#pragma once

namespace Engine::Raytracing
{
	/// <summary>
	/// メッシュのポリゴン情報を空間分割ツリーとして保持するリソース
	/// 静的も出るとスキニング等で毎フレーム変形する動的モデル用で振る舞いが変わる
	/// </summary>
	class BLAS
	{
	public:

		~BLAS() = default;
		NON_COPYABLE_MOVABLE(BLAS);

		// ===================================================================================
		// 静的BLASの構築
		// ===================================================================================

		/// <summary>
		/// 静的モデル用BLASの作成 : 更新不可
		/// ビルド完了後、構築用スクラッチバッファは破棄可能
		/// </summary>
		void CreateStatic(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
		);


		// ===================================================================================
		// 動的BLASの構築と更新
		// ===================================================================================

		/// <summary>
		/// 動的更新可能なインスタンスBLASとして作成
		/// スキニングキャラクター等の初期化時に呼び出す
		/// </summary>
		/// <param name="a_sourceBLAS">元となる静的モデルのBLAS</param>
		/// <param name="a_animatedGeometries">変形後の頂点バッファアドレスをセットしたGeometryDesc配列</param>
		void CreateDynamic(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_animatedGeometries
		);

		/// <summary>
		/// 動的BLASの頂点座標を更新
		/// 毎フレームの SkinningPass 完了後、TLAS構築の前に呼び出す
		/// 事前に ALLOW_UPDATE フラグ付きで作成(CloneAsDynamic等)されている必要がある
		/// </summary>
		void Update(D3D12::GraphicsCommandList* a_pCmdList);

		// ===================================================================================
		// ユーティリティ
		// ===================================================================================
		/// <summary>
		/// BLASのリソース解放 : 動的BLASの場合は更新用スクラッチも解放
		/// </summary>
		void Release();

		/// <summary>
		/// BLASの構築または更新完了を待機するUAVバリアを発行
		/// これを呼んでからTLASの構築を行わないと、GPU側で不完全なツリーを参照しクラッシュする
		/// </summary>
		void UAVBarrier(D3D12::GraphicsCommandList* a_pCmdList) const;

		/// <summary>
		/// デバッグ用途 ： PIXやNsightでリソースを識別しやすくする名前付け
		/// </summary>
		void SetName(LPCWSTR a_name);

		// ===================================================================================
		// アクセサ
		// ===================================================================================
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_cpResource ? m_cpResource->GetGPUVirtualAddress() : 0; }
		UINT GetSubsetCount() const { return static_cast<UINT>(m_geometryDescVec.size()); }
		const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GetGeometryDesc() const { return m_geometryDescVec; }
		bool IsDynamic() const { return m_isDynamic; }

	private:

		// 内部のビルド/アップデート共通処理
		bool BuildInternal(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags,
			bool a_isUpdate
		);

	private:
		// BLAS本体のリソース
		ComPtr<ID3D12Resource> m_cpResource = nullptr;

		// 動的BLASの場合のみ保持し続けるアップデート用スクラッチバッファ。
		// 静的BLAS作成時のスクラッチバッファは一時的なものなのでクラス内に保持せず関数内で一時確保
		ComPtr<ID3D12Resource> m_cpUpdateScratch = nullptr;

		// BLASを構成するジオメトリ（サブメッシュ）情報のキャッシュ
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescVec = {};

		// このBLASが動的(更新可能)として作成されたかのフラグ
		bool m_isDynamic = false;
	};
}