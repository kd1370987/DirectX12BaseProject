#include "MonitorPass.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Graphics::Pipeline
{
	MonitorPass::~MonitorPass()
	{
		ReleasePreviewTexture();
	}

	void MonitorPass::SetupSlots()
	{
		// シェーダーを通さないので、読み書きともコピーのアクセスにする
		DeclareInput("Source", EAccessType::CopySrc);

		// フォーマットと大きさは OnLinksResolved で入力からもらう。
		// ここで決め打つと、繋いだ相手と食い違ったときにコピーが通らない
		DeclareOutput("Result", m_resourceName, DXGI_FORMAT_R8G8B8A8_UNORM, EAccessType::CopyDst);
	}

	void MonitorPass::ApplyOutput()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_resourceName;
	}

	//======================================================================================
	// 出力の形を入力に合わせる
	//
	// CopyResource は大きさが一致していて、フォーマットが同じ型無し親を持つ組でないと通らない。
	// 入力に何を繋がれても素通しできるよう、名前だけ自分のものにして
	// 実体に関わるところは全部もらう(深度だけは読める形へ直す)
	//======================================================================================
	void MonitorPass::OnLinksResolved()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		// 名前はロード直後などに落ちていることがあるので、ここでも揃えておく
		_pOut->name = m_resourceName;

		const Slot* _pIn = FindInputSlot(MakeSlotID("Source"));
		if (!_pIn || !_pIn->IsConnected()) return;

		_pOut->format = ToCopyableFormat(_pIn->format);
		_pOut->width = _pIn->width;
		_pOut->height = _pIn->height;
		_pOut->scale = _pIn->scale;
	}

	DXGI_FORMAT MonitorPass::ToCopyableFormat(DXGI_FORMAT a_format)
	{
		switch (a_format)
		{
			// 深度 : 型無し親が同じなので、コピーはそのまま通る
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;

		default:
			return a_format;
		}
	}

	void MonitorPass::Compile(const PassContext& a_context)
	{
		const Slot* _pIn = FindInputSlot(MakeSlotID("Source"));
		if (!_pIn || !_pIn->IsConnected())
		{
			ReleasePreviewTexture();
			return;
		}

		// 物理リソースが割り当てられた後に呼ばれるので、ここで実体の形を見られる
		EnsurePreviewTexture(a_context.GetResource(*_pIn));
	}

	void MonitorPass::Update(const PassContext& a_context)
	{
		if (!a_context.pCmdList) return;

		const Slot* _pIn = FindInputSlot(MakeSlotID("Source"));
		if (!_pIn || !_pIn->IsConnected()) return;

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		D3D12::GPUResource* _pSrc = a_context.GetResource(*_pIn);
		D3D12::GPUResource* _pDst = a_context.GetResource(*_pOut);
		if (!_pSrc || !_pDst) return;

		//----------------------------------------------------------------------------------
		// 素通し : ステートはグラフが CopySrc / CopyDst へ遷移させてある
		//----------------------------------------------------------------------------------
		a_context.pCmdList->CopyResource(_pDst->GetResource(), _pSrc->GetResource());

		//----------------------------------------------------------------------------------
		// モニターへの写し
		//
		// グラフの外の持ち物なのでステートは自分で運ぶ。
		// 写し終わりに PIXEL_SHADER_RESOURCE へ置いておくと、
		// このフレームの後半で走る ImGui がそのまま読める
		//----------------------------------------------------------------------------------
		if (!m_isPreview || !m_upPreviewTex) return;

		m_upPreviewTex->Barrier(a_context.pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		a_context.pCmdList->CopyResource(m_upPreviewTex->GetResource(), _pSrc->GetResource());
		m_upPreviewTex->Barrier(a_context.pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	//======================================================================================
	// モニター用テクスチャの用意
	//======================================================================================
	void MonitorPass::EnsurePreviewTexture(D3D12::GPUResource* a_pSource)
	{
		if (!a_pSource || !a_pSource->GetResource())
		{
			ReleasePreviewTexture();
			return;
		}

		const D3D12_RESOURCE_DESC _srcDesc = a_pSource->GetResource()->GetDesc();

		// バッファは絵にならないので持たない
		if (_srcDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		{
			ReleasePreviewTexture();
			return;
		}

		// ImGuiへ渡すので、SRVを張れる形にして持つ
		const DXGI_FORMAT _format = ToCopyableFormat(_srcDesc.Format);

		// すでに同じ形のものを持っていれば作り直さない
		if (m_upPreviewTex)
		{
			const D3D12_RESOURCE_DESC& _curDesc = m_upPreviewTex->GetDesc();
			if (_curDesc.Format == _format &&
				_curDesc.Width == _srcDesc.Width &&
				_curDesc.Height == _srcDesc.Height)
			{
				return;
			}
		}

		ReleasePreviewTexture();

		Resource::TextureCreateDesc _desc = {};
		_desc.name = GetName() + "_Monitor";
		_desc.width = _srcDesc.Width;
		_desc.height = _srcDesc.Height;
		_desc.format = _format;

		// 読むだけの持ち物なので SRV だけでよい。
		// ここで DSV や RTV を立てると、要らないビューとクリアバリューまで抱える
		_desc.usage = Resource::TextureUsage::SRV;

		m_upPreviewTex = std::make_unique<Resource::Texture>();
		m_upPreviewTex->Create(_desc);
	}

	void MonitorPass::ReleasePreviewTexture()
	{
		if (m_upPreviewTex) m_upPreviewTex->Release();
		m_upPreviewTex.reset();
	}

	//======================================================================================
	// ノードに出す中身を持っているパスを探す
	//======================================================================================
	const MonitorPass* MonitorPass::ResolveViewSource()
	{
		// 自分が実行インスタンスなら、自分の中身がそのまま最新
		if (m_upPreviewTex) return this;

		auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return this;

		// GUIDは設計図から複製するときに引き継がれるので、これで同じノードを指せる
		Pass* _pRuntime = _pGE->FindPipelinePass(GetGUID());
		if (!_pRuntime || _pRuntime == this) return this;

		// GUIDで引いている以上ここは必ず一致するが、
		// 万一ずれていたら別のパスへキャストすることになるので確かめておく
		if (_pRuntime->GetTypeID() != GetTypeID()) return this;

		return static_cast<const MonitorPass*>(_pRuntime);
	}

	EPassEditResult MonitorPass::EditUpdate()
	{
		EPassEditResult _result = EPassEditResult::None;

		// 出力リソースの表示名。
		// 同一性は「作ったパス + 出力ピン」で決まるので、ここが被っても中身は混ざらない。
		// リソース一覧やデバッグ表示で見分けるためのラベル
		char _nameBuf[128] = {};
		std::snprintf(_nameBuf, sizeof(_nameBuf), "%s", m_resourceName.c_str());
		if (ImGui::InputText("ResourceName", _nameBuf, sizeof(_nameBuf)))
		{
			m_resourceName = _nameBuf;
			ApplyOutput();

			// リソースを作り直すので組み直しが要る
			_result = EPassEditResult::Structure;
		}

		// 写しを取るかどうかは実行インスタンス側の振る舞いなので、値を配る必要がある
		if (ImGui::Checkbox("Preview", &m_isPreview) && _result == EPassEditResult::None)
		{
			_result = EPassEditResult::Param;
		}

		// 表示の大きさは設計図側でしか使わないので、配らない
		ImGui::DragFloat("PreviewWidth", &m_previewWidth, 1.0f, 64.0f, 1024.0f);

		ImGui::TextDisabled("フォーマットと大きさは入力から受け取る");

		return _result;
	}

	void MonitorPass::EditNode()
	{
		if (!m_isPreview)
		{
			ImGui::TextDisabled("Preview : off");
			return;
		}

		// 設計図のパスは実行されないので、中身は実行インスタンスから借りる
		const MonitorPass* _pView = ResolveViewSource();
		const Resource::Texture* _pTex = _pView ? _pView->GetPreviewTexture() : nullptr;

		if (!_pTex || !_pTex->GetImGuiSRV().IsValid())
		{
			// カメラがこのパイプラインを回していないあいだはここに来る
			ImGui::TextDisabled("表示するものがありません");
			return;
		}

		const D3D12_RESOURCE_DESC& _desc = _pTex->GetDesc();
		if (_desc.Width == 0 || _desc.Height == 0) return;

		const auto _gpuHandle =
			D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());

		const float _aspect = static_cast<float>(_desc.Height) / static_cast<float>(_desc.Width);
		const ImVec2 _size(m_previewWidth, m_previewWidth * _aspect);

		ImGui::Image(static_cast<ImTextureID>(_gpuHandle.ptr), _size);

		ImGui::TextDisabled("%llu x %u", _desc.Width, _desc.Height);
	}

	void MonitorPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_resourceName);
		a_arch.Field("isPreview", m_isPreview);
		a_arch.Field("previewWidth", m_previewWidth);

		// 値が入ったのはスロットを作った後なので、ここで反映し直す
		if (a_arch.IsLoading()) ApplyOutput();
	}
}
