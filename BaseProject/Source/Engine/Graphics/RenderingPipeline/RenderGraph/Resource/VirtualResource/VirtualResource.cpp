#include "VirtualResource.h"

#include "../../../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Graphics::Pipeline
{
	void VirtualResource::SetupFromOutputSlot(const Slot& a_slot, UINT64 a_baseWidth, UINT a_baseHeight)
	{
		m_resourceID = a_slot.resourceID;
		m_name = a_slot.name;
		m_type = a_slot.type;
		m_format = a_slot.format;

		// スロットが言ってきた値は宣言としてそのまま持つ。
		// 0 は「解像度に従う」の意味なので、ここで潰してはいけない
		m_declWidth = a_slot.width;
		m_declHeight = a_slot.height;
		m_scale = a_slot.scale;

		m_baseWidth = a_baseWidth;
		m_baseHeight = a_baseHeight;

		m_usage = ToUsage(a_slot.accessType, true);

		m_clearColor = a_slot.clearColor;
		m_isClear = (a_slot.loadOp == ELoadOp::Clear);

		m_isImported = false;
		m_isTemporal = a_slot.isTemporal;

		// 自前で作るリソースはフレームの入口では COMMON。
		// 毎フレーム同じ状態から始まるように、最後もここへ戻す
		m_initialState[0] = D3D12_RESOURCE_STATE_COMMON;
		m_initialState[1] = D3D12_RESOURCE_STATE_COMMON;
		ResetStateToInitial();

		m_physicalIndex[0] = INVALID_PHYSICAL_INDEX;
		m_physicalIndex[1] = INVALID_PHYSICAL_INDEX;

		// 区間は実行順が決まってから RenderGraph が入れる
		ResetLifetime();

		// 実サイズと占有サイズは、生まれたこの時点で出しておく。
		// 読み手が用途フラグを足すと変わりうるので、MergeSlot でも出し直す
		ResolveSizeAndAllocation();
	}

	void VirtualResource::SetupAsImported(const std::string& a_name, EPassSlotType a_type, D3D12_RESOURCE_STATES a_initialState)
	{
		// 外部リソースは作り手のパスが居ないので、識別子は名前から起こす。
		// 差し込む側とパスの出力ピンは、この名前で待ち合わせる
		m_resourceID = ResourceID::FromImportName(a_name);
		m_name = a_name;
		m_type = a_type;

		// 外部で作られた実体をそのまま使うので、こちらでサイズやフォーマットは決めない。
		// ヒープも食わないので占有サイズは 0 のまま
		m_format = DXGI_FORMAT_UNKNOWN;
		m_declWidth = 0;
		m_declHeight = 0;
		m_baseWidth = 0;
		m_baseHeight = 0;
		m_width = 0;
		m_height = 0;
		m_scale = 1.f;
		m_usage = Resource::TextureUsage::None;

		m_allocationSize = 0;
		m_allocationAlignment = 0;

		m_clearColor = { 0.f, 0.f, 0.f, 1.f };
		m_isClear = false;

		m_isImported = true;

		// 外部リソースはフレームをまたいだ入れ替えをこちらで面倒見ない
		m_isTemporal = false;

		// 外部リソースは向こうが渡してきた状態で入ってくる
		m_initialState[0] = a_initialState;
		m_initialState[1] = a_initialState;
		ResetStateToInitial();

		m_physicalIndex[0] = INVALID_PHYSICAL_INDEX;
		m_physicalIndex[1] = INVALID_PHYSICAL_INDEX;

		// 区間は実行順が決まってから RenderGraph が入れる。
		// 外部リソースは使い回せないが、どこで触られているかは同じように分かる
		ResetLifetime();
	}

	void VirtualResource::MergeSlot(const Slot& a_slot)
	{
		const bool _isWrite = !a_slot.isIn;

		// 用途フラグは触ったぶんだけ足していく。
		// 「Aパスが書いて、B・Cパスが読む」なら RTV|SRV が立った状態になる
		m_usage = m_usage | ToUsage(a_slot.accessType, _isWrite);

		// 外部リソースは向こうが持っている実体が正なので、要件は上書きしない
		if (m_isImported) return;

		// どこか1つでも Temporal と言っていたら履歴つきとして扱う
		if (a_slot.isTemporal) m_isTemporal = true;

		if (_isWrite)
		{
			// フォーマットとサイズは書き込み側が主導権を持つ
			if (a_slot.format != DXGI_FORMAT_UNKNOWN) m_format = a_slot.format;
			if (a_slot.width != 0) m_declWidth = a_slot.width;
			if (a_slot.height != 0) m_declHeight = a_slot.height;
			m_scale = a_slot.scale;

			// 一度でもクリア指定が入ったら、クリアバリューを持つリソースとして作る
			if (a_slot.loadOp == ELoadOp::Clear)
			{
				m_isClear = true;
				m_clearColor = a_slot.clearColor;
			}
		}

		// 書き手はサイズとフォーマットを、読み手は用途フラグを変える。
		// どちらも占有サイズに効くので、足し込むたびに出し直す
		ResolveSizeAndAllocation();
	}

	void VirtualResource::ResolveSize(UINT64 a_baseWidth, UINT a_baseHeight)
	{
		// 外部リソースは実体が向こうにあるので触らない
		if (m_isImported) return;

		m_baseWidth = a_baseWidth;
		m_baseHeight = a_baseHeight;

		// テクスチャで宣言が 0 なら、土台の解像度が無いと決めようがない。
		// バッファは宣言のバイト数だけで決まるので解像度は要らない
		if (!IsBuffer() && (m_declWidth == 0 || m_declHeight == 0) &&
			(a_baseWidth == 0 || a_baseHeight == 0))
		{
			ENGINE_WARNING("[VirtualResource] 描画解像度が設定されていないためサイズを決められません : %s", m_name.c_str());
			return;
		}

		ResolveSizeAndAllocation();
	}

	// 宣言値と土台の解像度から実サイズを出す。
	// 宣言側は触らないので、何度呼んでも同じ結果になる
	void VirtualResource::ResolveSizeAndAllocation()
	{
		if (m_isImported) return;

		if (IsBuffer())
		{
			// バッファは width をバイト数として使っているので、解像度を掛けても意味がない
			m_width = m_declWidth;
			m_height = 1;

			CalcAllocationSize();
			return;
		}

		m_width = m_declWidth;
		m_height = m_declHeight;

		// 宣言が 0 のところだけ解像度から埋める。
		// スケール指定 : ハーフ解像度のGIバッファなどはここで 0.5 が掛かる
		if (m_width == 0 || m_height == 0)
		{
			// 土台がまだ来ていないなら決められない。
			// 異常かどうかは呼び手(ResolveSize)が判断する
			if (m_baseWidth == 0 || m_baseHeight == 0) return;

			if (m_width == 0)  m_width = static_cast<UINT64>(static_cast<float>(m_baseWidth) * m_scale);
			if (m_height == 0) m_height = static_cast<UINT>(static_cast<float>(m_baseHeight) * m_scale);
		}

		// 0 にすると生成に失敗するので、最低1ピクセルは残す
		if (m_width == 0)  m_width = 1;
		if (m_height == 0) m_height = 1;

		CalcAllocationSize();
	}

	bool VirtualResource::HasUsage(Resource::TextureUsage a_usage) const
	{
		return (m_usage & a_usage) != Resource::TextureUsage::None;
	}

	void VirtualResource::CalcAllocationSize()
	{
		m_allocationSize = 0;
		m_allocationAlignment = 0;

		// 外部リソースの実体はグラフの外の持ち物なので、こちらのヒープは食わない
		if (m_isImported) return;

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		if (!_pDevice) return;

		D3D12_RESOURCE_DESC _desc = {};
		_desc.Alignment = 0;
		_desc.DepthOrArraySize = 1;
		_desc.MipLevels = 1;
		_desc.SampleDesc.Count = 1;
		_desc.SampleDesc.Quality = 0;

		if (IsBuffer())
		{
			// まだサイズが決まっていない : 決まってから出し直される
			if (m_width == 0) return;

			_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			_desc.Width = m_width;			// width にバイト数が入っている
			_desc.Height = 1;
			_desc.Format = DXGI_FORMAT_UNKNOWN;
			_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			// PhysicalResource::Create が立てるフラグに合わせる
			_desc.Flags = HasUsage(Resource::TextureUsage::UAV)
				? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				: D3D12_RESOURCE_FLAG_NONE;
		}
		else
		{
			// 要件がまだ揃っていない : 揃ってから出し直される
			if (m_width == 0 || m_height == 0) return;
			if (m_format == DXGI_FORMAT_UNKNOWN) return;

			_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			_desc.Width = m_width;
			_desc.Height = m_height;
			_desc.Format = m_format;
			_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

			// 用途フラグは詰み方と大きさを変える。
			// 実際に作るときと同じものを渡さないと、見積もりが実体とずれる
			_desc.Flags = Resource::GetResourceFlags(m_usage);
		}

		D3D12_RESOURCE_ALLOCATION_INFO _info = _pDevice->GetResourceAllocationInfo(0,1,&_desc);

		// 通らない組み合わせを渡すと SizeInBytes が UINT64_MAX で返る。
		// そのまま足すとヒープサイズが桁あふれするので、0 のままにしておく
		if (_info.SizeInBytes == UINT64_MAX)
		{
			ENGINE_WARNING("[VirtualResource] 占有サイズを見積もれませんでした : %s", m_name.c_str());
			return;
		}

		m_allocationSize = _info.SizeInBytes;
		m_allocationAlignment = _info.Alignment;
	}

	Resource::TextureUsage VirtualResource::ToUsage(EAccessType a_accessType, bool a_isWrite)
	{
		if (a_isWrite)
		{
			// 書いた結果は後続のパスが読むのが普通なので、SRV も一緒に立てておく
			switch (a_accessType)
			{
			case EAccessType::RTV:			return Resource::TextureUsage::RTV | Resource::TextureUsage::SRV;
			case EAccessType::Depth_Write:	return Resource::TextureUsage::DSV | Resource::TextureUsage::SRV;
			case EAccessType::UAV:			return Resource::TextureUsage::UAV | Resource::TextureUsage::SRV;
			default:						return Resource::TextureUsage::None;
			}
		}

		switch (a_accessType)
		{
		case EAccessType::SRV:			return Resource::TextureUsage::SRV;
		case EAccessType::UAV:			return Resource::TextureUsage::UAV;
		case EAccessType::Depth_Read:	return Resource::TextureUsage::DSV;
		default:						return Resource::TextureUsage::None;
		}
	}

	D3D12_RESOURCE_STATES VirtualResource::ToResourceState(EAccessType a_accessType)
	{
		switch (a_accessType)
		{
			// シェーダーから読むものは、どのステージから引かれても通るように両方立てる
		case EAccessType::SRV:			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case EAccessType::RTV:			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case EAccessType::UAV:			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case EAccessType::Depth_Read:	return D3D12_RESOURCE_STATE_DEPTH_READ;
		case EAccessType::Depth_Write:	return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case EAccessType::CopySrc:		return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case EAccessType::CopyDst:		return D3D12_RESOURCE_STATE_COPY_DEST;
		default:						return D3D12_RESOURCE_STATE_COMMON;
		}
	}

	bool VirtualResource::IsReadOnlyState(D3D12_RESOURCE_STATES a_state)
	{
		// 読み取り専用のステートだけを並べたもの。
		// これに収まっていれば、同じ塊の中で複数のパスが同時に読んでも問題ない
		constexpr D3D12_RESOURCE_STATES _readOnlyMask =
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_DEPTH_READ |
			D3D12_RESOURCE_STATE_COPY_SOURCE;

		// COMMON(=0) は「読み取り専用」ではないので弾く
		if (a_state == D3D12_RESOURCE_STATE_COMMON) return false;

		return (a_state & ~_readOnlyMask) == 0;
	}
}
