#pragma once
//==========================================================================================
//
// RenderingPipelineAssetIO
//
// パイプラインアセットの読み書き。
// 他のアセット(EffectAssetIO など)と同じ形にしてあるが、
// 読み込みには PassMetaRegistry が要る点だけが違う。
//
// 保存されているのはパスの「型ID」と配線だけなので、
// 読むときにレジストリから実体を作り直す必要がある。
// レジストリは ResourceBuildContext 経由で渡ってくる(GraphicsEngine が持ち主)。
//
//==========================================================================================
#include "../RenderingPipelineAsset/RenderingPipelineAsset.h"

namespace Engine::Graphics::Pipeline
{
	class PassMetaRegistry;

	class RenderingPipelineAssetIO
	{
	public:

		/// <summary>
		/// ファイルパスからの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <param name="a_pRegistry">生成できるパスの一覧(無いと空のアセットが返る)</param>
		/// <returns>実体を返す</returns>
		static RenderingPipelineAsset LoadFromFile(const std::string& a_path, PassMetaRegistry* a_pRegistry);

		/// <summary>
		/// 作成 : メタファイルと空のファイルを作成
		/// </summary>
		/// <param name="a_path">Asset/RenderingPipeline/ 以下のディレクトリ名</param>
		/// <param name="a_name">ファイルとパイプラインの名前</param>
		/// <param name="a_pRegistry">生成できるパスの一覧</param>
		static void Create(const std::string& a_path, const std::string& a_name, PassMetaRegistry* a_pRegistry);
	};
}
