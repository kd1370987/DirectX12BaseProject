#pragma once
namespace Engine::Resource
{
	class ShadingModelTable
	{
	public:
		ShadingModelTable() = default;
		ShadingModelTable(const std::string& a_name) : m_typeName(a_name) {}
		~ShadingModelTable() = default;
		NON_COPYABLE_MOVABLE(ShadingModelTable);

		// ---- アクセサ ----
		std::span<const ResourceRef<Shader>> GetShaderHandles(UINT a_passHash) const;

		/// <summary>
		/// シリアライズ処理
		/// </summary>
		/// <param name="a_ar">保存・読み込み</param>
		void Archive(Persistence::Archive& a_ar);

		std::string GetName() const { return m_typeName; }

		// シェーダーの有無に関わらず、有効なパスのハッシュを返す
		std::vector<UINT> GetPassHashes() const;

		// ---- パス編集用アクセサ ----
		// シリアライズ用データとランタイム用データを常に同期させるため、
		// エディターからはこの関数を経由して編集する

		// このパスが有効化されているか
		bool HasPass(const std::string& a_passName) const { return m_shaderGUIDMap.contains(a_passName); }

		// パスに登録されているシェーダーGUID配列を取得 : 無効なパスなら空配列
		const std::vector<Engine::GUID>& GetShaderGUIDs(const std::string& a_passName) const;

		// パスの有効・無効切り替え
		void EnablePass(const std::string& a_passName);
		void DisablePass(const std::string& a_passName);

		// パスへのシェーダー追加・削除
		void AddShader(const std::string& a_passName, const Engine::GUID& a_shaderGUID);
		void RemoveShader(const std::string& a_passName, size_t a_index);

	private:
		// シェーディングモデルタイプネーム
		std::string m_typeName;

		// 保存用データ
		std::unordered_map<std::string, std::vector<Engine::GUID>> m_shaderGUIDMap = {};

		// ランタイムデータ
		// パス名ハッシュ、シェーダーハンドル
		std::unordered_map<UINT, std::vector<ResourceRef<Shader>>> m_shaderHandleMap = {};

		// このマテリアルが描画されるパスのリスト
		std::vector<std::string> m_activePasses; // シリアライズ用
		std::vector<UINT> m_activePassHashes;    // ランタイム用
	};
}