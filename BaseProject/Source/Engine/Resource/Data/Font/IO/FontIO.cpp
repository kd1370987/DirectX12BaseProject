#include "FontIO.h"

namespace Engine::Resource
{
	Font FontIO::LoadFromFile(const std::string& a_filePath, const ResourceBuildContext* a_pContext)
	{
		Font _font = {};
		if (!_font.Load(a_filePath, a_pContext))
		{
			// 読めなかったときは空のまま返す。
			// 使う側は Font::IsValid() を見て描画を諦める
			return _font;
		}

		// 半角の英数字・記号は必ず出るので、読み込みのバッチに相乗りさせて焼いておく。
		// ここで一度でも Flush が走れば、アトラス全面の0クリアも一緒に転送される
		_font.PreloadAscii(a_pContext);

		return _font;
	}
}
