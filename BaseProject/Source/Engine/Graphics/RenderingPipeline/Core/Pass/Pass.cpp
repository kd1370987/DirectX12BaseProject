#include "Pass.h"

#include "../../RenderGraph/RenderGraph.h"
#include "../../RenderGraph/Resource/VirtualResource/VirtualResource.h"
#include "../../../RenderContext/RenderContext.h"

#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../../Resource/Data/Shader/IO/ShaderIO.h"
#include "../../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{

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

	//======================================================================================
	// 仮想リソースの識別子
	//======================================================================================
	ResourceID ResourceID::FromOutputSlot(const Engine::GUID& a_passGUID, uint32_t a_slotID)
	{
		ResourceID _id = {};
		_id.passGUID = a_passGUID;

		// 万一ハッシュが無効値と同じになっても「無効」に化けないようずらしておく
		_id.slotID = (a_slotID == INVALID_SLOT_ID) ? (a_slotID - 1) : a_slotID;
		return _id;
	}

	ResourceID ResourceID::FromImportName(const std::string& a_name)
	{
		// 作り手のパスが居ないので GUID は空のまま。
		// パスの出力は必ず有効なGUIDを持つので、こちらとぶつかることはない
		ResourceID _id = {};

		const uint32_t _hash = Pass::MakeSlotID(a_name);
		_id.slotID = (_hash == INVALID_SLOT_ID) ? (_hash - 1) : _hash;
		return _id;
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
		_pPin->resourceID = a_slotData.resourceID;
		_pPin->name = a_slotData.name;
		_pPin->format = a_slotData.format;
		_pPin->width = a_slotData.width;
		_pPin->height = a_slotData.height;
		_pPin->scale = a_slotData.scale;
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
		_pPin->resourceID = {};
		_pPin->name.clear();
		_pPin->format = DXGI_FORMAT_UNKNOWN;
		_pPin->width = 0;
		_pPin->height = 0;
		_pPin->scale = 1.f;
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

		const VirtualResource* _pRes = a_context.pGraph->GetVirtualResource(a_slot.resourceID);
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

			const VirtualResource* _pRes = a_context.pGraph->GetVirtualResource(_out.resourceID);
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

	// 入力に来たリソースを、そのまま自分の出力先として引き継ぐ。
	//
	// リソースの同一性は識別子で決まるので、出力名を前段と揃えるだけでは合流しない。
	// 「前段と同じリソースへ描く」パスは必ずここを通すこと
	bool Pass::AliasOutputToInput(const std::string& a_inPinName, const std::string& a_outPinName)
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID(a_outPinName));
		if (!_pOut) return false;

		const Slot* _pIn = FindInputSlot(MakeSlotID(a_inPinName));
		if (!_pIn || !_pIn->IsConnected()) return false;

		// 実体に関わるところだけをもらう。
		// アクセスの種類(RTV/UAV)は自分の描き方なので触らない
		_pOut->resourceID = _pIn->resourceID;
		_pOut->name = _pIn->name;
		_pOut->format = _pIn->format;
		_pOut->width = _pIn->width;
		_pOut->height = _pIn->height;
		_pOut->scale = _pIn->scale;

		// 前段の絵を消さない
		_pOut->loadOp = ELoadOp::Load;
		return true;
	}

	// 上書きではなく描き足すパス(スカイ・パーティクル・UIなど)がこれを通す
	void Pass::FollowInputToOutput(const std::string& a_inPinName, const std::string& a_outPinName)
	{
		// 引き継げたならそれで終わり
		if (AliasOutputToInput(a_inPinName, a_outPinName)) return;

		Slot* _pOut = FindOutputSlot(MakeSlotID(a_outPinName));
		if (!_pOut) return;

		// 描き足す先が繋がっていない = このパスが最初に描く。
		//
		// Load のままにすると前フレームの中身がそのまま残り、
		// 「UIパスと出口だけのパイプライン」でもUIの後ろに前フレームの絵が出てしまう。
		// 描き足す相手が居ないのだから、まず消してから描く
		_pOut->loadOp = ELoadOp::Clear;

		// 消す色は「透明な黒」。
		//
		// 描き足す相手が居ないということは、この板は絵そのものではなく
		// 単体のレイヤー(UIだけを描いた板など)であり、後段で下の絵へ
		// アルファ合成される。ここを不透明黒(a=1)で消すと、何も描かれて
		// いない部分まで「不透明な黒がある」と主張することになり、
		// 合成した先でUIの背景が真っ黒に塗り潰される
		_pOut->clearColor = { 0.f, 0.f, 0.f, 0.f };
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

	Slot& Pass::DeclareImportedOutput(
		const std::string& a_pinName,
		const std::string& a_importName,
		EAccessType a_accessType)
	{
		// 実体は外から差し込まれるので、フォーマットはこちらでは決めない
		Slot& _slot = DeclareOutput(a_pinName, a_importName, DXGI_FORMAT_UNKNOWN, a_accessType);

		// 名前で待ち合わせる印。Compile の頭で、識別子がこの名前から起こされる
		_slot.importName = a_importName;
		return _slot;
	}
}
