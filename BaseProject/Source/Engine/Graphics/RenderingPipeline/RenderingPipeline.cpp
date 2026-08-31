#include "RenderingPipeline.h"
#include "StandardPipeline/StandardPipeline.h"

#include "RenderGraph/RenderGraph.h"

#include "../RenderContext/RenderContext.h"
#include "../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../Resource/Data/Shader/IO/ShaderIO.h"
#include "../GraphicEngine.h"
#include "RenderingPipelineMetaRegistry.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	//
	// Connection
	//
	//======================================================================================
	void Connection::Archive(Persistence::Archive& a_arch)
	{
		a_arch.Field("linkID", linkID);
		a_arch.GUIDField("dstPassGUID", dstPassGUID);
		a_arch.Field("srcSlotID", srcSlotID);
		a_arch.Field("dstSlotID", dstSlotID);
	}

	void Connection::EditConnection(int a_srcOutPinID, int a_dstInPinID) const
	{
		ImNodes::Link(linkID, a_srcOutPinID, a_dstInPinID);
	}

	//======================================================================================
	//
	// Pass
	//
	//======================================================================================
	void Pass::Init()
	{
		// インスタンス固有のGUID : ノードと線の紐づけに使うので必ず持たせる
		if (!m_passGUID.IsValid())
		{
			m_passGUID.Create();
		}

		// 継承先のスロット宣言 : 二重に積まないように一度空にしてから走らせる
		m_inputSlots.clear();
		m_outputSlots.clear();
		SetupSlots();
	}

	uint32_t Pass::MakeSlotID(const std::string& a_pinName)
	{
		return static_cast<uint32_t>(Engine::String::ToHash(a_pinName));
	}

	void Pass::SetInput(const std::string& a_pinName, const Slot& a_slotData)
	{
		SetInput(MakeSlotID(a_pinName), a_slotData);
	}

	void Pass::SetInput(uint32_t a_slotID, const Slot& a_slotData)
	{
		Slot* _pPin = FindInputSlot(a_slotID);
		if (!_pPin)
		{
			ENGINE_WARNING("[Pass] 入力スロットが見つかりません : %s <- %u", m_name.c_str(), a_slotID);
			return;
		}

		// テクスチャのピンにバッファをつなぐような組み方は配線ミス。
		// 黙って上書きすると後段で理由の分からない落ち方をするので、ここで出しておく
		if (_pPin->type != a_slotData.type)
		{
			ENGINE_WARNING("[Pass] スロットのタイプが合っていません : %s.%s", m_name.c_str(), _pPin->pinName.c_str());
		}

		// ピンの役割(pinName / pinID / accessType / type / isIn)は自分の持ち物なので触らない。
		// リソースの実体に関わる部分だけ、前のパスの出力から丸ごともらう
		_pPin->name = a_slotData.name;
		_pPin->format = a_slotData.format;
		_pPin->width = a_slotData.width;
		_pPin->height = a_slotData.height;
		_pPin->scale = a_slotData.scale;
		_pPin->resourceHandle = a_slotData.resourceHandle;
	}

	void Pass::ClearInput(const std::string& a_pinName)
	{
		ClearInput(MakeSlotID(a_pinName));
	}

	void Pass::ClearInput(uint32_t a_slotID)
	{
		Slot* _pPin = FindInputSlot(a_slotID);
		if (!_pPin) return;

		// つながっていた情報だけ落とす(役割は残す)
		_pPin->name.clear();
		_pPin->format = DXGI_FORMAT_UNKNOWN;
		_pPin->width = 0;
		_pPin->height = 0;
		_pPin->scale = 1.f;
		_pPin->resourceHandle = {};
	}

	const Slot& Pass::GetSlot(const std::string& a_name)
	{
		// 出力を先に見る : 「このパスが作ったリソース」を取りに来る呼ばれ方が主
		for (Slot& _out : m_outputSlots)
		{
			if (_out.name == a_name) return _out;
		}
		for (Slot& _in : m_inputSlots)
		{
			if (_in.name == a_name) return _in;
		}

		// 見つからないときに参照を返せないので、空スロットを返して呼び出し側で弾けるようにする
		static const Slot s_emptySlot = {};
		ENGINE_WARNING("[Pass] スロットが見つかりません : %s <- %s", m_name.c_str(), a_name.c_str());
		return s_emptySlot;
	}

	// 共通部分をここで処理して、固有データは継承先の Archive() へ渡す。
	// ピンIDまで写すのは、エディター上の線がピンIDで結ばれているため
	void Pass::ArchivePass(Persistence::Archive& a_arch)
	{
		a_arch.StringField("passName", m_name);
		a_arch.GUIDField("passGUID", m_passGUID);
		a_arch.GUIDField("shaderGUID", m_shaderGUID);
		a_arch.Field("editorPos", m_editorPos);
		a_arch.Field("nodeID", m_nodeID);

		// スロットは SetupSlots() が宣言済みなので、器はすでにある。
		// 中身ではなくエディター用のピンIDだけを写す(リソース名は配線から組み直される)。
		//
		// 添字ではなく slotID で突き合わせるのが要点。
		// 添字で書くと、パスがスロットを1つ足しただけで
		// 保存済みのピンIDが別のスロットへ入ってしまう
		auto _archivePinIDs = [&a_arch](const char* a_label, std::vector<Slot>& a_slotVec)
			{
				size_t _count = a_slotVec.size();
				if (!a_arch.BeginArray(a_label, _count)) return;

				for (size_t _i = 0; _i < _count; ++_i)
				{
					if (!a_arch.BeginObject(_i)) continue;

					if (a_arch.IsSaving())
					{
						if (_i < a_slotVec.size())
						{
							a_arch.Field("slotID", a_slotVec[_i].slotID);
							a_arch.Field("pinID", a_slotVec[_i].pinID);
						}
					}
					else
					{
						uint32_t _slotID = 0;
						int _pinID = 0;
						a_arch.Field("slotID", _slotID);
						a_arch.Field("pinID", _pinID);

						// 宣言が変わって無くなったスロットの分は捨てる
						for (Slot& _slot : a_slotVec)
						{
							if (_slot.slotID != _slotID) continue;
							_slot.pinID = _pinID;
							break;
						}
					}

					a_arch.EndObject();
				}
				a_arch.EndArray();
			};

		_archivePinIDs("inputSlots", m_inputSlots);
		_archivePinIDs("outputSlots", m_outputSlots);

		// 継承先固有のデータ
		Archive(a_arch);
	}

	bool Pass::HasOutputSlot(const std::string& a_name) const
	{
		for (const Slot& _out : m_outputSlots)
		{
			if (_out.name == a_name) return true;
		}
		return false;
	}

	Slot* Pass::FindInputSlot(uint32_t a_slotID)
	{
		for (Slot& _in : m_inputSlots)
		{
			if (_in.slotID == a_slotID) return &_in;
		}
		return nullptr;
	}

	const Slot* Pass::FindInputSlot(uint32_t a_slotID) const
	{
		for (const Slot& _in : m_inputSlots)
		{
			if (_in.slotID == a_slotID) return &_in;
		}
		return nullptr;
	}

	Slot* Pass::FindOutputSlot(uint32_t a_slotID)
	{
		for (Slot& _out : m_outputSlots)
		{
			if (_out.slotID == a_slotID) return &_out;
		}
		return nullptr;
	}

	const Slot* Pass::FindOutputSlot(uint32_t a_slotID) const
	{
		for (const Slot& _out : m_outputSlots)
		{
			if (_out.slotID == a_slotID) return &_out;
		}
		return nullptr;
	}

	Slot* Pass::FindInputPin(const std::string& a_pinName)		{ return FindInputSlot(MakeSlotID(a_pinName)); }
	Slot* Pass::FindOutputPin(const std::string& a_pinName)		{ return FindOutputSlot(MakeSlotID(a_pinName)); }
	const Slot* Pass::FindOutputPin(const std::string& a_pinName) const { return FindOutputSlot(MakeSlotID(a_pinName)); }

	Slot* Pass::FindSlotByPinID(int a_pinID, bool* a_pOutIsInput)
	{
		for (Slot& _in : m_inputSlots)
		{
			if (_in.pinID != a_pinID) continue;
			if (a_pOutIsInput) *a_pOutIsInput = true;
			return &_in;
		}
		for (Slot& _out : m_outputSlots)
		{
			if (_out.pinID != a_pinID) continue;
			if (a_pOutIsInput) *a_pOutIsInput = false;
			return &_out;
		}
		return nullptr;
	}

	void Pass::EnsureEditorIDs(const std::function<int()>& a_generateID)
	{
		if (!a_generateID) return;

		// すでに振られているIDは触らない。
		// 線はピンIDで結ばれているので、振り直すと保存済みのつなぎが全部外れる
		if (m_nodeID == 0) m_nodeID = a_generateID();

		for (Slot& _in : m_inputSlots)
		{
			if (_in.pinID == 0) _in.pinID = a_generateID();
		}
		for (Slot& _out : m_outputSlots)
		{
			if (_out.pinID == 0) _out.pinID = a_generateID();
		}
	}

	void Pass::DispatchFullScreen(const PassContext& a_context) const
	{
		if (!a_context.pRenderContext || !a_context.pGraph) return;

		// このパイプラインの描画解像度で回す(カメラごとに違うことがある)
		const UINT _width = static_cast<UINT>(a_context.pGraph->GetViewportWidth());
		const UINT _height = a_context.pGraph->GetViewportHeight();
		if (_width == 0 || _height == 0) return;

		a_context.pRenderContext->Dispatch((_width + 7) / 8, (_height + 7) / 8, 1);
	}

	void Pass::DispatchForSlot(const PassContext& a_context, const Slot& a_slot) const
	{
		if (!a_context.pRenderContext || !a_context.pGraph) return;

		const VirtualResource* _pRes = a_context.pGraph->GetVirtualResource(a_slot.resourceHandle);
		if (!_pRes) return;

		const UINT _width = static_cast<UINT>(_pRes->GetWidth());
		const UINT _height = _pRes->GetHeight();
		if (_width == 0 || _height == 0) return;

		a_context.pRenderContext->Dispatch((_width + 7) / 8, (_height + 7) / 8, 1);
	}

	// 頂点+ピクセルシェーダーからルートシグネチャとPSOを作る。
	// 出力フォーマットはこのパスのRTVスロットから引くので、
	// リソースが決まった後(Compile)でないと正しく作れない
	bool Pass::SetupRasterShader(
		const PassContext& a_context,
		const std::string& a_vsPath,
		const std::string& a_psPath,
		const D3D12_INPUT_LAYOUT_DESC& a_inputLayout,
		const std::string& a_psoName,
		const std::function<void(D3D12::GraphicsPipelineDesc&)>& a_configure,
		EPassHeapMode a_heapMode,
		Handle<ID3D12PipelineState>* a_pOutPSOHandle)
	{
		if (!a_context.pGraphicsEngine || !a_context.pGraph) return false;

		auto* _pPSOManager = a_context.pGraphicsEngine->RefPipelineStateManager();
		if (!_pPSOManager) return false;

		D3D12::GraphicsPipelineDesc _desc = {};
		_desc.SetName(a_psoName);
		_desc.SetInputLayout(a_inputLayout);

		// 頂点シェーダー : ルートシグネチャもこのブロブから起こす
		auto _vsHandle = Resource::ShaderIO::Request(a_vsPath);
		auto* _pVS = Resource::ResourceManager::Instance().Ref(_vsHandle);
		if (!_pVS || !_pVS->Get())
		{
			ENGINE_WARNING("[Pass] 頂点シェーダーが読めません : %s", a_vsPath.c_str());
			return false;
		}
		_desc.SetVS(_pVS->GetByteCode());

		// ピクセルシェーダー : 深度だけ書くパスでは空でよい
		if (!a_psPath.empty())
		{
			auto _psHandle = Resource::ShaderIO::Request(a_psPath);
			if (auto* _pPS = Resource::ResourceManager::Instance().Ref(_psHandle))
			{
				_desc.SetPS(_pPS->GetByteCode());
			}
		}

		m_rootSigHandle = _pPSOManager->Request(_pVS->Get());
		if (!m_rootSigHandle.IsValid())
		{
			ENGINE_WARNING("[Pass] ルートシグネチャが作れません : %s", a_vsPath.c_str());
			return false;
		}
		_desc.SetRootSignature(_pPSOManager->GetRootSignature(m_rootSigHandle));

		// 出力フォーマット : 宣言したRTVスロットの並び順がそのままレンダーターゲットの順になる
		for (const Slot& _out : m_outputSlots)
		{
			if (_out.accessType != EAccessType::RTV) continue;

			const VirtualResource* _pRes = a_context.pGraph->GetVirtualResource(_out.resourceHandle);
			if (!_pRes) continue;

			_desc.AddRenderTargetFormat(_pRes->GetFormat());
		}

		// 深度・ブレンド・トポロジなどパス固有の設定
		if (a_configure) a_configure(_desc);

		const auto _psoHandle = _pPSOManager->RequestHandle(_desc);

		// 受け取り先を渡されたらそちらへ。
		// m_psoHandle は無効のままにしておくと、グラフはPSOを張らずパスに任せる
		if (a_pOutPSOHandle)	*a_pOutPSOHandle = _psoHandle;
		else					m_psoHandle = _psoHandle;

		m_pipelineType = EPassPipelineType::Graphics;
		m_heapMode = a_heapMode;
		return true;
	}

	// コンピュートシェーダーからルートシグネチャとPSOを作る。
	// 宣言だけで済むように、グラフが実行前に張れる形で控えておく
	bool Pass::SetupComputeShader(
		const PassContext& a_context,
		const std::string& a_csPath,
		const std::string& a_psoName,
		EPassHeapMode a_heapMode)
	{
		if (!a_context.pGraphicsEngine) return false;

		auto* _pPSOManager = a_context.pGraphicsEngine->RefPipelineStateManager();
		if (!_pPSOManager) return false;

		// シェーダー
		auto _csHandle = Resource::ShaderIO::Request(a_csPath);
		auto* _pShader = Resource::ResourceManager::Instance().Ref(_csHandle);
		if (!_pShader || !_pShader->Get())
		{
			ENGINE_WARNING("[Pass] コンピュートシェーダーが読めません : %s", a_csPath.c_str());
			return false;
		}

		// ルートシグネチャ : シェーダーに埋まっている定義から起こす
		m_rootSigHandle = _pPSOManager->Request(_pShader->Get());
		if (!m_rootSigHandle.IsValid())
		{
			ENGINE_WARNING("[Pass] ルートシグネチャが作れません : %s", a_csPath.c_str());
			return false;
		}

		// PSO
		D3D12::ComputePipelineDesc _desc = {};
		_desc.SetName(a_psoName);
		_desc.desc.CS.pShaderBytecode = _pShader->Get()->GetBufferPointer();
		_desc.desc.CS.BytecodeLength = _pShader->Get()->GetBufferSize();
		_desc.SetRootSignature(_pPSOManager->GetRootSignature(m_rootSigHandle));

		m_psoHandle = _pPSOManager->RequestHandle(_desc);

		m_pipelineType = EPassPipelineType::Compute;
		m_heapMode = a_heapMode;
		return true;
	}

	// 入力に来たリソースをそのまま出力先にする。
	// 上書きではなく描き足すパス(スカイ・パーティクル・UIなど)がこれを通す
	void Pass::FollowInputToOutput(const std::string& a_inPinName, const std::string& a_outPinName)
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID(a_outPinName));
		if (!_pOut) return;

		const Slot* _pIn = FindInputSlot(MakeSlotID(a_inPinName));
		if (!_pIn || !_pIn->IsConnected())
		{
			// 描き足す先が繋がっていない = このパスが最初に描く。
			//
			// Load のままにすると前フレームの中身がそのまま残り、
			// 「UIパスと出口だけのパイプライン」でもUIの後ろに前フレームの絵が出てしまう。
			// 描き足す相手が居ないのだから、まず消してから描く
			_pOut->loadOp = ELoadOp::Clear;
			return;
		}

		// 実体に関わるところだけをもらう。
		// アクセスの種類(RTV/UAV)は自分の描き方なので触らない
		_pOut->name = _pIn->name;
		_pOut->format = _pIn->format;
		_pOut->width = _pIn->width;
		_pOut->height = _pIn->height;
		_pOut->scale = _pIn->scale;

		// 前段の絵を消さない
		_pOut->loadOp = ELoadOp::Load;
	}

	Slot& Pass::DeclareInput(const std::string& a_pinName, EAccessType a_accessType, EPassSlotType a_type, bool a_isRequired, int a_rootParamIndex, bool a_isTemporal)
	{
		Slot _slot = {};
		_slot.isIn = true;
		_slot.pinName = a_pinName;
		_slot.slotID = MakeSlotID(a_pinName);
		_slot.isRequired = a_isRequired;
		_slot.rootParamIndex = a_rootParamIndex;
		_slot.type = a_type;
		_slot.accessType = a_accessType;

		// 前フレームを読むピンかどうかはピン自身の役割。
		// つないだ相手からもらうものではないので、線を張り直しても変わらない
		_slot.isTemporal = a_isTemporal;

		// name(リソース名)はつながって初めて埋まる
		m_inputSlots.push_back(std::move(_slot));
		return m_inputSlots.back();
	}

	Slot& Pass::DeclareOutput(
		const std::string& a_pinName,
		const std::string& a_resourceName,
		DXGI_FORMAT a_format,
		EAccessType a_accessType,
		EPassSlotType a_type,
		bool a_isTemporal,
		int a_rootParamIndex)
	{
		Slot _slot = {};
		_slot.isIn = false;
		_slot.isTemporal = a_isTemporal;
		_slot.rootParamIndex = a_rootParamIndex;
		_slot.pinName = a_pinName;
		_slot.slotID = MakeSlotID(a_pinName);
		_slot.isRequired = false;		// 出力は自分が作るので、繋がっていなくても成立する
		_slot.name = a_resourceName;		// このパスが作るリソースなので最初から名前を持つ
		_slot.type = a_type;
		_slot.accessType = a_accessType;
		_slot.format = a_format;

		m_outputSlots.push_back(std::move(_slot));
		return m_outputSlots.back();
	}

	//======================================================================================
	//
	// RenderingPipelineAsset
	//
	// パス・つなぎ・実行順はすべて RenderGraph の持ち物。
	// ここは「グラフを1つ抱えて、それを編集するUIを出す」役に徹する
	//
	//======================================================================================

	// RenderGraph を unique_ptr で持つので、生成/破棄はここ(完全型が見える場所)に置く
	RenderingPipelineAsset::RenderingPipelineAsset()
		: m_upRenderGraph(std::make_unique<RenderGraph>())
	{}

	RenderingPipelineAsset::~RenderingPipelineAsset()
	{
		DestroyContext();
	}

	// ImNodes のエディターコンテキストは生ポインタで持っているので、
	// 移した側を必ず nullptr にする(両方が同じコンテキストを解放しないように)
	RenderingPipelineAsset::RenderingPipelineAsset(RenderingPipelineAsset&& a_other) noexcept
		: m_name(std::move(a_other.m_name))
		, m_pMetaRegistry(a_other.m_pMetaRegistry)
		, m_upRenderGraph(std::move(a_other.m_upRenderGraph))
		, m_context(a_other.m_context)
		, m_applyPositions(a_other.m_applyPositions)
		, m_pendingDeletePass(a_other.m_pendingDeletePass)
	{
		a_other.m_context = nullptr;
		a_other.m_pMetaRegistry = nullptr;
	}

	RenderingPipelineAsset& RenderingPipelineAsset::operator=(RenderingPipelineAsset&& a_other) noexcept
	{
		if (this == &a_other) return *this;

		DestroyContext();

		m_name = std::move(a_other.m_name);
		m_pMetaRegistry = a_other.m_pMetaRegistry;
		m_upRenderGraph = std::move(a_other.m_upRenderGraph);
		m_context = a_other.m_context;
		m_applyPositions = a_other.m_applyPositions;
		m_pendingDeletePass = a_other.m_pendingDeletePass;

		a_other.m_context = nullptr;
		a_other.m_pMetaRegistry = nullptr;
		return *this;
	}

	// 保存・読込はグラフ側が持っている(パスと配線はあちらの持ち物)
	void RenderingPipelineAsset::Archive(Persistence::Archive& a_arch)
	{
		if (!m_upRenderGraph) return;
		if (!m_pMetaRegistry)
		{
			ENGINE_WARNING("[RenderingPipelineAsset] PassMetaRegistry が未設定のため読み書きできません");
			return;
		}

		a_arch.StringField("pipelineName", m_name);
		m_upRenderGraph->Archive(a_arch, *m_pMetaRegistry);

		// ロード直後は ImNodes へノード座標を流し込む
		if (a_arch.IsLoading())
		{
			// 古いデータには出口が入っていないので、ここで必ず用意する
			EnsureFinalPass();
			RequestApplyLoadPositions();
		}
	}

	// グラフの出口が無ければ足す。
	// 常駐させることで、パス側は「自分が画面に出るかどうか」を気にしなくてよくなる
	void RenderingPipelineAsset::EnsureFinalPass()
	{
		if (!m_pMetaRegistry || !m_upRenderGraph) return;

		const ID<Pass> _finalTypeID = m_pMetaRegistry->GetFinalPassTypeID();
		if (!_finalTypeID.IsValid()) return;

		// すでに居れば何もしない
		for (const auto& _upPass : m_upRenderGraph->GetPasses())
		{
			if (_upPass && _upPass->GetTypeID() == _finalTypeID) return;
		}

		Pass* _pPass = m_upRenderGraph->AddPass(*m_pMetaRegistry, _finalTypeID);
		if (!_pPass) return;

		// 出口なので既定では右のほうへ置いておく
		_pPass->SetEditorPos(Math::Vector2(520.0f, 40.0f));
		SetDirty();
	}

	bool RenderingPipelineAsset::IsFinalPass(const Pass& a_pass) const
	{
		if (!m_pMetaRegistry) return false;
		return m_pMetaRegistry->IsFinalPassType(a_pass.GetTypeID());
	}

	void RenderingPipelineAsset::Compile()
	{
		if (!m_upRenderGraph) return;

		// 失敗しても Dirty は下ろす。
		// 直さないまま押し続けても同じ結果にしかならないので、
		// 「押した = 一度は試した」で区切る
		m_upRenderGraph->Compile();
		m_isDirty = false;
	}

	void RenderingPipelineAsset::Save(const std::string& a_baseFilePath)
	{
		// 保存の前に、ImNodes 上で動かしたノード座標を書き戻す
		SyncPositions();

		auto _fileDir = Engine::File::GetDirFromPath(a_baseFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_baseFilePath);

		// 読み込みと同じくJSON固定(理由は RenderingPipelineAssetIO::LoadFromFile を参照)
		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _fileDir, _fileName, kExtension,
			Persistence::Archive::ArchiveFormat::Json);
		Archive(_arch);
	}

	//======================================================================================
	//
	// エディター
	//
	//======================================================================================
	void RenderingPipelineAsset::DrawEditor()
	{
		if (!m_upRenderGraph) return;

		// 出口は常駐。レジストリが後から入った場合もここで揃う
		EnsureFinalPass();

		// インスタンスごとにポップアップ/ウィジェットIDを分離
		ImGui::PushID(this);

		DrawAddPass();
		ImGui::SameLine();

		// 構成が変わっていなければ押しても結果は同じなので、そのときは通さない
		if (ImGui::Button("Compile") && m_isDirty)
		{
			Compile();
		}
		ImGui::SameLine();
		if (m_isDirty)	ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Modified");
		else			ImGui::TextDisabled("Compiled");

		ImGui::SameLine();
		ImGui::TextDisabled("| Pass : %d", static_cast<int>(m_upRenderGraph->GetPasses().size()));

		// 既存の描画と同じ流れを一式組む。
		// 今入っているものは全部捨てるので、押し間違いが痛い分だけ確認を挟む
		ImGui::SameLine();
		if (ImGui::Button("Standard")) ImGui::OpenPopup("StandardPipelinePopup");

		if (ImGui::BeginPopup("StandardPipelinePopup"))
		{
			ImGui::TextDisabled("今のパスと配線をすべて捨てて組み直します");
			if (Engine::Editor::EditorHelper::CreateButton("Build") && m_pMetaRegistry)
			{
				BuildStandardPipeline(*this, *m_pMetaRegistry);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::Separator();

		DrawValidation();

		DrawSelectedPassDetail();

		DrawNodeEditor();

		// 線の生成は EndNodeEditor の後でないと拾えない
		HandleCreateLink();

		ImGui::PopID();
	}

	// 繋ぎ方の不備をその場で見せる。
	// ログにしか出ないと、パスが増えたときにどのノードが原因か追えなくなる
	void RenderingPipelineAsset::DrawValidation()
	{
		if (!m_upRenderGraph) return;

		std::vector<ValidationIssue> _issueVec = {};
		const bool _isValid = m_upRenderGraph->Validate(&_issueVec);

		if (_issueVec.empty())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Validation : OK");
			return;
		}

		// エラーが1つでもあるとコンパイルは通らない
		if (_isValid)	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Validation : %d warning(s)", static_cast<int>(_issueVec.size()));
		else			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Validation : NG");

		if (ImGui::TreeNodeEx("Issues", _isValid ? 0 : ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const ValidationIssue& _issue : _issueVec)
			{
				const bool _isError = (_issue.level == ValidationIssue::ELevel::Error);
				const ImVec4 _color = _isError
					? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
					: ImVec4(1.0f, 0.8f, 0.3f, 1.0f);

				ImGui::TextColored(_color, "%s : %s", _isError ? "Error" : "Warn", _issue.message.c_str());

				// クリックでそのノードを選ぶ
				if (!ImGui::IsItemClicked()) continue;
				if (!_issue.passGUID.IsValid()) continue;

				Pass* _pPass = m_upRenderGraph->FindPass(_issue.passGUID);
				if (!_pPass) continue;

				ImNodes::EditorContextSet(m_context);
				ImNodes::ClearNodeSelection();
				ImNodes::SelectNode(_pPass->GetNodeID());
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
	}

	void RenderingPipelineAsset::DrawNodeEditor()
	{
		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		// ロード後の初回Drawでノード座標を反映
		// (メインスレッド・コンテキスト有効状態でないと ImNodes を触れない)
		if (m_applyPositions)
		{
			for (auto& _upPass : m_upRenderGraph->GetPasses())
			{
				if (!_upPass) continue;
				const Math::Vector2& _pos = _upPass->GetEditorPos();
				ImNodes::SetNodeEditorSpacePos(_upPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
			}
			m_applyPositions = false;
		}

		ImNodes::BeginNodeEditor();

		const auto& _connectionMap = m_upRenderGraph->GetConnections();
		for (auto& _upPass : m_upRenderGraph->GetPasses())
		{
			if (!_upPass) continue;

			DrawNode(*_upPass);

			// このパスから伸びる線を描く
			auto _it = _connectionMap.find(_upPass->GetGUID());
			if (_it == _connectionMap.end()) continue;

			for (const Connection& _connection : _it->second)
			{
				const Slot* _pSrcSlot = _upPass->FindOutputSlot(_connection.srcSlotID);
				if (!_pSrcSlot) continue;

				Pass* _pDst = m_upRenderGraph->FindPass(_connection.dstPassGUID);
				if (!_pDst) continue;

				const Slot* _pDstSlot = _pDst->FindInputSlot(_connection.dstSlotID);
				if (!_pDstSlot) continue;

				_connection.EditConnection(_pSrcSlot->pinID, _pDstSlot->pinID);
			}
		}

		ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
		ImNodes::EndNodeEditor();

		// 選択中のノード/線を Delete キーで削除
		HandleDeleteSelection();

		// ノード内「Delete Pass」ボタンで予約された削除を実行
		// (パス配列を回している最中に消すとイテレータが壊れる)
		if (m_pendingDeletePass.IsValid())
		{
			m_upRenderGraph->RemovePass(m_pendingDeletePass);
			m_pendingDeletePass = {};
			SetDirty();
		}
	}

	void RenderingPipelineAsset::DrawNode(Pass& a_pass)
	{
		ImNodes::BeginNode(a_pass.GetNodeID());

		Editor::EditorHelper::DrawNodeTitleBar(a_pass.GetName());

		// 入力ピン : つながっていればリソース名まで出す
		for (const Slot& _in : a_pass.GetInputSlots())
		{
			ImNodes::BeginInputAttribute(_in.pinID);
			if (_in.IsConnected())
			{
				ImGui::Text("%s : %s", _in.pinName.c_str(), _in.name.c_str());
			}
			else
			{
				ImGui::TextDisabled("%s", _in.pinName.c_str());
			}
			ImNodes::EndInputAttribute();
		}

		// 出力ピン : 作るリソース名は宣言時に決まっている
		for (const Slot& _out : a_pass.GetOutputSlots())
		{
			ImNodes::BeginOutputAttribute(_out.pinID);
			ImGui::Text("%s : %s", _out.pinName.c_str(), _out.name.c_str());
			ImNodes::EndOutputAttribute();
		}

		// パス固有のノード内UI
		a_pass.EditNode();

		// 出口は常駐なので消させない
		if (!IsFinalPass(a_pass))
		{
			// 削除は反復中に消すとイテレータが壊れるので予約だけする
			ImGui::Spacing();
			if (Editor::EditorHelper::DeleteSmallButton("Delete Pass"))
			{
				m_pendingDeletePass = a_pass.GetGUID();
			}
		}

		ImNodes::EndNode();
	}

	void RenderingPipelineAsset::DrawAddPass()
	{
		if (!m_pMetaRegistry)
		{
			ImGui::TextDisabled("No PassMetaRegistry");
			return;
		}

		if (Engine::Editor::EditorHelper::CreateButton("AddPass"))
		{
			ImGui::OpenPopup("AddPassPopup");
		}
		if (ImGui::BeginPopup("AddPassPopup"))
		{
			ImGui::TextDisabled("Select Pass");
			ImGui::Separator();

			const auto& _allMeta = m_pMetaRegistry->GetAllMeta();
			if (_allMeta.empty())
			{
				ImGui::TextDisabled("No registered pass");
			}
			else
			{
				// 検索用
				const std::string& _search = Editor::EditorHelper::DrawSearchBox();

				// クラス名順に並べて表示
				std::vector<ID<Pass>> _ids;
				_ids.reserve(_allMeta.size());
				for (const auto& [_id, _meta] : _allMeta) _ids.push_back(_id);
				std::sort(_ids.begin(), _ids.end(),
					[&_allMeta](ID<Pass> a_lhs, ID<Pass> a_rhs)
					{return _allMeta.at(a_lhs).name < _allMeta.at(a_rhs).name;}
				);

				// 選択欄
				for (ID<Pass> _id : _ids)
				{
					const auto& _meta = _allMeta.at(_id);

					// 出口は常駐なので、手で足せないようにする
					if (_meta.isFinalPass) continue;

					if (!Editor::EditorHelper::IsMatchSearch(_search, _meta.name)) continue;

					std::string _label = _meta.name + "##addobj" + std::to_string(_id.value);
					if (ImGui::Selectable(_label.c_str()))
					{
						AddPassFromEditor(_id);
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}
	}

	// パスの生成そのものは RenderGraph の仕事。
	// ここは、決まったノード座標を ImNodes 側へ反映するところだけを受け持つ
	void RenderingPipelineAsset::AddPassFromEditor(ID<Pass> a_typeID)
	{
		if (!m_pMetaRegistry || !m_upRenderGraph) return;

		Pass* _pPass = m_upRenderGraph->AddPass(*m_pMetaRegistry, a_typeID);
		if (!_pPass) return;

		SetDirty();

		// まだ描いていないノードでも ImNodes 側は FindOrCreate なので座標だけ先に置ける。
		// 逆に ImNodes::SelectNode は描画前だと ObjectPool に無くて assert するので呼ばないこと
		if (m_context)
		{
			ImNodes::EditorContextSet(m_context);
			const Math::Vector2& _pos = _pPass->GetEditorPos();
			ImNodes::SetNodeEditorSpacePos(_pPass->GetNodeID(), ImVec2(_pos.x, _pos.y));
		}
	}

	// 選択中のパスの詳細(パス固有の設定)を出す。
	// ノードの中に全部詰めると線が見えなくなるので、細かい設定はこちら側で編集する
	void RenderingPipelineAsset::DrawSelectedPassDetail()
	{
		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		if (ImNodes::NumSelectedNodes() != 1) return;

		int _nodeID = 0;
		ImNodes::GetSelectedNodes(&_nodeID);

		Pass* _pPass = m_upRenderGraph->FindPassByNodeID(_nodeID);
		if (!_pPass) return;

		if (ImGui::CollapsingHeader("Selected Pass", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(_nodeID);
			ImGui::Text("%s", _pPass->GetName().c_str());
			ImGui::Separator();

			// パス固有の設定(フォーマットやスケールなど)もリソースの要件を変えるので、
			// 触られたら Dirty にする。
			// 値を確定したところ(ドラッグを離した等)で1回だけ立つ
			// パラメータだけなら組み直さず、カメラ側へ値を写すだけで済ませる
			switch (_pPass->EditUpdate())
			{
			case EPassEditResult::Structure:	SetDirty();			break;
			case EPassEditResult::Param:		++m_paramVersion;	break;
			default: break;
			}

			ImGui::PopID();
		}
		ImGui::Separator();
	}

	void RenderingPipelineAsset::HandleCreateLink()
	{
		int _startAttr = 0;
		int _endAttr = 0;
		if (!ImNodes::IsLinkCreated(&_startAttr, &_endAttr)) return;

		// どちらが出力側で引かれたか分からないので、両端をそれぞれ判定する
		Pass* _pSrc = nullptr;
		Slot* _pSrcSlot = nullptr;
		Pass* _pDst = nullptr;
		Slot* _pDstSlot = nullptr;

		const int _attrs[2] = { _startAttr, _endAttr };
		for (int _attr : _attrs)
		{
			Slot* _pSlot = nullptr;
			bool _isInput = false;
			Pass* _pPass = m_upRenderGraph->FindPassByPinID(_attr, &_pSlot, &_isInput);
			if (!_pPass || !_pSlot) continue;

			if (_isInput)
			{
				_pDst = _pPass;
				_pDstSlot = _pSlot;
			}
			else
			{
				_pSrc = _pPass;
				_pSrcSlot = _pSlot;
			}
		}

		// 入力どうし・出力どうしをつないだ場合はここで弾かれる
		if (!_pSrc || !_pSrcSlot || !_pDst || !_pDstSlot) return;

		// 自分自身へのつなぎや、入力スロットの張り替えは RenderGraph 側が面倒を見る
		if (m_upRenderGraph->Link(
			_pSrc->GetGUID(), _pSrcSlot->slotID,
			_pDst->GetGUID(), _pDstSlot->slotID))
		{
			SetDirty();
		}
	}

	void RenderingPipelineAsset::HandleDeleteSelection()
	{
		if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false)) return;

		// 選択中の線を削除
		int _numLinks = ImNodes::NumSelectedLinks();
		if (_numLinks > 0)
		{
			std::vector<int> _links(_numLinks);
			ImNodes::GetSelectedLinks(_links.data());
			for (int _linkID : _links)
			{
				m_upRenderGraph->RemoveLink(_linkID);
			}
			ImNodes::ClearLinkSelection();
			m_upRenderGraph->ApplyLinks();
			SetDirty();
		}

		// 選択中のパスを削除(出入りする線も RemovePass 側で巻き添え削除)
		int _numNodes = ImNodes::NumSelectedNodes();
		if (_numNodes > 0)
		{
			std::vector<int> _nodes(_numNodes);
			ImNodes::GetSelectedNodes(_nodes.data());

			// nodeID -> GUID をここで引いておく(消しながら引くと参照が切れる)
			std::vector<Engine::GUID> _targets;
			_targets.reserve(_nodes.size());
			for (int _nodeID : _nodes)
			{
				Pass* _pPass = m_upRenderGraph->FindPassByNodeID(_nodeID);
				if (!_pPass) continue;

				// 出口は常駐なので Delete キーでも消さない
				if (IsFinalPass(*_pPass)) continue;

				_targets.push_back(_pPass->GetGUID());
			}
			for (const Engine::GUID& _guid : _targets)
			{
				m_upRenderGraph->RemovePass(_guid);
			}
			ImNodes::ClearNodeSelection();
			SetDirty();
		}
	}

	void RenderingPipelineAsset::EnsureContext()
	{
		if (!m_context)
		{
			m_context = ImNodes::EditorContextCreate();
		}
	}

	void RenderingPipelineAsset::DestroyContext()
	{
		if (m_context)
		{
			ImNodes::EditorContextFree(m_context);
			m_context = nullptr;
		}
	}

	void RenderingPipelineAsset::SyncPositions()
	{
		if (!m_upRenderGraph) return;

		EnsureContext();
		ImNodes::EditorContextSet(m_context);

		for (auto& _upPass : m_upRenderGraph->GetPasses())
		{
			if (!_upPass) continue;

			ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_upPass->GetNodeID());
			_upPass->SetEditorPos(Math::Vector2(_pos.x, _pos.y));
		}
	}
}
