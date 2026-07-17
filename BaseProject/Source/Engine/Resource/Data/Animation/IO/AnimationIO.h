#pragma once
namespace Engine::Resource
{
	class AnimationIO
	{
	public:
		/// <summary>
		/// パーティクルの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>パーティクルの実体</returns>
		static AnimationData LoadFromFile(const std::string& a_path);
	};
}