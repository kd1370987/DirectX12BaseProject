//==========================================================================================
//
// BuiltinCompositeNodes (Engine::Editor::Inspector)
//
// エンジン標準の合成ノード。
//
//   SpatialDenoise : 同じデノイズパスを数珠つなぎにしたもの。回数をノード上で選ぶ
//   Bloom          : 抽出 → 縮小4段 → 合流 → 合成 を1ノードに見せる
//
// どちらも「グラフはパスを並べたまま、見た目だけまとめる」形。
// ランタイムには一切手を入れていない
//
//==========================================================================================
#include "CompositeNode.h"

#include "../PassEditor/PassEditor.h"

#include "Engine/Editor/Helper/EditorHelper.h"

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPipelineMetaRegistry.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPipelineAsset/RenderingPipelineAsset.h"

#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Denoise/GI/GISpatialDenoisePass/GISpatialDenoisePass.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Denoise/Shadow/ShadowSpatialDenoisePass/ShadowSpatialDenoisePass.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Bloom/BloomExtractPass/BloomExtractPass.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Bloom/BloomCompositePass/BloomCompositePass.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Bloom/KawaseBlurPass/KawaseBlurPass.h"
#include "Engine/Graphics/RenderingPipeline/RenderingPasses/PostEffect/Blur/GaussianBlurPass/GaussianBlurPass.h"

namespace Engine::Editor::Inspector
{
	using namespace Engine::Graphics::Pipeline;

	namespace
	{
		// まとまりへ入れつつグラフへ足す
		template<class TPass>
		TPass* AddGroupedPass(
			RenderingPipelineAsset& a_asset,
			const std::string& a_typeName,
			const Engine::GUID& a_groupGUID,
			int a_index,
			const std::string& a_passName)
		{
			PassMetaRegistry* _pRegistry = a_asset.RefMetaRegistry();
			RenderGraph* _pGraph = a_asset.RefRenderGraph();
			if (!_pRegistry || !_pGraph) return nullptr;

			const ID<Pass> _typeID = _pRegistry->GetTypeID<TPass>();
			if (!_typeID.IsValid()) return nullptr;

			Pass* _pPass = _pGraph->AddPass(*_pRegistry, _typeID);
			if (!_pPass) return nullptr;

			_pPass->SetName(a_passName);
			_pPass->SetEditorGroup(a_typeName, a_groupGUID, a_index);

			return static_cast<TPass*>(_pPass);
		}

		//==================================================================================
		//
		// 空間デノイズの反復
		//
		// 1段目の入力(GI / Shadow・Depth・Normal)を外へ見せ、
		// 最終段の Result を出口にする。
		// 段と段のつなぎと、2段目以降の Depth / Normal は中に隠す
		//
		//==================================================================================
		template<class TPass>
		class SpatialDenoiseComposite : public ICompositeNode
		{
		public:

			// a_pSourcePin : ならす対象が入ってくるピン名(GI / Shadow)
			SpatialDenoiseComposite(const char* a_pDisplayName, const char* a_pSourcePin, const char* a_pBaseName)
				: m_pDisplayName(a_pDisplayName)
				, m_pSourcePin(a_pSourcePin)
				, m_pBaseName(a_pBaseName)
			{}

			const char* GetDisplayName() const override { return m_pDisplayName; }

			std::string MakeTitle(const CompositeGroup& a_group) const override
			{
				return std::string(m_pDisplayName) + " x" + std::to_string(a_group.members.size());
			}

			//------------------------------------------------------------------------------
			// ノードの中身 : 回数だけ
			//------------------------------------------------------------------------------
			EPassEditResult DrawNode(const CompositeGroup& a_group, CompositeNodeRequest& a_outRequest) override
			{
				int _count = static_cast<int>(a_group.members.size());

				ImGui::SetNextItemWidth(120.0f);
				if (!ImGui::DragInt("Count", &_count, 0.1f, 1, kMaxCount)) return EPassEditResult::None;

				_count = std::clamp(_count, 1, kMaxCount);
				if (_count == static_cast<int>(a_group.members.size())) return EPassEditResult::None;

				// ここで増減させると、ノードを回している最中にパス配列が変わる。
				// 要求だけ置いて、実際に触るのは描き終わってから
				a_outRequest.resizeCount = _count;

				// パスが増減するので、リソースの要件ごと組み直しになる
				return EPassEditResult::Structure;
			}

			//------------------------------------------------------------------------------
			// 詳細 : 段ごとの設定を並べる
			//------------------------------------------------------------------------------
			EPassEditResult DrawDetail(
				const CompositeGroup& a_group,
				PassEditorRegistry& a_passEditorRegistry,
				CompositeNodeRequest& a_outRequest) override
			{
				ImGui::TextDisabled("同じパスを %d 段つないでいます", static_cast<int>(a_group.members.size()));
				ImGui::TextDisabled("StepSize は段ごとに 1, 2, 4, 8... と広げます");
				ImGui::Separator();

				EPassEditResult _result = EPassEditResult::None;

				for (size_t _i = 0; _i < a_group.members.size(); ++_i)
				{
					Pass* _pPass = a_group.members[_i];
					if (!_pPass) continue;

					ImGui::PushID(static_cast<int>(_i));

					const std::string _label = "Stage " + std::to_string(_i);
					if (ImGui::TreeNodeEx(_label.c_str(), _i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))
					{
						// 段ごとの中身は、単体のときと同じ編集UIをそのまま使う
						if (IPassEditor* _pEditor = a_passEditorRegistry.Find(*_pPass))
						{
							const EPassEditResult _passResult = _pEditor->DrawDetail(*_pPass);

							// 強いほう(組み直し)が勝つ
							if (_passResult == EPassEditResult::Structure) _result = EPassEditResult::Structure;
							else if (_passResult == EPassEditResult::Param && _result == EPassEditResult::None)
							{
								_result = EPassEditResult::Param;
							}
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}

				ImGui::Separator();
				const int _count = static_cast<int>(a_group.members.size());

				if (EditorHelper::CreateButton("Add Stage") && _count < kMaxCount)
				{
					a_outRequest.resizeCount = _count + 1;
					_result = EPassEditResult::Structure;
				}
				ImGui::SameLine();
				if (EditorHelper::DeleteSmallButton("Remove Stage") && _count > 1)
				{
					a_outRequest.resizeCount = _count - 1;
					_result = EPassEditResult::Structure;
				}

				return _result;
			}

			//------------------------------------------------------------------------------
			// 中の配線を配り直す
			//
			// 見せているのは1段目のピンだけなので、外から線が引かれても
			// 2段目以降の Depth / Normal は空のまま残る。
			// 必須入力なので埋めておかないと検証で落ちる
			//------------------------------------------------------------------------------
			void SyncInternalLinks(RenderGraph& a_graph, const CompositeGroup& a_group) override
			{
				if (a_group.members.size() < 2) return;

				Pass* _pHead = a_group.GetHead();
				if (!_pHead) return;

				const uint32_t _sourceSlotID = Pass::MakeSlotID(m_pSourcePin);
				const uint32_t _resultSlotID = Pass::MakeSlotID(kResultPin);

				for (size_t _i = 1; _i < a_group.members.size(); ++_i)
				{
					Pass* _pPrev = a_group.members[_i - 1];
					Pass* _pCur = a_group.members[_i];
					if (!_pPrev || !_pCur) continue;

					// 前の段の結果を受け取る
					a_graph.Link(_pPrev->GetGUID(), _resultSlotID, _pCur->GetGUID(), _sourceSlotID);

					// 全段が要る共有入力は1段目と同じ相手から取る
					for (const char* _pPin : kSharedPins)
					{
						CompositeUtil::CopyInputLink(a_graph, *_pHead, *_pCur, _pPin);
					}
				}
			}

			//------------------------------------------------------------------------------
			// 新規作成 : まず1段だけ置く
			//------------------------------------------------------------------------------
			Pass* Build(RenderingPipelineAsset& a_asset) override
			{
				Engine::GUID _groupGUID = {};
				_groupGUID.Create();

				TPass* _pPass = AddGroupedPass<TPass>(
					a_asset, m_pDisplayName, _groupGUID, 0, std::string(m_pBaseName) + "0");
				if (!_pPass) return nullptr;

				ApplyStage(*_pPass, 0);
				return _pPass;
			}

			static constexpr int kMaxCount = 8;

		private:

			static constexpr const char* kResultPin = "Result";

			// 段が増えても外から取り直す入力
			static constexpr const char* kSharedPins[] = { "Depth", "Normal" };

			// 段ごとの値を決める。
			// StepSize は 1, 2, 4, 8... と倍にしていくのが本来の使い方
			void ApplyStage(TPass& a_pass, int a_index) const
			{
				auto& _params = a_pass.RefParams();
				_params.cb.stepSize = 1 << a_index;
				_params.resourceName = std::string(m_pBaseName) + std::to_string(a_index);
				a_pass.ApplyResourceName();
			}

			//------------------------------------------------------------------------------
			// 段数を変える
			//
			// 増やすときは末尾へ足して数珠つなぎにし、
			// 減らすときは末尾から消す。
			// どちらも「最終段の結果を受け取っていた相手」へ繋ぎ直すのを忘れないこと
			//------------------------------------------------------------------------------
			bool Resize(RenderingPipelineAsset& a_asset, const CompositeGroup& a_group, int a_count) override
			{
				RenderGraph* _pGraph = a_asset.RefRenderGraph();
				if (!_pGraph) return false;

				const int _current = static_cast<int>(a_group.members.size());
				if (a_count == _current || a_count < 1) return false;

				Pass* _pOldTail = a_group.GetTail();
				if (!_pOldTail) return false;

				// 最終段の結果を受け取っていた相手を控える(繋ぎ直しに要る)
				const uint32_t _resultSlotID = Pass::MakeSlotID(kResultPin);
				std::vector<std::pair<Engine::GUID, uint32_t>> _consumerVec = {};
				for (const auto& [_srcGUID, _connectionVec] : _pGraph->GetConnections())
				{
					if (_srcGUID != _pOldTail->GetGUID()) continue;

					for (const Connection& _connection : _connectionVec)
					{
						if (_connection.srcSlotID != _resultSlotID) continue;
						_consumerVec.emplace_back(_connection.dstPassGUID, _connection.dstSlotID);
					}
				}

				// 増減
				std::vector<Pass*> _members = a_group.members;
				if (a_count > _current)
				{
					for (int _i = _current; _i < a_count; ++_i)
					{
						TPass* _pNew = AddGroupedPass<TPass>(
							a_asset, a_group.typeName, a_group.guid, _i,
							std::string(m_pBaseName) + "Pass" + std::to_string(_i));
						if (!_pNew) break;

						ApplyStage(*_pNew, _i);

						// 並べる位置を1段ぶんずらしておく(重なって掴めなくなるのを避ける)
						const Math::Vector2& _prevPos = _members.back()->GetEditorPos();
						_pNew->SetEditorPos(Math::Vector2(_prevPos.x + 40.0f, _prevPos.y + 40.0f));

						_members.push_back(_pNew);
					}
				}
				else
				{
					for (int _i = _current - 1; _i >= a_count; --_i)
					{
						_pGraph->RemovePass(_members[_i]->GetGUID());
						_members.pop_back();
					}
				}

				if (_members.empty()) return false;

				// 段の並びを振り直す
				for (size_t _i = 0; _i < _members.size(); ++_i)
				{
					_members[_i]->SetEditorGroup(a_group.typeName, a_group.guid, static_cast<int>(_i));
				}

				// 中の配線を組み直す
				CompositeGroup _newGroup = {};
				_newGroup.guid = a_group.guid;
				_newGroup.typeName = a_group.typeName;
				_newGroup.members = _members;
				SyncInternalLinks(*_pGraph, _newGroup);

				// 最終段の結果を、元の受け取り先へ繋ぎ直す
				Pass* _pNewTail = _members.back();
				for (const auto& [_dstGUID, _dstSlotID] : _consumerVec)
				{
					_pGraph->Link(_pNewTail->GetGUID(), _resultSlotID, _dstGUID, _dstSlotID);
				}

				return true;
			}

			const char* m_pDisplayName = "";
			const char* m_pSourcePin = "";
			const char* m_pBaseName = "";
		};

		//==================================================================================
		//
		// ブルーム
		//
		// 抽出 → ガウシアン縮小4段 → Kawase 合流 → 合成。
		// Kawase の入力が Down0..Down3 の4本固定なので、段数は変えられない。
		//
		// 外へ見せるのは
		//   入力 : 抽出の Color と 合成の Color(どちらも素の絵をもらう)
		//   出力 : 合成の Result
		//
		//==================================================================================
		class BloomComposite : public ICompositeNode
		{
		public:

			const char* GetDisplayName() const override { return kTypeName; }

			std::string MakeTitle(const CompositeGroup& a_group) const override
			{
				(void)a_group;
				return "Bloom";
			}

			//------------------------------------------------------------------------------
			// 見せるピン
			//
			// 数珠つなぎではないので既定のままだと合わない。
			// 抽出と合成の入力・合成の出力だけを出す
			//------------------------------------------------------------------------------
			void CollectVisibleSlots(
				const CompositeGroup& a_group,
				std::vector<Slot*>& a_outInputVec,
				std::vector<Slot*>& a_outOutputVec) const override
			{
				a_outInputVec.clear();
				a_outOutputVec.clear();

				Pass* _pExtract = FindMember<BloomExtractPass>(a_group);
				Pass* _pComposite = FindMember<BloomCompositePass>(a_group);

				if (_pExtract)
				{
					for (Slot& _in : _pExtract->RefInputSlots()) a_outInputVec.push_back(&_in);
				}
				if (_pComposite)
				{
					// Bloom ピンは Kawase から中で来るので見せない
					for (Slot& _in : _pComposite->RefInputSlots())
					{
						if (_in.pinName == kBloomPin) continue;
						a_outInputVec.push_back(&_in);
					}
					for (Slot& _out : _pComposite->RefOutputSlots()) a_outOutputVec.push_back(&_out);
				}
			}

			EPassEditResult DrawNode(const CompositeGroup& a_group, CompositeNodeRequest& a_outRequest) override
			{
				(void)a_outRequest;

				ImGui::TextDisabled("Extract -> Blur x4 -> Kawase -> Composite");
				ImGui::TextDisabled("段数は Kawase の入力が4本固定なので変えられません");

				// 一番よく触る強さだけノードに出しておく
				auto* _pComposite = FindMember<BloomCompositePass>(a_group);
				if (!_pComposite) return EPassEditResult::None;

				auto& _params = static_cast<BloomCompositePass*>(_pComposite)->RefParams();

				ImGui::SetNextItemWidth(120.0f);
				const bool _isEdit = ImGui::DragFloat("Intensity", &_params.intensity, 0.01f, 0.0f);

				return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
			}

			EPassEditResult DrawDetail(
				const CompositeGroup& a_group,
				PassEditorRegistry& a_passEditorRegistry,
				CompositeNodeRequest& a_outRequest) override
			{
				(void)a_outRequest;

				ImGui::TextDisabled("縮小率ごとにボケの広がりが変わり、重ねると");
				ImGui::TextDisabled("芯は明るく外へゆるく広がる減衰になります");
				ImGui::Separator();

				EPassEditResult _result = EPassEditResult::None;

				for (size_t _i = 0; _i < a_group.members.size(); ++_i)
				{
					Pass* _pPass = a_group.members[_i];
					if (!_pPass) continue;

					ImGui::PushID(static_cast<int>(_i));
					if (ImGui::TreeNodeEx(_pPass->GetName().c_str(), _i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0))
					{
						if (IPassEditor* _pEditor = a_passEditorRegistry.Find(*_pPass))
						{
							const EPassEditResult _passResult = _pEditor->DrawDetail(*_pPass);

							if (_passResult == EPassEditResult::Structure) _result = EPassEditResult::Structure;
							else if (_passResult == EPassEditResult::Param && _result == EPassEditResult::None)
							{
								_result = EPassEditResult::Param;
							}
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}

				return _result;
			}

			//------------------------------------------------------------------------------
			// 中の配線
			//
			// 形が固定なので毎回同じつなぎ方をすればよい
			//------------------------------------------------------------------------------
			void SyncInternalLinks(RenderGraph& a_graph, const CompositeGroup& a_group) override
			{
				auto* _pExtract = FindMember<BloomExtractPass>(a_group);
				auto* _pKawase = FindMember<KawaseBlurPass>(a_group);
				auto* _pComposite = FindMember<BloomCompositePass>(a_group);
				if (!_pExtract || !_pKawase || !_pComposite) return;

				const uint32_t _resultSlotID = Pass::MakeSlotID(kResultPin);
				const uint32_t _colorSlotID = Pass::MakeSlotID(kColorPin);

				// 抽出 -> 縮小1段目 -> 2段目 ... と数珠つなぎ
				Pass* _pPrev = _pExtract;
				for (int _i = 0; _i < kDownCount; ++_i)
				{
					Pass* _pDown = FindDownStage(a_group, _i);
					if (!_pDown) return;

					a_graph.Link(_pPrev->GetGUID(), _resultSlotID, _pDown->GetGUID(), _colorSlotID);

					// 各段の結果を Kawase の Down0..Down3 へ
					a_graph.Link(
						_pDown->GetGUID(), _resultSlotID,
						_pKawase->GetGUID(), Pass::MakeSlotID("Down" + std::to_string(_i)));

					_pPrev = _pDown;
				}

				// Kawase の結果を合成へ
				a_graph.Link(_pKawase->GetGUID(), _resultSlotID, _pComposite->GetGUID(), Pass::MakeSlotID(kBloomPin));
			}

			//------------------------------------------------------------------------------
			// 新規作成 : 一式まとめて置く
			//------------------------------------------------------------------------------
			Pass* Build(RenderingPipelineAsset& a_asset) override
			{
				RenderGraph* _pGraph = a_asset.RefRenderGraph();
				if (!_pGraph) return nullptr;

				Engine::GUID _groupGUID = {};
				_groupGUID.Create();

				int _index = 0;
				auto* _pExtract = AddGroupedPass<BloomExtractPass>(a_asset, kTypeName, _groupGUID, _index++, "BloomExtractPass");
				if (!_pExtract) return nullptr;

				// 各段の解像度スケールとブラーの広がり。
				// すべて縮小後の低解像度で回るので広め(5x5)に取れる
				constexpr float kScales[kDownCount] = { 0.5f, 0.25f, 0.125f, 0.0625f };
				constexpr float kSigma = 1.2f;
				constexpr int   kTapRadius = 2;

				for (int _i = 0; _i < kDownCount; ++_i)
				{
					auto* _pDown = AddGroupedPass<GaussianBlurPass>(
						a_asset, kTypeName, _groupGUID, _index++, "BloomBlurDownPass" + std::to_string(_i));
					if (!_pDown) return nullptr;

					_pDown->Configure("BloomBlurDown" + std::to_string(_i), kScales[_i], kSigma, kTapRadius);
				}

				if (!AddGroupedPass<KawaseBlurPass>(a_asset, kTypeName, _groupGUID, _index++, "KawaseBlurPass")) return nullptr;
				if (!AddGroupedPass<BloomCompositePass>(a_asset, kTypeName, _groupGUID, _index++, "BloomCompositePass")) return nullptr;

				// 中の配線は組み直しに任せる
				CompositeGroupTable _table = BuildCompositeGroups(*_pGraph);
				if (const CompositeGroup* _pGroup = _table.Find(_pExtract->GetGUID()))
				{
					SyncInternalLinks(*_pGraph, *_pGroup);
				}
				return _pExtract;
			}

			static constexpr const char* kTypeName = "Bloom";

		private:

			static constexpr int kDownCount = 4;
			static constexpr const char* kResultPin = "Result";
			static constexpr const char* kColorPin = "Color";
			static constexpr const char* kBloomPin = "Bloom";

			// 段の番号は EditorGroupIndex で決まる(抽出が0なので +1)
			static Pass* FindDownStage(const CompositeGroup& a_group, int a_downIndex)
			{
				const int _target = a_downIndex + 1;
				for (Pass* _pPass : a_group.members)
				{
					if (_pPass && _pPass->GetEditorGroupIndex() == _target) return _pPass;
				}
				return nullptr;
			}

			template<class TPass>
			static TPass* FindMember(const CompositeGroup& a_group)
			{
				for (Pass* _pPass : a_group.members)
				{
					if (auto* _pTyped = dynamic_cast<TPass*>(_pPass)) return _pTyped;
				}
				return nullptr;
			}
		};
	}

	//======================================================================================
	//
	// 登録
	//
	//======================================================================================
	void RegisterBuiltinCompositeNodes(CompositeNodeRegistry& a_registry)
	{
		a_registry.Register<SpatialDenoiseComposite<GISpatialDenoisePass>>(
			"GISpatialDenoise", "GISpatialDenoise", "GI", "DenoisedGI");

		a_registry.Register<SpatialDenoiseComposite<ShadowSpatialDenoisePass>>(
			"ShadowSpatialDenoise", "ShadowSpatialDenoise", "Shadow", "DenoisedShadow");

		a_registry.Register<BloomComposite>(BloomComposite::kTypeName);
	}
}
