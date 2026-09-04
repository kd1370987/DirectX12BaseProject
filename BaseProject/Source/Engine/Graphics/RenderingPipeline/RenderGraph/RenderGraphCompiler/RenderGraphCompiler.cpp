#include "RenderGraphCompiler.h"

#include "../RenderGraph.h"
#include "../../Core/Pass/Pass.h"
#include "../Resource/ResourceRegistry.h"

namespace Engine::Graphics::Pipeline
{
	CompileResult RenderGraphCompiler::Compile()
	{
		CompileResult _result = {};
		if (!m_pRenderGraph) return _result;

		// 繋ぎ方のみでわかる不備を検証 : エディターで作る関係上エラーが追いにくいため
		if (!m_pRenderGraph->Validate())
		{
			for (const ValidationIssue& _issue : m_pRenderGraph->GetValidationIssues())
			{
				if (_issue.level != ValidationIssue::ELevel::Error) continue;
				ENGINE_WARNING("[RenderGraph] %s", _issue.message.c_str());
			}
			ENGINE_WARNING("[RenderGraph] 検証に失敗したためコンパイルを中止しました");
			return _result;
		}

		//----------------------------------------------------------------------------------
		// 実行順を先に決める
		//
		// 依存の辺は「つないだ相手」だけで決まり、リソース名を一切見ない。
		// 先に並べてしまえば、名前の伝播は依存順に一回流すだけで端まで届く
		//----------------------------------------------------------------------------------
		std::vector<Pass*> _sortedVec = {};
		if (!BuildExecutionOrder(_sortedVec)) return _result;

		// 配線をスロットへ反映する
		StampResourceIDs();					// 出力ピンに識別子を覚えさせる
		ResolveLinksInOrder(_sortedVec);	// 配線をソート済み配列に沿ってそろえる

		// 名前が確定したため同じリソースを複数のパスで使っていないかを確認
		ValidateResourceWriters();

		// 配線が完成したため仮想リソースを組みなおす : GPUには触らない
		BuildVirtualResources();

		// パスが一つもないグラフ : 差し込まれた外部リソースだけを起こして終わり
		if (_sortedVec.empty())
		{
			_result.isSuccess = true;
			return _result;
		}

		// コンパイルパス配列を作成する
		_result.compiledPassVec.reserve(_sortedVec.size());
		for (Pass* _pPass : _sortedVec)
		{
			CompiledPass _compiledPass = {};
			_compiledPass.pPass = _pPass;
			_result.compiledPassVec.push_back(std::move(_compiledPass));
		}

		// リソースの生存区間を計算 : 実行順が決まって初めて「何番目から何番目まで要るか」が言える
		BuildResourceLifetimes(_result.compiledPassVec);

		// リソースのステート遷移を積む
		BuildBarriers(_result.compiledPassVec, _result.endBarrierVec);

		// 各パスの Compile() はここでは呼ばない。
		// 物理リソースが決まってからでないとディスクリプタを引けないので、
		// AllocateResources() の最後で呼ぶ(仕様のコンパイル順どおり)
		_result.isSuccess = true;
		return _result;
	}

	//======================================================================================
	// 実行順を決める
	//
	// 「つないだ相手」だけを辺にする。リソース名で突き合わせると、
	// 同じリソースへ描き足すパスが並んだときに全員が互いの書き手になって循環する
	// (ライティング → 空 → パーティクルはどれも AfterLighting を読んで書く)。
	// 線は「この出力の後に走れ」という指示そのものなので、これだけを見ればよい
	//======================================================================================
	bool RenderGraphCompiler::BuildExecutionOrder(std::vector<Pass*>& a_outSortedVec)
	{
		a_outSortedVec.clear();

		// 並べ替え対象をポインタで集める : 実体は RenderGraph が持ち続けるので参照のみ
		const auto& _passes = m_pRenderGraph->GetPasses();

		std::vector<Pass*> _nodes = {};
		_nodes.reserve(_passes.size());
		for (const auto& _upPass : _passes)
		{
			if (!_upPass) continue;
			_nodes.push_back(_upPass.get());
		}

		// 依存の洗い出し : 繋いだ相手だけを辺とする
		std::unordered_map<Engine::GUID, std::unordered_set<Engine::GUID>> _dependMap = {};
		for (const auto& [_srcGUID, _connectionVec] : m_pRenderGraph->GetConnections())
		{
			// パスごとに出ている線を見る
			for (const Connection& _connection : _connectionVec)
			{
				// 自分自身へのつなぎは順序を持たない
				if (_connection.dstPassGUID == _srcGUID) continue;

				// 出力先パスを取得
				Pass* _pDst = m_pRenderGraph->FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				// テンポラルは前フレームの結果を読むため、同フレームの書き手を待たない。
				// 辺にしてしまうと「History を読んで History を書く」構成が循環になる
				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (_pDstSlot && _pDstSlot->isTemporal) continue;

				// 依存あり
				_dependMap[_connection.dstPassGUID].insert(_srcGUID);
			}
		}

		// lhs の入力が rhs から来ているなら、rhs が先に走るようにする : 入力をそろえるため
		auto _isDepends = [&_dependMap](const Pass* a_pLhs, const Pass* a_pRhs)
			{
				if (!a_pLhs || !a_pRhs) return false;

				auto _it = _dependMap.find(a_pLhs->GetGUID());
				if (_it == _dependMap.end()) return false;

				return _it->second.find(a_pRhs->GetGUID()) != _it->second.end();
			};

		// 依存の向き通りに一本へ並べる
		if (!_nodes.empty() && !Algorithm::Graph::TopologicalSort(_nodes, a_outSortedVec, _isDepends))
		{
			// 循環すると結果からパスが抜け落ちるため、中途半端な順序で走らせない
			ENGINE_WARNING("[RenderGraph] パスの並べ替えに失敗しました : 入出力が循環しています");
			a_outSortedVec.clear();
			return false;
		}

		return true;
	}

	//======================================================================================
	// 出力ピンへ識別子を焼く
	//
	// 「どのパスのどの出力ピンが作ったリソースか」を、配線を解決する前に確定させる。
	//
	// 描き足すパスは、このあとの OnLinksResolved で入力からもらった識別子へ差し替える。
	// 毎回ここで焼き直すので、前回のコンパイルで差し替わっていても元へ戻る
	//======================================================================================
	void RenderGraphCompiler::StampResourceIDs()
	{
		for (auto& _upPass : m_pRenderGraph->RefPasses())
		{
			if (!_upPass) continue;

			const Engine::GUID& _passGUID = _upPass->GetGUID();

			for (Slot& _out : _upPass->RefOutputSlots())
			{
				// グラフの外へ書き出すピンは差し込む側と名前で調整
				_out.resourceID = _out.importName.empty()
					? ResourceID::FromOutputSlot(_passGUID, _out.slotID)
					: ResourceID::FromImportName(_out.importName);
			}
		}
	}

	//======================================================================================
	// 配線を実行順に1回だけ解決する
	//
	// ApplyLinks は「接続元の出力スロット」を入力へ写すだけなので、これ単体では
	// 描き足すパス(OnLinksResolved で自分の出力名を入力に合わせるもの)の数珠つなぎに
	// 追いつけない。
	//
	//   魚眼(FishEyeColor) → デバッグ線 → UI → トーンマップ
	//
	// 前段の出力が決まる前に後段が名前を受け取ってしまうと、UI だけが誰も書かない
	// 別のテクスチャへ描くことになる(画面に UI しか出ない・前フレームの絵が残る)。
	//
	// 依存順に「入力を引き込む → 出力を決めさせる」を1回ずつやれば、
	// 後段を見るときには前段の出力がもう確定しているので、往復させる必要がない
	//======================================================================================
	void RenderGraphCompiler::ResolveLinksInOrder(const std::vector<Pass*>& a_sortedVec)
	{
		// 接続を入力側から引けるようにしておく。
		// 接続表は出力側が鍵なので、そのままだと
		// 「このパスの入力へ誰が来ているか」を毎回全走査することになる
		struct IncomingLink
		{
			Engine::GUID srcPassGUID = {};		// 元のパス
			uint32_t srcSlotID = 0;				// 元パスのスロットID
			uint32_t dstSlotID = 0;				// つながっているところのID
		};

		// 対応表作成
		std::unordered_map<Engine::GUID, std::vector<IncomingLink>> _incomingMap = {};
		for (const auto& [_srcGUID, _connectionVec] : m_pRenderGraph->GetConnections())
		{
			for (const Connection& _connection : _connectionVec)
			{
				IncomingLink _link = {};
				_link.srcPassGUID = _srcGUID;
				_link.srcSlotID = _connection.srcSlotID;
				_link.dstSlotID = _connection.dstSlotID;

				_incomingMap[_connection.dstPassGUID].push_back(_link);
			}
		}

		// 入力をすべて外す : 「今つながっている線」だけが正
		for (auto& _upPass : m_pRenderGraph->RefPasses())
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				_upPass->ClearInput(_in.slotID);
			}
		}

		// 実行順に一回ずつ
		for (Pass* _pPass : a_sortedVec)
		{
			if (!_pPass) continue;

			// この時点で前段の出力はもう動かないので、入力をそろえる
			auto _it = _incomingMap.find(_pPass->GetGUID());
			if (_it != _incomingMap.end())
			{
				for (const IncomingLink& _link : _it->second)
				{
					Pass* _pSrc = m_pRenderGraph->FindPass(_link.srcPassGUID);
					if (!_pSrc) continue;

					const Slot* _pSrcSlot = _pSrc->FindOutputSlot(_link.srcSlotID);
					if (!_pSrcSlot) continue;

					_pPass->SetInput(_link.dstSlotID, *_pSrcSlot);
				}
			}

			// 入力がそろったため、入力によってふるまいを変えるパスの出力を決める
			_pPass->OnLinksResolved();
		}

		// 前フレームを読むピン(Temporal)は実行順の辺にならないので、
		// 書き手が自分より後ろに居ることがある。そこだけは1周では埋まらない。
		// 出力はもう確定しているので、最後に配り直せば落ち着く
		m_pRenderGraph->ApplyLinks();
	}

	void RenderGraphCompiler::ValidateResourceWriters()
	{
		// リソースごとに自分では読まずに書いているパスを調べる
		struct Writers
		{
			std::string label = "";					// 出しても分かるように表示名を控える
			std::vector<const Pass*> passVec = {};
		};
		std::unordered_map<ResourceID, Writers> _creatorMap = {};

		for (const auto& _upPass : m_pRenderGraph->GetPasses())
		{
			if (!_upPass) continue;

			for (const Slot& _out : _upPass->GetOutputSlots())
			{
				if (!_out.resourceID.IsValid()) continue;

				// 同じリソースが入力にも来ていれば書き足す : このパスは作り手ではない
				bool _isFollower = false;
				for (const Slot& _in : _upPass->GetInputSlots())
				{
					if (!_in.IsConnected()) continue;
					if (_in.resourceID != _out.resourceID) continue;

					_isFollower = true;
					break;
				}
				if (_isFollower) continue;

				Writers& _writers = _creatorMap[_out.resourceID];
				_writers.label = _out.name;
				_writers.passVec.push_back(_upPass.get());
			}
		}

		for (const auto& [_resourceID, _writers] : _creatorMap)
		{
			if (_writers.passVec.size() <= 1) continue;

			// エラー用メッセージ : どのパスかが判別できるようにする
			std::string _passName = {};
			for (const Pass* _pPass : _writers.passVec)
			{
				if (!_passName.empty()) _passName += " / ";
				_passName += _pPass->GetName();
			}

			const std::string _message =
				"リソース \"" + _writers.label + "\" を複数のパスが作っています : " + _passName +
				" (同じ1枚に相乗りします。描き足すつもりなら入力へ繋いでください)";

			ValidationIssue _issue = {};
			_issue.level = ValidationIssue::ELevel::Warning;
			_issue.passGUID = _writers.passVec.front()->GetGUID();
			_issue.message = _message;
			m_pRenderGraph->AddIssue(std::move(_issue));

			// Compile はエラーしか並べて出さないので、ここで出しておく
			ENGINE_WARNING("[RenderGraph] %s", _message.c_str());
		}

	}

	//======================================================================================
	// 仮想リソースを、パスの入出力スロットから組み直す
	//
	// 「同じ識別子 = 同じリソース」なので、識別子ごとに1つ起こして要件を足し込んでいく
	//======================================================================================
	void RenderGraphCompiler::BuildVirtualResources()
	{
		auto* _pResRegistry = m_pRenderGraph->RefResourceRegistry();

		// 仮想リソースをすべてクリア
		_pResRegistry->ClearVirtualResources();

		// 外部リソースを仮想リソースとして先に登録 : パスが知らないもの(バックバッファなど)も席を持つ
		_pResRegistry->SetupImportedResources();

		// 出力スロットから仮想リソースを作成していく。
		// フォーマットやサイズを知っているのは作る側だけなので、先に出力を全部通す
		for (auto& _upPass : m_pRenderGraph->RefPasses())
		{
			if (!_upPass) continue;

			for (Slot& _out : _upPass->RefOutputSlots())
			{
				if (!_out.resourceID.IsValid()) continue;
				_pResRegistry->Request(_out).MergeSlot(_out);
			}
		}

		// 入力スロットの要件を足しこむ : 読む側は用途フラグ(SRV/UAV/DSV)のみを足す
		for (auto& _upPass : m_pRenderGraph->RefPasses())
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				// つながっていないピンは飛ばす
				if (!_in.IsConnected()) continue;

				VirtualResource* _pRes = _pResRegistry->RefByID(_in.resourceID);
				if (!_pRes)
				{
					// どのパスも作っていない = Import し忘れているリソース。
					// 黙って落とすと後で実体が無くて落ちるので出しておく
					ENGINE_WARNING(
						"[RenderGraph] 入力に来ているリソースを誰も作っていません : %s <- %s",
						_upPass->GetName().c_str(), _in.name.c_str());
					continue;
				}

				_pRes->MergeSlot(_in);
			}
		}

		// サイズを決める
		for (VirtualResource& _vRes : _pResRegistry->RefVirtualResources())
		{
			_vRes.ResolveSize(m_pRenderGraph->GetViewportWidth(), m_pRenderGraph->GetViewportHeight());
		}

		// スロットは識別子をそのまま持っているので、書き戻すものは無い。
		// この先で GetVirtualResource を引けば、今作った席がそのまま返る

		// 後続に使われているかを出力スロットへ書き戻す
		ResolveStoreOps();
	}

	// 出力したリソースが後続で使われるかどうかを StoreOp に落とす。
	//
	// 今は「使われ方を把握するための情報」でしかないが、
	// リソースの生存区間が分かる形にしておくと、
	// あとでエイリアシング(使い回し)を入れるときの判断材料になる
	void RenderGraphCompiler::ResolveStoreOps()
	{
		// どこかの入力に来ているリソースを集める
		// 名前でなく識別子で見ること : 表示名が同じでも別リソースがありうる
		std::unordered_set<ResourceID> _consumedIDSet = {};
		for (const auto& _upPass : m_pRenderGraph->GetPasses())
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				if (!_in.IsConnected()) continue;
				_consumedIDSet.insert(_in.resourceID);
			}
		}

		for (auto& _upPass : m_pRenderGraph->RefPasses())
		{
			if (!_upPass) continue;

			for (Slot& _out : _upPass->RefOutputSlots())
			{
				// 外部から差し込まれたリソース(このカメラの最終出力など)は
				// グラフの外で使われるので、誰も読んでいなくても残す
				const VirtualResource* _pVirtual = m_pRenderGraph->GetVirtualResource(_out.resourceID);
				const bool _isImported = (_pVirtual && _pVirtual->IsImported());

				const bool _isConsumed = _isImported || (_consumedIDSet.count(_out.resourceID) != 0);

				_out.storeOp = _isConsumed ? EStoreOp::Store : EStoreOp::DontCare;
			}
		}
	}

	//======================================================================================
	// リソースの生存区間を出す
	//
	// 並べ替えが済んだパスを頭から見て、
	// 各リソースを「最初に触ったパス」と「最後に触ったパス」で挟む。
	//
	// 区間の外ではそのリソースの中身が誰にも要らないので、
	// 区間が重ならないもの同士は同じ実体を使い回せる(エイリアシング)。
	// 今はまだ 1リソース = 1実体で作っているが、判断材料はここで揃う。
	//
	// 読み書きのどちらでも「触った」として数える。
	// 書いた瞬間から要るようになり、最後に読まれた時点で要らなくなるため
	//======================================================================================
	void RenderGraphCompiler::BuildResourceLifetimes(const std::vector<CompiledPass>& a_compiledPassVec)
	{
		auto* _pResRegistry = m_pRenderGraph->RefResourceRegistry();

		// すべての仮想リソースの生存時間をクリア
		for (VirtualResource& _vRes : _pResRegistry->RefVirtualResources())
		{
			_vRes.ResetLifetime();
		}

		// 並べかえがすんだパスを見てリソースを最初に触ったパスと最後に触ったパスで挟む
		for (uint32_t _passIdx = 0; _passIdx < static_cast<uint32_t>(a_compiledPassVec.size()); ++_passIdx)
		{
			const Pass* _pPass = a_compiledPassVec[_passIdx].pPass;
			if (!_pPass) continue;

			// 読み書きどちらでも触ったとみなす
			auto _extend = [&](const std::vector<Slot>& a_slotVec)
				{
					for (const Slot& _slot : a_slotVec)
					{
						// つながっていないピンはどのリソースも指していない
						VirtualResource* _pVRes = m_pRenderGraph->RefVirtualResource(_slot.resourceID);
						if (!_pVRes) continue;

						_pVRes->ExtendLifetime(_passIdx);
					}
				};

			_extend(_pPass->GetInputSlots());
			_extend(_pPass->GetOutputSlots());
		}
	}

	//======================================================================================
	//
	// バリア構築
	//
	//======================================================================================

	// 並んだパスを先頭から見て、リソースのステートが変わるところにバリアを積む。
	// 積み先は「そのパスの直前」なので、実行側は
	// 「preBarriers を張る -> パスを回す」を前から繰り返すだけでよい。
	//
	// GPUには触らないので実体(pResource)はここでは埋めない。
	// AllocateResources() の後に ResolveBarrierResources() で埋める
	void RenderGraphCompiler::BuildBarriers(
		std::vector<CompiledPass>& a_compiledPassVec,
		std::vector<ResourceBarrier>& a_outEndBarrierVec)
	{
		if (!m_pRenderGraph) return;

		a_outEndBarrierVec.clear();
		auto* _pResRegistry = m_pRenderGraph->RefResourceRegistry();

		// 各リソースのステートを初期値(フレーム入口)に戻してから積み始める
		for (VirtualResource& _vRes : _pResRegistry->RefVirtualResources())
		{
			_vRes.ResetStateToInitial();
		}

		// パスごとに遷移状態を作る
		for (CompiledPass& _compiledPass : a_compiledPassVec)
		{
			BuildPassBarriers(_compiledPass);
		}

		// フレームの終わりに入口のステートへ戻す。
		// グラフは毎フレーム同じ手順で回るので、戻しておかないと
		// 次のフレームで before が食い違ってバリアが張られなくなる
		for (VirtualResource& _vRes : _pResRegistry->RefVirtualResources())
		{
			const uint32_t _sliceCount = _vRes.GetPhysicalCount();
			for (uint32_t _slice = 0; _slice < _sliceCount; ++_slice)
			{
				const D3D12_RESOURCE_STATES _before = _vRes.GetCurrentState(_slice);
				const D3D12_RESOURCE_STATES _after = _vRes.GetInitialState(_slice);
				if (_before == _after) continue;

				// バリアに積む
				ResourceBarrier _barrier = {};
				_barrier.resourceID = _vRes.GetResourceID();
				_barrier.slice = _slice;
				_barrier.before = _before;
				_barrier.after = _after;
				a_outEndBarrierVec.push_back(_barrier);

				_vRes.SetCurrentState(_after, _slice);
			}
		}
	}
	void RenderGraphCompiler::BuildAliasingBarriers(std::vector<CompiledPass>& a_compiledPassVec, std::vector<AliasingBarrier>& a_outEndBarrierVec)
	{
		for (const VirtualResource& _vRes : m_pRenderGraph->GetVirtualResources())
		{
			const auto& _info = _vRes.GetAllocationInfo();

			// 初回使用ならバリアはいらない
			if (!_info.pPrevVRes) continue;

			// リソースが使われる初めのパスにのみ張る
			const uint32_t _firstPass = _vRes.GetFirstPassIndex();

			// バリア構築
			AliasingBarrier _barrier = {};
			_barrier.before = _info.pPrevVRes->GetResourceID();		// 前回のリソース
			_barrier.after = _vRes.GetResourceID();					// バリア後リソース

			a_compiledPassVec[_firstPass].preAliasingBarriers.push_back(_barrier);
		}
	}
	void RenderGraphCompiler::BuildPassBarriers(CompiledPass& a_compiledPass)
	{
		a_compiledPass.preBarriers.clear();

		Pass* _pPass = a_compiledPass.pPass;
		if (!_pPass) return;

		auto _pushBarrier = [&](const Slot& a_slot)
			{
				if (!a_slot.resourceID.IsValid()) return;
				if (a_slot.accessType == EAccessType::None) return;

				VirtualResource* _pVRes = m_pRenderGraph->RefVirtualResource(a_slot.resourceID);
				if (!_pVRes) return;

				// テンポラルは２枚の物理リソースが別々の遷移をたどるので、ステートもスライスごとに追う
				// 出力は Current / 入力は Previous
				const uint32_t _slice = VirtualResource::ToSlice(a_slot,_pVRes->IsTemporal());

				const D3D12_RESOURCE_STATES _before = _pVRes->GetCurrentState(_slice);
				const D3D12_RESOURCE_STATES _after = VirtualResource::ToResourceState(a_slot.accessType);

				const bool _isStateChanged = (_before != _after);

				// UAV -> UAV はステートが変わらないが、
				// 前の書き込みが終わるのを待たせないと結果が混ざる
				const bool _isUAVtoUAV =
					(_before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
						_after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				if (!_isStateChanged && !_isUAVtoUAV) return;

				ResourceBarrier _barrier = {};
				_barrier.resourceID = a_slot.resourceID;
				_barrier.slice = _slice;
				_barrier.before = _before;
				_barrier.after = _after;
				_barrier.isUAVBarrier = _isUAVtoUAV;
				a_compiledPass.preBarriers.push_back(_barrier);

				_pVRes->SetCurrentState(_after, _slice);
			};

		// 読込を先に、書き込みを後にみる
		// 同じパスが読んでから書くリソースの遷移順を崩さないため
		for (const Slot& _in : _pPass->GetInputSlots())
		{
			_pushBarrier(_in);
		}
		for (const Slot& _out : _pPass->GetOutputSlots())
		{
			_pushBarrier(_out);
		}
	}
}
