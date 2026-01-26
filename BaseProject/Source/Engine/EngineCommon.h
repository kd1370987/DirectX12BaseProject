#pragma once

#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/RenderTarget/RenderTarget.h"

#include "Engine/GraphicResource/Resource/Model/ModelResource/Vertex/Vertex.h"

#include "Engine/ECS/ECSCommon.h"

#include "Engine/GraphicResource/Serialize/ModelSerialize/TinyGLTFSerialize/TinyGLTFSerialize.h"
#include "Engine/GraphicResource/Serialize/ModelSerialize/AssimpSerialize/AssimpSerialize.h"

namespace Resource
{
	using DataIndex = uint16_t;
	using DataGeneration = uint16_t;
	using ID = uint32_t;

	namespace Limits
	{
		constexpr ID MAX_STORAGE = 0xFFFFFFFF;
	}
}

#include "Engine/SlotStorage/SlotStorage.h"

#include "D3D12/D3D12Common.h"