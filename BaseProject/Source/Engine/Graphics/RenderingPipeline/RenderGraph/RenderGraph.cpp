#include "RenderGraph.h"

#include "../RenderingPipelineMetaRegistry.h"

// 実行時に触るもの
#include "../../RenderContext/RenderContext.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Graphics::Pipeline
{
	// スロットに割り当てられたGPUリソースを引く
	D3D12::GPUResource* PassContext::GetResource(const Slot& a_slot) const
	{
		if (!pGraph) return nullptr;
		return pGraph->RefGPUResource(a_slot);
	}

	void RenderGraph::Clear()
	{
		ReleaseResources();

		m_compilePasses.clear();
		m_endBarriers.clear();
		m_connectionMap.clear();
		m_passes.clear();

		// IDは振り直さない。
		// 振り直すと、保存済みのつなぎが持つピンIDと衝突する
	}

	//======================================================================================
	//
	// 設計図 / 実行インスタンス
	//
	//======================================================================================

	// 型IDからパスを作り直して、アーカイブから中身を流し込む
	std::unique_ptr<Pass> RenderGraph::CreatePassFromArchive(
		const PassMetaRegistry& a_registry, ID<Pass> a_typeID, Persistence::Archive& a_arch)
	{
		std::unique_ptr<Pass> _upPass = a_registry.Create(a_typeID);
		if (!_upPass)
		{
			ENGINE_WARNING("[RenderGraph] 未登録のパスタイプです : %u", a_typeID.value);
			return nullptr;
		}

		_upPass->SetTypeID(a_typeID);

		// 先に Init() でスロットを宣言させる。
		// ArchivePass はピンIDを「並び順で」写すので、器ができていないと入らない
		_upPass->Init();

		// 名前・GUID・ノード/ピンID・固有データが入る
		_upPass->ArchivePass(a_arch);

		return _upPass;
	}

	// 設計図のグラフから実行用のグラフを組み立てる。
	// パスは1つずつメモリ上のJSONへ書き出して読み直すので、
	// 各パスは Archive() さえ書いておけば複製にも対応できる(複製専用の関数を持たなくてよい)
	bool RenderGraph::BuildFrom(const RenderGraph& a_source, const PassMetaRegistry& a_registry)
	{
		Clear();

		// IDは設計図と同じものを使う。
		// 線はピン名で結び直すのでIDが被っても実害はないが、
		// エディター側と突き合わせるときに揃っていたほうが追いやすい
		m_idCounter = a_source.m_idCounter;

		m_passes.reserve(a_source.m_passes.size());
		for (const auto& _upSrcPass : a_source.m_passes)
		{
			if (!_upSrcPass) continue;

			// 設計図側を書き出して、実行用側で読み直す
			nlohmann::json _json = {};
			{
				Persistence::Archive _saveArch(Persistence::Archive::Mode::Save, _json);
				_upSrcPass->ArchivePass(_saveArch);
			}

			Persistence::Archive _loadArch(Persistence::Archive::Mode::Load, _json);
			std::unique_ptr<Pass> _upPass = CreatePassFromArchive(a_registry, _upSrcPass->GetTypeID(), _loadArch);
			if (!_upPass) continue;

			m_passes.push_back(std::move(_upPass));
		}

		// 配線はそのまま写す(GUIDとピン名で結んでいるので中身を作り替える必要がない)
		m_connectionMap = a_source.m_connectionMap;

		ApplyLinks();
		return true;
	}

	// 形は同じままなので、パスをGUIDで突き合わせて中身だけ写す
	void RenderGraph::SyncParamsFrom(const RenderGraph& a_source)
	{
		for (const auto& _upSrcPass : a_source.m_passes)
		{
			if (!_upSrcPass) continue;

			Pass* _pDst = FindPass(_upSrcPass->GetGUID());
			if (!_pDst) continue;

			nlohmann::json _json = {};
			{
				Persistence::Archive _saveArch(Persistence::Archive::Mode::Save, _json);
				_upSrcPass->ArchivePass(_saveArch);
			}

			Persistence::Archive _loadArch(Persistence::Archive::Mode::Load, _json);
			_pDst->ArchivePass(_loadArch);
		}
	}

	void RenderGraph::Archive(Persistence::Archive& a_arch, const PassMetaRegistry& a_registry)
	{
		a_arch.Field("idCounter", m_idCounter);

		//----------------------------------------------------------------------------------
		// パス
		//----------------------------------------------------------------------------------
		if (a_arch.IsLoading())
		{
			Clear();
			m_connectionMap.clear();
		}

		size_t _passCount = m_passes.size();
		if (a_arch.BeginArray("passes", _passCount))
		{
			for (size_t _i = 0; _i < _passCount; ++_i)
			{
				if (!a_arch.BeginObject(_i)) continue;

				if (a_arch.IsSaving())
				{
					Pass* _pPass = m_passes[_i].get();
					if (_pPass)
					{
						uint32_t _typeID = _pPass->GetTypeID().value;
						a_arch.Field("typeID", _typeID);
						_pPass->ArchivePass(a_arch);
					}
				}
				else
				{
					uint32_t _typeIDValue = 0;
					a_arch.Field("typeID", _typeIDValue);

					if (auto _upPass = CreatePassFromArchive(a_registry, ID<Pass>(_typeIDValue), a_arch))
					{
						m_passes.push_back(std::move(_upPass));
					}
				}

				a_arch.EndObject();
			}
			a_arch.EndArray();
		}

		//----------------------------------------------------------------------------------
		// 配線
		//
		// キーが接続元パスのGUIDなので、マップのままだと配列に落とせない。
		// 「接続元GUID + 線1本」の並びへ展開して保存する
		//----------------------------------------------------------------------------------
		std::vector<std::pair<Engine::GUID, Connection>> _flatConnectionVec = {};
		if (a_arch.IsSaving())
		{
			for (const auto& [_srcGUID, _connectionVec] : m_connectionMap)
			{
				for (const Connection& _connection : _connectionVec)
				{
					_flatConnectionVec.emplace_back(_srcGUID, _connection);
				}
			}
		}

		size_t _connectionCount = _flatConnectionVec.size();
		if (a_arch.BeginArray("connections", _connectionCount))
		{
			if (a_arch.IsLoading()) _flatConnectionVec.resize(_connectionCount);

			for (size_t _i = 0; _i < _connectionCount && _i < _flatConnectionVec.size(); ++_i)
			{
				if (!a_arch.BeginObject(_i)) continue;

				a_arch.GUIDField("srcPassGUID", _flatConnectionVec[_i].first);
				_flatConnectionVec[_i].second.Archive(a_arch);

				a_arch.EndObject();
			}
			a_arch.EndArray();
		}

		if (a_arch.IsLoading())
		{
			for (auto& [_srcGUID, _connection] : _flatConnectionVec)
			{
				m_connectionMap[_srcGUID].push_back(_connection);
			}

			// 読み込んだ配線を入力スロットへ流し込む
			ApplyLinks();
		}
	}

	//======================================================================================
	//
	// パス
	//
	//======================================================================================
	Pass* RenderGraph::AddPass(const PassMetaRegistry& a_registry, ID<Pass> a_typeID)
	{
		std::unique_ptr<Pass> _upPass = a_registry.Create(a_typeID);
		if (!_upPass) return nullptr;

		// 表示名と型IDはレジストリ側の情報から入れる
		const PassMeta* _pMeta = a_registry.GetMeta(a_typeID);
		_upPass->SetTypeID(a_typeID);
		_upPass->SetName(_pMeta ? _pMeta->name : std::string("Pass"));

		// GUID発行 + スロット宣言 -> そのぶんのノード/ピンIDを確保
		_upPass->Init();
		_upPass->EnsureEditorIDs([this]() { return GenerateID(); });

		// 置き場所 : 全部同じ場所に出ると重なって掴めないので少しずつずらす。
		// ImNodes へ反映するのは編集UI側の仕事なので、ここでは値を決めるだけ
		const float _offset = static_cast<float>(m_passes.size() % 8) * 30.0f;
		_upPass->SetEditorPos(Math::Vector2(40.0f + _offset, 40.0f + _offset));

		Pass* _pPass = _upPass.get();
		m_passes.push_back(std::move(_upPass));
		return _pPass;
	}

	void RenderGraph::RemovePass(const Engine::GUID& a_passGUID)
	{
		// このパスから出る線を消す
		m_connectionMap.erase(a_passGUID);

		// このパスへ入る線を消す
		for (auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			_connectionVec.erase(
				std::remove_if(_connectionVec.begin(), _connectionVec.end(),
					[&a_passGUID](const Connection& a_connection) { return a_connection.dstPassGUID == a_passGUID; }),
				_connectionVec.end());
		}

		// 実体を消す
		m_passes.erase(
			std::remove_if(m_passes.begin(), m_passes.end(),
				[&a_passGUID](const std::unique_ptr<Pass>& a_upPass)
				{ return a_upPass && a_upPass->GetGUID() == a_passGUID; }),
			m_passes.end());

		// 消したパスを指していた Pass* が残るので、コンパイル結果は捨てる
		m_compilePasses.clear();
		m_endBarriers.clear();

		ApplyLinks();
	}

	Pass* RenderGraph::FindPass(const Engine::GUID& a_passGUID)
	{
		for (auto& _upPass : m_passes)
		{
			if (_upPass && _upPass->GetGUID() == a_passGUID) return _upPass.get();
		}
		return nullptr;
	}

	Pass* RenderGraph::FindPassByNodeID(int a_nodeID)
	{
		for (auto& _upPass : m_passes)
		{
			if (_upPass && _upPass->GetNodeID() == a_nodeID) return _upPass.get();
		}
		return nullptr;
	}

	Pass* RenderGraph::FindPassByPinID(int a_pinID, Slot** a_ppOutSlot, bool* a_pOutIsInput)
	{
		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			bool _isInput = false;
			if (Slot* _pSlot = _upPass->FindSlotByPinID(a_pinID, &_isInput))
			{
				if (a_ppOutSlot) *a_ppOutSlot = _pSlot;
				if (a_pOutIsInput) *a_pOutIsInput = _isInput;
				return _upPass.get();
			}
		}
		return nullptr;
	}

	//======================================================================================
	//
	// つなぎ
	//
	//======================================================================================
	bool RenderGraph::Link(
		const Engine::GUID& a_srcPassGUID, uint32_t a_srcSlotID,
		const Engine::GUID& a_dstPassGUID, uint32_t a_dstSlotID)
	{
		Pass* _pSrc = FindPass(a_srcPassGUID);
		Pass* _pDst = FindPass(a_dstPassGUID);
		if (!_pSrc || !_pDst) return false;

		// スロットが実在するかまで見る(IDだけ渡されても張らない)
		const Slot* _pSrcSlot = _pSrc->FindOutputSlot(a_srcSlotID);
		const Slot* _pDstSlot = _pDst->FindInputSlot(a_dstSlotID);
		if (!_pSrcSlot || !_pDstSlot) return false;

		// 自分自身へのつなぎは基本的に循環になるので通さない。
		// ただし Temporal は前フレームの結果を読むだけなので、
		// TAA のように「自分の履歴を自分で読む」構成は許す
		if (a_srcPassGUID == a_dstPassGUID && !_pSrcSlot->isTemporal) return false;

		// 繋いだ瞬間に弾けるものはここで弾く。
		// 残り(必須未接続・循環)はグラフ全体を見ないと分からないので Validate() 側
		std::string _reason = {};
		if (!IsConnectable(*_pSrcSlot, *_pDstSlot, &_reason))
		{
			ENGINE_WARNING("[RenderGraph] つなげない組み合わせです : %s.%s -> %s.%s (%s)",
				_pSrc->GetName().c_str(), _pSrcSlot->pinName.c_str(),
				_pDst->GetName().c_str(), _pDstSlot->pinName.c_str(),
				_reason.c_str());
			return false;
		}

		// 入力スロットは1本まで : 先に張られていた線を外してから張り直す
		DisconnectInputSlot(a_dstPassGUID, a_dstSlotID);

		Connection _connection = {};
		_connection.linkID = GenerateID();
		_connection.dstPassGUID = a_dstPassGUID;
		_connection.srcSlotID = a_srcSlotID;
		_connection.dstSlotID = a_dstSlotID;
		m_connectionMap[a_srcPassGUID].push_back(_connection);

		// つないだ内容をすぐ入力スロットへ反映(ノード表示にも出る)
		ApplyLinks();
		return true;
	}

	// 線1本を消す : どのパスから出ているか分からないので全部見る
	void RenderGraph::RemoveLink(int a_linkID)
	{
		for (auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			_connectionVec.erase(
				std::remove_if(_connectionVec.begin(), _connectionVec.end(),
					[a_linkID](const Connection& a_connection) { return a_connection.linkID == a_linkID; }),
				_connectionVec.end());
		}
	}

	// 入力スロットは1本しか受けられないので、張り直す前に既存の線を外す
	void RenderGraph::DisconnectInputSlot(const Engine::GUID& a_dstPassGUID, uint32_t a_dstSlotID)
	{
		for (auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			_connectionVec.erase(
				std::remove_if(_connectionVec.begin(), _connectionVec.end(),
					[&](const Connection& a_connection)
					{ return a_connection.dstPassGUID == a_dstPassGUID && a_connection.dstSlotID == a_dstSlotID; }),
				_connectionVec.end());
		}
	}

	void RenderGraph::ApplyLinks()
	{
		// 入力を一旦すべて外す
		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;
			for (const Slot& _in : _upPass->GetInputSlots())
			{
				_upPass->ClearInput(_in.slotID);
			}
		}

		// 線をたどって出力スロットを流し込む
		for (auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			Pass* _pSrc = FindPass(_srcGUID);
			if (!_pSrc) continue;

			for (const Connection& _connection : _connectionVec)
			{
				Pass* _pDst = FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				const Slot* _pSrcSlot = _pSrc->FindOutputSlot(_connection.srcSlotID);
				if (!_pSrcSlot) continue;

				_pDst->SetInput(_connection.dstSlotID, *_pSrcSlot);
			}
		}
	}

	//======================================================================================
	//
	// リソース
	//
	//======================================================================================
	void RenderGraph::SetViewportSize(UINT64 a_width, UINT a_height)
	{
		m_viewportWidth = a_width;
		m_viewportHeight = a_height;
	}

	void RenderGraph::ImportResource(
		const std::string& a_name,
		D3D12::GPUResource* a_pResource,
		D3D12_RESOURCE_STATES a_initialState,
		EPassSlotType a_type)
	{
		// 同じ名前があれば差し替える
		for (ImportedResource& _imported : m_importedResourceVec)
		{
			if (_imported.name != a_name) continue;

			_imported.type = a_type;
			_imported.pResource = a_pResource;
			_imported.initialState = a_initialState;
			return;
		}

		ImportedResource _imported = {};
		_imported.name = a_name;
		_imported.type = a_type;
		_imported.pResource = a_pResource;
		_imported.initialState = a_initialState;
		m_importedResourceVec.push_back(std::move(_imported));
	}

	void RenderGraph::RemoveImportedResource(const std::string& a_name)
	{
		m_importedResourceVec.erase(
			std::remove_if(m_importedResourceVec.begin(), m_importedResourceVec.end(),
				[&a_name](const ImportedResource& a_imported) { return a_imported.name == a_name; }),
			m_importedResourceVec.end());
	}

	void RenderGraph::ClearImportedResources()
	{
		m_importedResourceVec.clear();
	}

	// 仮想リソースを、パスの入出力スロットから組み直す。
	// 「同じ名前 = 同じリソース」なので、名前ごとに1つ起こして要件を足し込んでいく
	void RenderGraph::BuildVirtualResources()
	{
		m_resourceNameMap.clear();
		m_virtualResourceVec.clear();

		// ---- 外部から差し込まれたものを先に並べる ----
		// パスが知らないリソース(バックバッファなど)もここで席を持つ
		for (const ImportedResource& _imported : m_importedResourceVec)
		{
			if (_imported.name.empty()) continue;
			if (m_resourceNameMap.find(_imported.name) != m_resourceNameMap.end()) continue;

			VirtualResource _res = {};
			_res.SetupAsImported(_imported.name, _imported.type, _imported.initialState);

			m_resourceNameMap[_imported.name] = static_cast<uint32_t>(m_virtualResourceVec.size());
			m_virtualResourceVec.push_back(std::move(_res));
		}

		// ---- 出力スロットから起こす ----
		// フォーマットやサイズを知っているのは作る側だけなので、先に出力を全部通す
		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			for (const Slot& _out : _upPass->GetOutputSlots())
			{
				if (_out.name.empty()) continue;
				FindOrCreateVirtual(_out.name, _out).MergeSlot(_out);
			}
		}

		// ---- 入力スロットの要件を足し込む ----
		// 読む側は用途フラグ(SRV/UAV/DSV)だけを足す
		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				// つながっていないピンは飛ばす
				if (!_in.IsConnected()) continue;

				auto _it = m_resourceNameMap.find(_in.name);
				if (_it == m_resourceNameMap.end())
				{
					// どのパスも作っていない = Import し忘れているリソース。
					// 黙って落とすと後で実体が無くて落ちるので出しておく
					ENGINE_WARNING(
						"[RenderGraph] 入力に来ているリソースを誰も作っていません : %s <- %s",
						_upPass->GetName().c_str(), _in.name.c_str());
					continue;
				}

				m_virtualResourceVec[_it->second].MergeSlot(_in);
			}
		}

		// ---- 実サイズを決める ----
		for (VirtualResource& _res : m_virtualResourceVec)
		{
			_res.ResolveSize(m_viewportWidth, m_viewportHeight);
		}

		// ---- スロットへ割り当て結果を書き戻す ----
		WriteBackSlotHandles();

		// ---- 後続に使われているかを出力スロットへ書き戻す ----
		ResolveStoreOps();
	}

	// 出力したリソースが後続で使われるかどうかを StoreOp に落とす。
	//
	// 今は「使われ方を把握するための情報」でしかないが、
	// リソースの生存区間が分かる形にしておくと、
	// あとでエイリアシング(使い回し)を入れるときの判断材料になる
	void RenderGraph::ResolveStoreOps()
	{
		// どこかの入力に来ているリソース名を集める
		std::unordered_set<std::string> _consumedNameSet = {};
		for (const auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				if (!_in.IsConnected()) continue;
				_consumedNameSet.insert(_in.name);
			}
		}

		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			for (Slot& _out : _upPass->RefOutputSlots())
			{
				// 外部から差し込まれたリソース(このカメラの最終出力など)は
				// グラフの外で使われるので、誰も読んでいなくても残す
				const VirtualResource* _pVirtual = GetVirtualResource(_out.resourceHandle);
				const bool _isImported = (_pVirtual && _pVirtual->IsImported());

				const bool _isConsumed = _isImported || (_consumedNameSet.count(_out.name) != 0);

				_out.storeOp = _isConsumed ? EStoreOp::Store : EStoreOp::DontCare;
			}
		}
	}

	VirtualResource& RenderGraph::FindOrCreateVirtual(const std::string& a_name, const Slot& a_outputSlot)
	{
		auto _it = m_resourceNameMap.find(a_name);
		if (_it != m_resourceNameMap.end())
		{
			return m_virtualResourceVec[_it->second];
		}

		VirtualResource _res = {};
		_res.SetupFromOutputSlot(a_name, a_outputSlot);

		m_resourceNameMap[a_name] = static_cast<uint32_t>(m_virtualResourceVec.size());
		m_virtualResourceVec.push_back(std::move(_res));
		return m_virtualResourceVec.back();
	}

	// 実行時にパスが「自分のスロット -> GPUリソース」を O(1) で引けるようにする
	void RenderGraph::WriteBackSlotHandles()
	{
		auto _assign = [this](std::vector<Slot>& a_slotVec)
			{
				for (Slot& _slot : a_slotVec)
				{
					_slot.resourceHandle = FindResource(_slot.name);
				}
			};

		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			_assign(_upPass->RefInputSlots());
			_assign(_upPass->RefOutputSlots());
		}
	}

	bool RenderGraph::AllocateResources(GraphicsEngine* a_pGraphicsEngine, D3D12::Device* a_pDevice)
	{
		// Temporal は物理を2枚使うので、必要な枚数を先に数える
		size_t _requiredCount = 0;
		for (const VirtualResource& _virtual : m_virtualResourceVec)
		{
			_requiredCount += _virtual.GetPhysicalCount();
		}

		while (m_physicalResourceVec.size() < _requiredCount)
		{
			m_physicalResourceVec.push_back(std::make_unique<PhysicalResource>());
		}

		// 減ったぶんは実体を手放してから切り詰める
		for (size_t _i = _requiredCount; _i < m_physicalResourceVec.size(); ++_i)
		{
			if (m_physicalResourceVec[_i]) m_physicalResourceVec[_i]->Release();
		}
		m_physicalResourceVec.resize(_requiredCount);

		bool _isSuccess = true;
		size_t _physicalCursor = 0;
		for (size_t _i = 0; _i < m_virtualResourceVec.size(); ++_i)
		{
			VirtualResource& _virtual = m_virtualResourceVec[_i];

			// この仮想リソースが使う物理の席を確保する
			const uint32_t _sliceCount = _virtual.GetPhysicalCount();
			for (uint32_t _slice = 0; _slice < _sliceCount; ++_slice)
			{
				_virtual.SetPhysicalIndex(static_cast<uint32_t>(_physicalCursor + _slice), _slice);
			}
			if (_sliceCount == 1) _virtual.SetPhysicalIndex(static_cast<uint32_t>(_physicalCursor), 1);

			PhysicalResource* _pPhysical = m_physicalResourceVec[_physicalCursor].get();
			const size_t _selfIndex = _physicalCursor;
			_physicalCursor += _sliceCount;

			if (!_pPhysical) continue;

			if (_virtual.IsImported())
			{
				// 外部リソースは実体を作らず、参照だけもらい受ける
				D3D12::GPUResource* _pExternal = nullptr;
				for (const ImportedResource& _imported : m_importedResourceVec)
				{
					if (_imported.name != _virtual.GetName()) continue;
					_pExternal = _imported.pResource;
					break;
				}

				if (!_pExternal)
				{
					ENGINE_WARNING("[RenderGraph] 差し込まれた外部リソースが空です : %s", _virtual.GetName().c_str());
					_isSuccess = false;
					continue;
				}

				_pPhysical->Import(_pExternal);
				continue;
			}

			// Temporal のぶんも含めて、必要な枚数だけ作る
			for (uint32_t _slice = 0; _slice < _sliceCount; ++_slice)
			{
				PhysicalResource* _pSlicePhysical = m_physicalResourceVec[_selfIndex + _slice].get();
				if (!_pSlicePhysical) continue;

				// 要件が変わっていなければ前フレームの実体をそのまま使う
				if (!_pSlicePhysical->IsMatch(_virtual))
				{
					if (!_pSlicePhysical->Create(a_pDevice, _virtual))
					{
						_isSuccess = false;
						continue;
					}

					// 作り直したので、1フレーム目に中身を読まれないようクリアしておく
					if (_virtual.IsTemporal()) m_isTemporalClearPending = true;
				}

				// 生成直後のステートがフレーム入口のステートになる(バリアの起点)
				if (D3D12::GPUResource* _pRes = _pSlicePhysical->RefResource())
				{
					_virtual.SetInitialState(_pRes->GetState(), _slice);
				}
			}
			_virtual.ResetStateToInitial();
		}

		// 実体ができて入口のステートが確定したので、バリアを積み直してから実体を結びつける。
		// Compile() の時点ではまだ実体が無く、before は仮の値で積んでいる
		BuildBarriers();
		ResolveBarrierResources();

		// 出力先(RTV/DSV)を焼き込んでから、各パスへランタイムデータを作らせる。
		// この順でないとパスがディスクリプタを引けない
		ResolveDescriptors();
		CompilePasses(a_pGraphicsEngine);

		return _isSuccess;
	}

	void RenderGraph::ReleaseResources()
	{
		for (auto& _upPhysical : m_physicalResourceVec)
		{
			if (_upPhysical) _upPhysical->Release();
		}
		m_physicalResourceVec.clear();

		m_resourceNameMap.clear();
		m_virtualResourceVec.clear();

		// 仮想リソースが消えたので、それを指していたバリアも捨てる
		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			_compiledPass.preBarriers.clear();
		}
		m_endBarriers.clear();

		// スロットが持っている参照も無効化しておく
		WriteBackSlotHandles();
	}

	ResourceHandle RenderGraph::FindResource(const std::string& a_name) const
	{
		auto _it = m_resourceNameMap.find(a_name);
		if (_it == m_resourceNameMap.end()) return {};

		ResourceHandle _handle = {};
		_handle.index = _it->second;
		return _handle;
	}

	const VirtualResource* RenderGraph::GetVirtualResource(ResourceHandle a_handle) const
	{
		if (!a_handle.IsValid() || a_handle.index >= m_virtualResourceVec.size()) return nullptr;
		return &m_virtualResourceVec[a_handle.index];
	}

	VirtualResource* RenderGraph::RefVirtualResource(ResourceHandle a_handle)
	{
		if (!a_handle.IsValid() || a_handle.index >= m_virtualResourceVec.size()) return nullptr;
		return &m_virtualResourceVec[a_handle.index];
	}

	PhysicalResource* RenderGraph::RefPhysicalResource(ResourceHandle a_handle, uint32_t a_slice) const
	{
		const VirtualResource* _pVirtual = GetVirtualResource(a_handle);
		if (!_pVirtual) return nullptr;

		const uint32_t _physicalIndex = _pVirtual->GetPhysicalIndex(a_slice);
		if (_physicalIndex >= m_physicalResourceVec.size()) return nullptr;

		return m_physicalResourceVec[_physicalIndex].get();
	}

	D3D12::GPUResource* RenderGraph::RefGPUResource(ResourceHandle a_handle, uint32_t a_slice) const
	{
		PhysicalResource* _pPhysical = RefPhysicalResource(a_handle, a_slice);
		return _pPhysical ? _pPhysical->RefResource() : nullptr;
	}

	// スロットの向きと今のフレームの偶奇から、触るべき実体を決める
	D3D12::GPUResource* RenderGraph::RefGPUResource(const Slot& a_slot) const
	{
		const VirtualResource* _pVirtual = GetVirtualResource(a_slot.resourceHandle);
		if (!_pVirtual) return nullptr;

		// 出力は Current、入力は Previous。
		// それをフレームの偶奇でひっくり返したものが実際の物理になる
		const uint32_t _slice = VirtualResource::ToSlice(a_slot, _pVirtual->IsTemporal());
		const uint32_t _index = _pVirtual->IsTemporal() ? ((GetFrameParity() + _slice) & 1u) : 0u;

		return RefGPUResource(a_slot.resourceHandle, _index);
	}

	bool RenderGraph::HasTemporalResource() const
	{
		for (const VirtualResource& _virtual : m_virtualResourceVec)
		{
			if (_virtual.IsTemporal()) return true;
		}
		return false;
	}

	//======================================================================================
	//
	// 検証
	//
	//======================================================================================
	bool RenderGraph::IsWriteAccess(EAccessType a_accessType)
	{
		switch (a_accessType)
		{
		case EAccessType::RTV:
		case EAccessType::UAV:
		case EAccessType::Depth_Write:
		case EAccessType::CopyDst:
			return true;
		default:
			return false;
		}
	}

	bool RenderGraph::IsReadAccess(EAccessType a_accessType)
	{
		switch (a_accessType)
		{
		case EAccessType::SRV:
		case EAccessType::UAV:
		case EAccessType::Depth_Read:
		case EAccessType::CopySrc:
			return true;
		default:
			return false;
		}
	}

	// 繋いでよい組み合わせか。
	// UAV は読み書きの両方に出てくるので、write / read の判定は重なる
	bool RenderGraph::IsConnectable(const Slot& a_srcSlot, const Slot& a_dstSlot, std::string* a_pOutReason)
	{
		auto _fail = [a_pOutReason](const char* a_reason)
			{
				if (a_pOutReason) *a_pOutReason = a_reason;
				return false;
			};

		// 向き : 出力スロットから入力スロットへしか繋がない
		if (a_srcSlot.isIn)  return _fail("接続元が入力スロットです");
		if (!a_dstSlot.isIn) return _fail("接続先が出力スロットです");

		// リソースタイプ : テクスチャにバッファは繋げない
		if (a_srcSlot.type != a_dstSlot.type) return _fail("リソースタイプが違います(Texture/Buffer)");

		// アクセスタイプ : 作る側が書き込みになっていること
		if (!IsWriteAccess(a_srcSlot.accessType)) return _fail("接続元が書き込みアクセスではありません");

		// 「すでに描かれている絵へ描き足す」つなぎ。
		// 受け側も書き込みアクセスなら、消して描き直すのではなく重ねる意味になる。
		// スカイ・パーティクル・UI のように、前段の結果へ上描きするパスがこれで前段の後ろに並ぶ。
		//
		// 書き方(RTV/UAV/深度)が揃っている必要は無い。
		// コンピュートが描いた絵へラスタで描き足す組み合わせは普通にある
		// (スカイ(UAV) -> パーティクル(RTV) / 魚眼(UAV) -> デバッグ線(RTV))。
		// ステートはグラフが受け側のアクセスへ遷移させるので、ここで揃える必要は無い
		if (IsWriteAccess(a_dstSlot.accessType)) return true;

		if (!IsReadAccess(a_dstSlot.accessType))  return _fail("接続先が読み取りアクセスではありません");

		return true;
	}

	// 繋ぎ方だけで分かる不備をまとめて拾う。
	// GPUには触らないので、エディターから呼んで結果を出してもよい
	bool RenderGraph::Validate(std::vector<ValidationIssue>* a_pOutIssueVec) const
	{
		m_validationIssueVec.clear();

		auto _push = [this](ValidationIssue::ELevel a_level, const Engine::GUID& a_guid, std::string a_message)
			{
				ValidationIssue _issue = {};
				_issue.level = a_level;
				_issue.passGUID = a_guid;
				_issue.message = std::move(a_message);
				m_validationIssueVec.push_back(std::move(_issue));
			};

		// パスをGUIDで引けるようにしておく(接続の検証で何度も引くため)
		std::unordered_map<Engine::GUID, const Pass*> _passMap = {};
		for (const auto& _upPass : m_passes)
		{
			if (!_upPass) continue;
			_passMap.emplace(_upPass->GetGUID(), _upPass.get());
		}

		//----------------------------------------------------------------------------------
		// 接続の検証
		//----------------------------------------------------------------------------------
		// 入力スロットが何本の線を受けているか。1本を超えたら繋ぎ方がおかしい
		std::unordered_map<uint64_t, int> _inputLinkCountMap = {};
		auto _makeInputKey = [](const Engine::GUID& a_guid, uint32_t a_slotID)
			{
				return (static_cast<uint64_t>(std::hash<Engine::GUID>{}(a_guid)) << 32) ^ a_slotID;
			};

		for (const auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			auto _srcIt = _passMap.find(_srcGUID);
			if (_srcIt == _passMap.end())
			{
				_push(ValidationIssue::ELevel::Error, _srcGUID, "接続元のパスがありません");
				continue;
			}
			const Pass* _pSrc = _srcIt->second;

			for (const Connection& _connection : _connectionVec)
			{
				auto _dstIt = _passMap.find(_connection.dstPassGUID);
				if (_dstIt == _passMap.end())
				{
					_push(ValidationIssue::ELevel::Error, _srcGUID,
						_pSrc->GetName() + " : 接続先のパスがありません");
					continue;
				}
				const Pass* _pDst = _dstIt->second;

				const Slot* _pSrcSlot = _pSrc->FindOutputSlot(_connection.srcSlotID);
				if (!_pSrcSlot)
				{
					_push(ValidationIssue::ELevel::Error, _srcGUID,
						_pSrc->GetName() + " : 接続元の出力スロットがありません");
					continue;
				}

				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (!_pDstSlot)
				{
					_push(ValidationIssue::ELevel::Error, _connection.dstPassGUID,
						_pDst->GetName() + " : 接続先の入力スロットがありません");
					continue;
				}

				std::string _reason = {};
				if (!IsConnectable(*_pSrcSlot, *_pDstSlot, &_reason))
				{
					_push(ValidationIssue::ELevel::Error, _connection.dstPassGUID,
						_pSrc->GetName() + "." + _pSrcSlot->pinName + " -> " +
						_pDst->GetName() + "." + _pDstSlot->pinName + " : " + _reason);
					continue;
				}

				_inputLinkCountMap[_makeInputKey(_connection.dstPassGUID, _connection.dstSlotID)]++;
			}
		}

		//----------------------------------------------------------------------------------
		// 必須入力の未接続
		//----------------------------------------------------------------------------------
		for (const auto& _upPass : m_passes)
		{
			if (!_upPass) continue;

			for (const Slot& _in : _upPass->GetInputSlots())
			{
				const int _linkCount = _inputLinkCountMap[_makeInputKey(_upPass->GetGUID(), _in.slotID)];

				if (_linkCount > 1)
				{
					_push(ValidationIssue::ELevel::Error, _upPass->GetGUID(),
						_upPass->GetName() + "." + _in.pinName + " : 入力に線が複数入っています");
				}

				if (_linkCount != 0) continue;

				if (_in.isRequired)
				{
					_push(ValidationIssue::ELevel::Error, _upPass->GetGUID(),
						_upPass->GetName() + "." + _in.pinName + " : 必須の入力が繋がっていません");
				}
				else
				{
					_push(ValidationIssue::ELevel::Warning, _upPass->GetGUID(),
						_upPass->GetName() + "." + _in.pinName + " : 入力が繋がっていません(任意)");
				}
			}
		}

		//----------------------------------------------------------------------------------
		// 循環依存
		//
		// 実行順を決めるのと同じ辺の張り方で数えて、全部を消化できるかだけを見る。
		// ここで拾っておくと、Compile() のソートが落ちる前に理由を出せる
		//----------------------------------------------------------------------------------
		{
			std::vector<const Pass*> _nodeVec = {};
			_nodeVec.reserve(m_passes.size());
			for (const auto& _upPass : m_passes)
			{
				if (_upPass) _nodeVec.push_back(_upPass.get());
			}

			const size_t _nodeCount = _nodeVec.size();
			std::vector<std::vector<size_t>> _edgeVec(_nodeCount);
			std::vector<size_t> _indegreeVec(_nodeCount, 0);

			// 接続をたどって「dst は src に依存する」を辺にする
			for (const auto& [_srcGUID, _connectionVec] : m_connectionMap)
			{
				for (const Connection& _connection : _connectionVec)
				{
					// Temporal の接続は前フレームを読むだけなので、循環の輪には数えない
					{
						auto _dstIt = _passMap.find(_connection.dstPassGUID);
						if (_dstIt != _passMap.end())
						{
							const Slot* _pDstSlot = _dstIt->second->FindInputSlot(_connection.dstSlotID);
							if (_pDstSlot && _pDstSlot->isTemporal) continue;
						}
					}

					size_t _srcIndex = _nodeCount;
					size_t _dstIndex = _nodeCount;
					for (size_t _i = 0; _i < _nodeCount; ++_i)
					{
						if (_nodeVec[_i]->GetGUID() == _srcGUID) _srcIndex = _i;
						if (_nodeVec[_i]->GetGUID() == _connection.dstPassGUID) _dstIndex = _i;
					}
					if (_srcIndex >= _nodeCount || _dstIndex >= _nodeCount) continue;
					if (_srcIndex == _dstIndex) continue;

					_edgeVec[_srcIndex].push_back(_dstIndex);
					_indegreeVec[_dstIndex]++;
				}
			}

			std::queue<size_t> _queue = {};
			for (size_t _i = 0; _i < _nodeCount; ++_i)
			{
				if (_indegreeVec[_i] == 0) _queue.push(_i);
			}

			size_t _sortedCount = 0;
			while (!_queue.empty())
			{
				size_t _index = _queue.front();
				_queue.pop();
				++_sortedCount;

				for (size_t _next : _edgeVec[_index])
				{
					if (--_indegreeVec[_next] == 0) _queue.push(_next);
				}
			}

			// 消化しきれなかったノードが輪の中に居る
			if (_sortedCount != _nodeCount)
			{
				for (size_t _i = 0; _i < _nodeCount; ++_i)
				{
					if (_indegreeVec[_i] == 0) continue;
					_push(ValidationIssue::ELevel::Error, _nodeVec[_i]->GetGUID(),
						_nodeVec[_i]->GetName() + " : 循環依存の中にあります");
				}
			}
		}

		//----------------------------------------------------------------------------------
		// 結果
		//----------------------------------------------------------------------------------
		bool _isValid = true;
		for (const ValidationIssue& _issue : m_validationIssueVec)
		{
			if (_issue.level == ValidationIssue::ELevel::Error) _isValid = false;
		}

		if (a_pOutIssueVec) *a_pOutIssueVec = m_validationIssueVec;
		return _isValid;
	}

	//======================================================================================
	//
	// 実行
	//
	//======================================================================================

	// パスの入出力スロットから実行順を決めて m_compilePasses へ流し込む
	// 前から順に「バリアを張ってパスを回す」だけでよい、一本の配列にする
	bool RenderGraph::Compile()
	{
		m_compilePasses.clear();
		m_endBarriers.clear();

		// 繋ぎ方だけで分かる不備を先に洗い出す。
		// ここを通さずに進むと、原因が「ソート失敗」や「実体が無い」に化けて追いにくい
		if (!Validate())
		{
			for (const ValidationIssue& _issue : m_validationIssueVec)
			{
				if (_issue.level != ValidationIssue::ELevel::Error) continue;
				ENGINE_WARNING("[RenderGraph] %s", _issue.message.c_str());
			}
			ENGINE_WARNING("[RenderGraph] 検証に失敗したためコンパイルを中止しました");
			return false;
		}

		//----------------------------------------------------------------------------------
		// 線の状態をスロットへ反映する
		//
		// ApplyLinks は「接続元の出力スロット」を入力へ写す。
		// ところが描き足すパスは OnLinksResolved で自分の出力名を入力に合わせるので、
		// 1往復では数珠つなぎの末尾まで名前が伝わらない。
		//
		//   魚眼(FishEyeColor) → デバッグ線 → UI → トーンマップ
		//
		// この並びだと、UI の入力にはデバッグ線が「まだ合わせる前」の名前が入り、
		// UI だけが誰も書かない別のテクスチャへ描いてしまう
		// (画面には UI しか出ない・前フレームの中身が残る)。
		// 変化が止まるまで往復させる
		//----------------------------------------------------------------------------------
		{
			// 出力名の一覧。これが変わらなくなったら落ち着いたとみなす
			auto _snapshotNames = [this]()
				{
					std::vector<std::string> _nameVec = {};
					for (const auto& _upPass : m_passes)
					{
						if (!_upPass) continue;
						for (const Slot& _out : _upPass->GetOutputSlots())
						{
							_nameVec.push_back(_out.name);
						}
					}
					return _nameVec;
				};

			// 最悪でもパスの数だけ回れば端まで伝わる(+1 は変化なしの確認ぶん)
			const size_t _maxLoop = m_passes.size() + 1;
			std::vector<std::string> _prevNameVec = {};

			for (size_t _i = 0; _i < _maxLoop; ++_i)
			{
				ApplyLinks();

				for (auto& _upPass : m_passes)
				{
					if (!_upPass) continue;
					_upPass->OnLinksResolved();
				}

				std::vector<std::string> _nameVec = _snapshotNames();
				if (_nameVec == _prevNameVec) break;

				_prevNameVec = std::move(_nameVec);
			}

			// 最後にもう一度配り直して、入力を最新の出力名に揃える
			ApplyLinks();
		}

		// 配線から仮想リソースを組み直す。
		// 実体を作るのは AllocateResources() の仕事なので、ここではGPUに触らない
		BuildVirtualResources();

		if (m_passes.empty()) return true;

		// ---- 並べ替え対象を生ポインタで集める ----
		// 実体は m_passes(unique_ptr)が持ち続ける。並べ替えるのは参照だけ
		std::vector<Pass*> _nodes = {};
		_nodes.reserve(m_passes.size());
		for (auto& _upPass : m_passes)
		{
			if (!_upPass) continue;
			_nodes.push_back(_upPass.get());
		}
		if (_nodes.empty()) return true;

		//----------------------------------------------------------------------------------
		// 依存の洗い出し
		//
		// 「つないだ相手」だけを辺にする。リソース名で突き合わせると、
		// 同じリソースへ描き足すパスが並んだときに全員が互いの書き手になって循環する
		// (ライティング → 空 → パーティクルはどれも AfterLighting を読んで書く)。
		// 線は「この出力の後に走れ」という指示そのものなので、これだけを見ればよい
		//----------------------------------------------------------------------------------
		std::unordered_map<Engine::GUID, std::unordered_set<Engine::GUID>> _dependMap = {};
		for (const auto& [_srcGUID, _connectionVec] : m_connectionMap)
		{
			for (const Connection& _connection : _connectionVec)
			{
				// 自分自身へのつなぎは順序を持たない
				if (_connection.dstPassGUID == _srcGUID) continue;

				Pass* _pDst = FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				// Temporal は前フレームの結果を読むので、同フレームの書き手を待たない。
				// 辺にしてしまうと「History を読んで History を書く」構成が循環になる
				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (_pDstSlot && _pDstSlot->isTemporal) continue;

				_dependMap[_connection.dstPassGUID].insert(_srcGUID);
			}
		}

		// lhs の入力が rhs から来ているなら、rhs が先に走らないと入力がそろわない
		auto _isDepends = [&_dependMap](const Pass* a_pLhs, const Pass* a_pRhs)
			{
				if (!a_pLhs || !a_pRhs) return false;

				auto _it = _dependMap.find(a_pLhs->GetGUID());
				if (_it == _dependMap.end()) return false;

				return _it->second.find(a_pRhs->GetGUID()) != _it->second.end();
			};

		// ---- 依存の向きどおりに一本へ並べる ----
		std::vector<Pass*> _sortedVec = {};
		if (!Engine::Algorithm::Graph::TopologicalSort(_nodes, _sortedVec, _isDepends))
		{
			// 循環すると結果からパスが抜け落ちる。中途半端な順序で走らせない
			ENGINE_WARNING("[RenderGraph] パスの並べ替えに失敗しました : 入出力が循環しています");
			m_compilePasses.clear();
			m_endBarriers.clear();
			return false;
		}

		m_compilePasses.reserve(_sortedVec.size());
		for (Pass* _pPass : _sortedVec)
		{
			CompiledPass _compiledPass = {};
			_compiledPass.pPass = _pPass;
			m_compilePasses.push_back(std::move(_compiledPass));
		}

		// ---- リソースのステート遷移を積む ----
		BuildBarriers();

		// 各パスの Compile() はここでは呼ばない。
		// 物理リソースが決まってからでないとディスクリプタを引けないので、
		// AllocateResources() の最後で呼ぶ(仕様のコンパイル順どおり)
		return true;
	}

	//======================================================================================
	//
	// ランタイム実行
	//
	//======================================================================================
	void RenderGraph::Execute(GraphicsEngine* a_pGraphicsEngine, RenderContext* a_pRenderContext)
	{
		if (!a_pRenderContext) return;

		D3D12::GraphicsCommandList* _pCmdList = a_pRenderContext->GetCurrentCmdList();
		if (!_pCmdList) return;

		// 割り当て直後の Temporal は中身が未初期化なので、走らせる前に一度クリアする
		if (m_isTemporalClearPending)
		{
			ClearTemporalResources(a_pRenderContext);
			m_isTemporalClearPending = false;
		}

		// このフレームで使う焼き込みを選ぶ
		const uint32_t _parity = GetFrameParity();

		PassContext _context = {};
		_context.pGraph = this;
		_context.pGraphicsEngine = a_pGraphicsEngine;
		_context.pRenderContext = a_pRenderContext;
		_context.pCmdList = _pCmdList;

		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			// ---- バリア ----
			// UAVバリアは既存グラフと同じく今は張らない(状態遷移のみ)
			for (const ResourceBarrier& _barrier : _compiledPass.preBarriers)
			{
				D3D12::GPUResource* _pResource = _barrier.pResource[_parity];
				if (!_pResource) continue;
				if (_barrier.isUAVBarrier) continue;

				_pResource->Barrier(_pCmdList, _barrier.after);
			}

			// ---- レンダーターゲット切り替え ----
			// 出力を1つも持たないパス(コンピュートなど)では張り替えない。
			//
			// 空のディスクリプタ(ptr == 0)を渡すと OMSetRenderTargets の中で落ちるので、
			// 焼き込みが入っているかどうかまで見てから渡す
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& _rtvHandles = _compiledPass.rtvHandles[_parity];
			const bool _hasDSV = _compiledPass.hasDSV && (_compiledPass.dsvHandle[_parity].ptr != 0);

			if (!_rtvHandles.empty() || _hasDSV)
			{
				a_pRenderContext->SetRenderTargets(
					_rtvHandles,
					_hasDSV ? &_compiledPass.dsvHandle[_parity] : nullptr);
			}

			// ---- クリア ----
			for (size_t _index : _compiledPass.clearRtvIndices)
			{
				if (_index >= _rtvHandles.size()) continue;
				a_pRenderContext->ClearRenderTarget(_rtvHandles[_index]);
			}
			if (_compiledPass.isDepthClear && _hasDSV)
			{
				a_pRenderContext->ClearDSV(_compiledPass.dsvHandle[_parity]);
			}

			// ---- グラフが張るもの ----
			// ヒープ・ルートシグネチャ・PSO・ディスクリプタテーブル。
			// ここまで済ませておくと、パスは定数バッファと Dispatch/Draw だけでよくなる
			if (_compiledPass.pPass) ApplyStaticBindings(a_pRenderContext, _compiledPass, _parity);

			// ---- パス本体 ----
			if (_compiledPass.pPass) _compiledPass.pPass->Update(_context);
		}

		// ---- フレーム入口のステートへ戻す ----
		// 戻しておかないと、次のフレームのバリアの before が食い違う
		for (const ResourceBarrier& _barrier : m_endBarriers)
		{
			D3D12::GPUResource* _pResource = _barrier.pResource[_parity];
			if (!_pResource) continue;

			_pResource->Barrier(_pCmdList, _barrier.after);
		}

		// 次のフレームでは Temporal の役が入れ替わる
		++m_frameIndex;
	}

	// 1フレーム目は前フレームが無く、そのままだと未初期化のメモリを読むことになる。
	// 割り当て直後に一度だけ両方をクリアしておく
	void RenderGraph::ClearTemporalResources(RenderContext* a_pRenderContext)
	{
		if (!a_pRenderContext) return;

		D3D12::GraphicsCommandList* _pCmdList = a_pRenderContext->GetCurrentCmdList();
		if (!_pCmdList) return;

		auto& _heapManager = D3D12::DescriptorHeapManager::Instance();

		for (uint32_t _i = 0; _i < static_cast<uint32_t>(m_virtualResourceVec.size()); ++_i)
		{
			const VirtualResource& _virtual = m_virtualResourceVec[_i];

			if (!_virtual.IsTemporal()) continue;
			if (_virtual.IsBuffer()) continue;		// バッファは書き手が埋める前提

			ResourceHandle _handle = {};
			_handle.index = _i;

			for (uint32_t _slice = 0; _slice < _virtual.GetPhysicalCount(); ++_slice)
			{
				D3D12::GPUResource* _pResource = RefGPUResource(_handle, _slice);
				if (!_pResource) continue;

				if (_virtual.HasUsage(Resource::TextureUsage::RTV))
				{
					_pResource->Barrier(_pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
					a_pRenderContext->ClearRenderTarget(_heapManager.GetCPU(_pResource->GetRTV()));
				}
				else if (_virtual.HasUsage(Resource::TextureUsage::DSV))
				{
					_pResource->Barrier(_pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
					a_pRenderContext->ClearDSV(_heapManager.GetCPU(_pResource->GetDSV()));
				}
				else if (_virtual.HasUsage(Resource::TextureUsage::UAV))
				{
					// 履歴(TAA・デノイズ)はどれもUAVのテクスチャ。
					// ここを消さないと、割り当て直後の1枚目が前に使われていた絵のまま残り、
					// 履歴を混ぜ続けるパスがいつまでもその残骸を引きずる
					_pResource->Barrier(_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

					const float _clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					a_pRenderContext->ClearUAV(
						_heapManager.GetCPU(_pResource->GetUAV()),
						_pResource->GetResource(),
						_clearColor);
				}

				// クリアのために動かしたぶんを入口のステートへ戻す
				_pResource->Barrier(_pCmdList, _virtual.GetInitialState(_slice));
			}
		}
	}

	// 宣言から焼き込んだものを張る。
	// 実行時に文字列やスロットを引き直すことはもう無い
	void RenderGraph::ApplyStaticBindings(RenderContext* a_pRenderContext, const CompiledPass& a_compiledPass, uint32_t a_parity)
	{
		Pass* _pPass = a_compiledPass.pPass;
		if (!_pPass || !a_pRenderContext) return;

		const bool _isCompute = (_pPass->GetPipelineType() == EPassPipelineType::Compute);

		// ヒープ
		switch (_pPass->GetHeapMode())
		{
		case EPassHeapMode::Default:				a_pRenderContext->BindHeap();						break;
		case EPassHeapMode::BindlessWithSampler:	a_pRenderContext->BindCopyHeapAndSumplerBindLess();	break;
		default: break;
		}

		// ルートシグネチャ : ハンドルから実体を引くのは使う直前
		if (_pPass->GetRootSignature().IsValid())
		{
			if (_isCompute)	a_pRenderContext->SetComputeRootSignature(_pPass->GetRootSignature());
			else			a_pRenderContext->SetGraphicsRootSignature(_pPass->GetRootSignature());
		}

		// パイプラインステート
		if (_pPass->GetPSOHandle().IsValid())
		{
			if (_isCompute)	a_pRenderContext->SetComputePSO(_pPass->GetPSOHandle());
			else			a_pRenderContext->SetGraphicPSO(_pPass->GetPSOHandle());
		}

		// ルートシグネチャが無いとディスクリプタテーブルは張れない
		if (a_compiledPass.binds.empty()) return;
		if (!_pPass->GetRootSignature().IsValid()) return;

		const auto& _table = a_compiledPass.descriptorTable[a_parity];
		for (const PassBind& _bind : a_compiledPass.binds)
		{
			if (static_cast<size_t>(_bind.firstHandle) + _bind.count > _table.size()) continue;

			std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> _handles(
				_table.data() + _bind.firstHandle, _bind.count);

			switch (_bind.type)
			{
			case PassBind::EType::SrvTable:
				if (_isCompute)	a_pRenderContext->ComputeBindSRV(_bind.rootIndex, _handles);
				else			a_pRenderContext->BindSRV(_bind.rootIndex, _handles);
				break;

			case PassBind::EType::Uav:
				a_pRenderContext->BindUAV(_bind.rootIndex, _handles[0]);
				break;

			default: break;
			}
		}
	}

	// 各パスの出力先を焼き込む。
	// 実行時にスロットから引き直すと毎フレーム同じ探索を繰り返すことになる
	void RenderGraph::ResolveDescriptors()
	{
		auto& _heapManager = D3D12::DescriptorHeapManager::Instance();

		// スロットとフレームの偶奇から実体を引く。
		// Temporal でなければ偶奇に関わらず同じものになる
		auto _refResource = [this](const Slot& a_slot, uint32_t a_parity) -> D3D12::GPUResource*
			{
				const VirtualResource* _pVirtual = GetVirtualResource(a_slot.resourceHandle);
				if (!_pVirtual) return nullptr;

				const uint32_t _slice = VirtualResource::ToSlice(a_slot, _pVirtual->IsTemporal());
				const uint32_t _index = _pVirtual->IsTemporal() ? ((a_parity + _slice) & 1u) : 0u;

				return RefGPUResource(a_slot.resourceHandle, _index);
			};

		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			_compiledPass.rtvHandles[0].clear();
			_compiledPass.rtvHandles[1].clear();
			_compiledPass.clearRtvIndices.clear();
			_compiledPass.dsvHandle[0] = { 0 };
			_compiledPass.dsvHandle[1] = { 0 };
			_compiledPass.hasDSV = false;
			_compiledPass.isDepthClear = false;

			Pass* _pPass = _compiledPass.pPass;
			if (!_pPass) continue;

			// PSOを焼くのに要る出力フォーマット。偶奇で変わらないので片方だけ集める
			std::vector<DXGI_FORMAT> _rtvFormatVec = {};
			DXGI_FORMAT _dsvFormat = DXGI_FORMAT_UNKNOWN;

			for (uint32_t _parity = 0; _parity < 2; ++_parity)
			{
				// この偶奇で書き込みDSVを決めたか。
				// 偶奇をまたいで持ち越すと、片方の焼き込みが丸ごと抜ける
				bool _isDepthWriteBound = false;

				// ---- 出力側 : RTV / DSV ----
				for (const Slot& _out : _pPass->GetOutputSlots())
				{
					if (_parity == 0)
					{
						const VirtualResource* _pVirtual = GetVirtualResource(_out.resourceHandle);
						const DXGI_FORMAT _format = _pVirtual ? _pVirtual->GetFormat() : DXGI_FORMAT_UNKNOWN;

						if (_out.accessType == EAccessType::RTV)				_rtvFormatVec.push_back(_format);
						else if (_out.accessType == EAccessType::Depth_Write)	_dsvFormat = _format;
					}

					D3D12::GPUResource* _pResource = _refResource(_out, _parity);
					if (!_pResource) continue;

					if (_out.accessType == EAccessType::RTV)
					{
						_compiledPass.rtvHandles[_parity].push_back(_heapManager.GetCPU(_pResource->GetRTV()));

						// クリア対象の並びは偶奇で変わらないので、片方だけ数える
						if (_parity == 0 && _out.loadOp == ELoadOp::Clear)
						{
							_compiledPass.clearRtvIndices.push_back(_compiledPass.rtvHandles[0].size() - 1);
						}
					}
					else if (_out.accessType == EAccessType::Depth_Write)
					{
						_compiledPass.dsvHandle[_parity] = _heapManager.GetCPU(_pResource->GetDSV());
						_compiledPass.hasDSV = true;
						_isDepthWriteBound = true;

						if (_out.loadOp == ELoadOp::Clear) _compiledPass.isDepthClear = true;
					}
				}

				// ---- 入力側 : 深度を読むだけのパスは読み取り専用DSVを張る ----
				//
				// 出力側で書き込みDSVを決めていたらそちらを優先する。
				// ここで見るのは「この偶奇で決めたか」であって hasDSV ではない。
				// hasDSV は偶奇をまたいで立ちっぱなしになるので、それで打ち切ると
				// 偶数フレームぶんだけ焼いて奇数フレームぶんが 0 のまま残り、
				// 深度を読むだけのパス(パーティクル・デバッグ線)が
				// 空のDSVで OMSetRenderTargets を叩いて落ちる
				if (_isDepthWriteBound) continue;

				for (const Slot& _in : _pPass->GetInputSlots())
				{
					if (_in.accessType != EAccessType::Depth_Read) continue;

					D3D12::GPUResource* _pResource = _refResource(_in, _parity);
					if (!_pResource) continue;

					_compiledPass.dsvHandle[_parity] = _heapManager.GetCPU(_pResource->GetReadOnlyDSV());
					_compiledPass.hasDSV = true;
					break;
				}
			}

			// 出力フォーマットが決まったので、このパスのPSOを組めるようにする。
			//
			// 鍵にするのはシェーディングモデル表の名前。
			// 名乗らないパス(モデルを受け取らないパス)は表示名で構わない
			const char* _pShadingName = _pPass->GetShadingPassName();
			const std::string& _psoKeyName = _pShadingName ? std::string(_pShadingName) : _pPass->GetName();

			_pPass->RefPipelineBuilder().Init(
				_rtvFormatVec, _dsvFormat, static_cast<UINT>(Engine::String::ToHash(_psoKeyName)));

			//------------------------------------------------------------------------------
			// SRV / UAV のディスクリプタテーブル
			//
			// ルートパラメータ番号を指定したスロットだけを、番号ごとにまとめて並べる。
			// 入力を先に見るのは、宣言順がそのままテーブルの並びになるため
			//------------------------------------------------------------------------------
			_compiledPass.descriptorTable[0].clear();
			_compiledPass.descriptorTable[1].clear();
			_compiledPass.binds.clear();

			// 同じ番号のスロットを集める(番号の小さい順に張る)
			std::map<int, std::vector<const Slot*>> _rootSlotMap = {};
			for (const Slot& _in : _pPass->GetInputSlots())
			{
				if (_in.rootParamIndex < 0) continue;
				_rootSlotMap[_in.rootParamIndex].push_back(&_in);
			}
			for (const Slot& _out : _pPass->GetOutputSlots())
			{
				if (_out.rootParamIndex < 0) continue;
				_rootSlotMap[_out.rootParamIndex].push_back(&_out);
			}

			auto& _heap = D3D12::DescriptorHeapManager::Instance();
			for (const auto& [_rootIndex, _slotVec] : _rootSlotMap)
			{
				PassBind _bind = {};
				_bind.rootIndex = static_cast<UINT>(_rootIndex);
				_bind.firstHandle = static_cast<uint16_t>(_compiledPass.descriptorTable[0].size());
				_bind.count = static_cast<uint16_t>(_slotVec.size());

				// UAV は1つのルートパラメータに1本だけ。それ以外はSRVテーブルとして扱う
				const bool _isUAV = (!_slotVec.empty() && _slotVec[0]->accessType == EAccessType::UAV);
				_bind.type = _isUAV ? PassBind::EType::Uav : PassBind::EType::SrvTable;

				for (uint32_t _parity = 0; _parity < 2; ++_parity)
				{
					for (const Slot* _pSlot : _slotVec)
					{
						D3D12::GPUResource* _pResource = _refResource(*_pSlot, _parity);
						if (!_pResource)
						{
							_compiledPass.descriptorTable[_parity].push_back({ 0 });
							continue;
						}

						switch (_pSlot->accessType)
						{
						case EAccessType::UAV:
							_compiledPass.descriptorTable[_parity].push_back(_heap.GetCPU(_pResource->GetUAV()));
							break;

						// 深度を読むときも、シェーダーからは SRV として引く
						case EAccessType::SRV:
						case EAccessType::Depth_Read:
						default:
							_compiledPass.descriptorTable[_parity].push_back(_heap.GetCPU(_pResource->GetSRV()));
							break;
						}
					}
				}

				_compiledPass.binds.push_back(_bind);
			}
		}
	}

	// リソースが揃ったところで各パスのランタイムデータを構築させる
	void RenderGraph::CompilePasses(GraphicsEngine* a_pGraphicsEngine)
	{
		PassContext _context = {};
		_context.pGraph = this;
		_context.pGraphicsEngine = a_pGraphicsEngine;

		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			if (!_compiledPass.pPass) continue;
			_compiledPass.pPass->Compile(_context);
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
	void RenderGraph::BuildBarriers()
	{
		m_endBarriers.clear();

		// 各リソースのカーソルをフレーム入口へ戻してから積み始める
		for (VirtualResource& _res : m_virtualResourceVec)
		{
			_res.ResetStateToInitial();
		}

		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			BuildPassBarriers(_compiledPass);
		}

		// ---- フレームの終わりに入口のステートへ戻す ----
		// グラフは毎フレーム同じ手順で回るので、戻しておかないと
		// 次のフレームで before が食い違ってバリアが張られなくなる
		for (uint32_t _i = 0; _i < static_cast<uint32_t>(m_virtualResourceVec.size()); ++_i)
		{
			VirtualResource& _res = m_virtualResourceVec[_i];

			const uint32_t _sliceCount = _res.GetPhysicalCount();
			for (uint32_t _slice = 0; _slice < _sliceCount; ++_slice)
			{
				const D3D12_RESOURCE_STATES _before = _res.GetCurrentState(_slice);
				const D3D12_RESOURCE_STATES _after = _res.GetInitialState(_slice);
				if (_before == _after) continue;

				ResourceBarrier _barrier = {};
				_barrier.handle.index = _i;
				_barrier.slice = _slice;
				_barrier.before = _before;
				_barrier.after = _after;
				m_endBarriers.push_back(_barrier);

				_res.SetCurrentState(_after, _slice);
			}
		}
	}

	void RenderGraph::BuildPassBarriers(CompiledPass& a_compiledPass)
	{
		a_compiledPass.preBarriers.clear();

		Pass* _pPass = a_compiledPass.pPass;
		if (!_pPass) return;

		auto _pushBarrier = [&](const Slot& a_slot)
			{
				if (!a_slot.resourceHandle.IsValid()) return;
				if (a_slot.accessType == EAccessType::None) return;

				const uint32_t _resIndex = a_slot.resourceHandle.index;
				if (_resIndex >= m_virtualResourceVec.size()) return;

				VirtualResource& _res = m_virtualResourceVec[_resIndex];

				// Temporal は2枚の物理が別々の遷移をたどるので、カーソルもスライスごとに追う。
				// 出力は Current(書く側)、入力は Previous(読む側)
				const uint32_t _slice = VirtualResource::ToSlice(a_slot, _res.IsTemporal());

				const D3D12_RESOURCE_STATES _before = _res.GetCurrentState(_slice);
				const D3D12_RESOURCE_STATES _after = VirtualResource::ToResourceState(a_slot.accessType);

				const bool _isStateChanged = (_before != _after);

				// UAV -> UAV はステートが変わらないが、
				// 前の書き込みが終わるのを待たせないと結果が混ざる
				const bool _isUAVtoUAV =
					(_before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
						_after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				if (!_isStateChanged && !_isUAVtoUAV) return;

				ResourceBarrier _barrier = {};
				_barrier.handle.index = _resIndex;
				_barrier.slice = _slice;
				_barrier.before = _before;
				_barrier.after = _after;
				_barrier.isUAVBarrier = _isUAVtoUAV;
				a_compiledPass.preBarriers.push_back(_barrier);

				_res.SetCurrentState(_after, _slice);
			};

		// 読みを先に、書きを後に見る。
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

	// 積んであるバリアへ物理リソースのポインタを埋める。
	// バリアを積む時点(Compile)ではまだ実体が無いので、割り当てが済んでから解決する
	void RenderGraph::ResolveBarrierResources()
	{
		auto _resolve = [this](std::vector<ResourceBarrier>& a_barrierVec)
			{
				for (ResourceBarrier& _barrier : a_barrierVec)
				{
					const VirtualResource* _pVirtual = GetVirtualResource(_barrier.handle);
					if (!_pVirtual)
					{
						_barrier.pResource[0] = nullptr;
						_barrier.pResource[1] = nullptr;
						continue;
					}

					// Temporal はフレームの偶奇で触る物理が入れ替わるので両方を焼き込む。
					// そうでなければどちらも同じ実体になる
					for (uint32_t _parity = 0; _parity < 2; ++_parity)
					{
						const uint32_t _index = _pVirtual->IsTemporal()
							? ((_parity + _barrier.slice) & 1u)
							: 0u;

						_barrier.pResource[_parity] = RefGPUResource(_barrier.handle, _index);
					}
				}
			};

		for (CompiledPass& _compiledPass : m_compilePasses)
		{
			_resolve(_compiledPass.preBarriers);
		}
		_resolve(m_endBarriers);
	}
}
