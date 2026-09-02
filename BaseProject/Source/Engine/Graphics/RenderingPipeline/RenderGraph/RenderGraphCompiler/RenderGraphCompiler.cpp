#include "RenderGraphCompiler.h"

#include "../RenderGraph.h"

namespace Engine::Graphics::Pipeline
{
	bool RenderGraphCompiler::Compile(RenderGraph* a_pRenderGraph)
	{
		a_pRenderGraph->ClearCompiledData();

		// 繋ぎ方のみでわかる不備を検証 : エディターで作る関係上エラーが追いにくいため
		if (!a_pRenderGraph->Validate())
		{
			for (const auto& _issue : a_pRenderGraph->GetValidationIssues())
			{
				if (_issue.level != ValidationIssue::ELevel::Error) continue;
				ENGINE_WARNING("[RenderGraph] %s", _issue.message.c_str());
			}
			ENGINE_WARNING("[RenderGraph] 検証に失敗したためコンパイルを中止しました");
			return false;
		}

		// 線の状態をスロットへ反映する
		// 出力名がグラフでそろうまで往復する
		{
			// 出力名の一覧,これが変わらなくなったら反映終了
			auto _snapshotName = [&a_pRenderGraph]()
				{
					std::vector<std::string> _nameVec = {};
					for (const auto& _upPass : a_pRenderGraph->GetPasses())
					{
						if (!_upPass) continue;
						for (const Slot& _out : _upPass->GetOutputSlots())
						{
							_nameVec.push_back(_out.name);
						}
					}
					return _nameVec;
				};

			// 最悪でもパスの数だけ回れば端まで伝わる
			const size_t _maxLoop = a_pRenderGraph->GetPasses().size() + 1;
			std::vector<std::string> _prevNameVec = {};

			for (size_t _i = 0; _i < _maxLoop; ++_i)
			{

			}
		}

		return false;
	}
	void RenderGraphCompiler::ApplyLinks(RenderGraph* a_pRenderGraph)
	{
	}
}