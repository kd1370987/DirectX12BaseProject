#include "VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	void VirtualResource::SetupFromOutputSlot(const std::string& a_name, const Slot& a_slot)
	{
		m_name = a_name;
		m_type = a_slot.type;
		m_format = a_slot.format;
		m_width = a_slot.width;
		m_height = a_slot.height;
		m_scale = a_slot.scale;

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

		m_physicalIndex[0] = ResourceHandle::INVALID_INDEX;
		m_physicalIndex[1] = ResourceHandle::INVALID_INDEX;
	}

	void VirtualResource::SetupAsImported(const std::string& a_name, EPassSlotType a_type, D3D12_RESOURCE_STATES a_initialState)
	{
		m_name = a_name;
		m_type = a_type;

		// 外部で作られた実体をそのまま使うので、こちらでサイズやフォーマットは決めない
		m_format = DXGI_FORMAT_UNKNOWN;
		m_width = 0;
		m_height = 0;
		m_scale = 1.f;
		m_usage = Resource::TextureUsage::None;

		m_clearColor = { 0.f, 0.f, 0.f, 1.f };
		m_isClear = false;

		m_isImported = true;

		// 外部リソースはフレームをまたいだ入れ替えをこちらで面倒見ない
		m_isTemporal = false;

		// 外部リソースは向こうが渡してきた状態で入ってくる
		m_initialState[0] = a_initialState;
		m_initialState[1] = a_initialState;
		ResetStateToInitial();

		m_physicalIndex[0] = ResourceHandle::INVALID_INDEX;
		m_physicalIndex[1] = ResourceHandle::INVALID_INDEX;
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
			if (a_slot.width != 0) m_width = a_slot.width;
			if (a_slot.height != 0) m_height = a_slot.height;
			m_scale = a_slot.scale;

			// 一度でもクリア指定が入ったら、クリアバリューを持つリソースとして作る
			if (a_slot.loadOp == ELoadOp::Clear)
			{
				m_isClear = true;
				m_clearColor = a_slot.clearColor;
			}
		}
	}

	void VirtualResource::ResolveSize(UINT64 a_baseWidth, UINT a_baseHeight)
	{
		// 外部リソースは実体が向こうにあるので触らない
		if (m_isImported) return;

		// バッファは width をバイト数として使っているので、解像度を掛けても意味がない
		if (IsBuffer()) return;

		// 明示的にサイズが入っているものはそのまま
		if (m_width != 0 && m_height != 0) return;

		if (a_baseWidth == 0 || a_baseHeight == 0)
		{
			ENGINE_WARNING("[VirtualResource] 描画解像度が設定されていないためサイズを決められません : %s", m_name.c_str());
			return;
		}

		// スケール指定 : ハーフ解像度のGIバッファなどはここで 0.5 が掛かる
		if (m_width == 0)  m_width = static_cast<UINT64>(static_cast<float>(a_baseWidth) * m_scale);
		if (m_height == 0) m_height = static_cast<UINT>(static_cast<float>(a_baseHeight) * m_scale);

		// 0 にすると生成に失敗するので、最低1ピクセルは残す
		if (m_width == 0)  m_width = 1;
		if (m_height == 0) m_height = 1;
	}

	bool VirtualResource::HasUsage(Resource::TextureUsage a_usage) const
	{
		return (m_usage & a_usage) != Resource::TextureUsage::None;
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
