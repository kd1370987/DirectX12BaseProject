#pragma once
namespace Engine::Resource
{
	class ShadingModelAsset
	{
	public:

		// ---- アクセサ ----
		std::span<const Handle<Shader>> GetShaderHandles(UINT a_passHash) const;
		std::span<const Handle<ShaderLibrary>> GetShaderLibraryHandles(UINT a_passHash) const;

		/// <summary>
		/// シリアライズ処理
		/// </summary>
		/// <param name="a_ar">保存・読み込み</param>
		void Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// 対応するパスを追加
		/// </summary>
		void AddPath(const std::string& a_pathName);

		/// <summary>
		/// パスを指定して、対応するシェーダーを追加する
		/// </summary>
		/// <param name="a_pathName">パスネーム</param>
		/// <param name="a_guid">シェーダーGUID</param>
		void AddShader(const std::string& a_pathName,const Engine::GUID& a_guid);

	private:
		// シェーディングモデルタイプネーム
		std::string m_typeName;

		// 保存用データ
		std::unordered_map<UINT, std::vector<Engine::GUID>> m_shaderGUIDMap = {};
		std::unordered_map<UINT, std::vector<Engine::GUID>> m_shaderLibraryGUIDMap = {};

		// ランタイムデータ
		// パス名ハッシュ、シェーダーハンドル
		std::unordered_map<UINT, std::vector<Handle<Shader>>> m_shaderHandleMap = {};
		std::unordered_map<UINT, std::vector<Handle<ShaderLibrary>>> m_shaderLibaryHandleMap = {};
	};
}