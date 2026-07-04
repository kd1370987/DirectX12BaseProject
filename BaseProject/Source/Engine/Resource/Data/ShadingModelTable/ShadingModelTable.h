#pragma once
namespace Engine::Resource
{
	class ShadingModelTable
	{
	public:

		ShadingModelTable() {}
		ShadingModelTable(const std::string& a_name) : m_typeName(a_name) {}

		// ---- アクセサ ----
		std::span<const Handle<Shader>> GetShaderHandles(UINT a_passHash) const;

		/// <summary>
		/// シリアライズ処理
		/// </summary>
		/// <param name="a_ar">保存・読み込み</param>
		void Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// 編集
		/// </summary>
		void Edit(const Engine::GUID& a_guid);

	private:
		// シェーディングモデルタイプネーム
		std::string m_typeName;

		// 保存用データ
		std::unordered_map<std::string, std::vector<Engine::GUID>> m_shaderGUIDMap = {};

		// ランタイムデータ
		// パス名ハッシュ、シェーダーハンドル
		std::unordered_map<UINT, std::vector<Handle<Shader>>> m_shaderHandleMap = {};
	};
}