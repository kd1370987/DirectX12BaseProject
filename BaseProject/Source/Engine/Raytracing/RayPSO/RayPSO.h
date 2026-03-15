#pragma once

#include "../../D3D12/D3DObject/RootSignature/RootSignature.h"

namespace Engine::Raytracing
{
	class RayPSO
	{
	public:

		// パイプラインステート作成
		void Init();

		void* GetShaderID(const std::string& a_shaderEntry);

		ID3D12StateObject* Get()
		{
			return m_cpPSO.Get();
		}

		ID3D12RootSignature* GetRootSig()
		{
			return m_rootSig.Get();
		}

	private:

		// ルートシグネチャ定義
		struct RootSignatureDesc
		{
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			std::vector<D3D12_DESCRIPTOR_RANGE> range;
			std::vector<D3D12_ROOT_PARAMETER> rootParam;
		};

	private:

		ComPtr<ID3D12StateObject> m_cpPSO;

		RootSignature m_rootSig;

		RootSignature m_rayGenRootSig;
		RootSignature m_hitRootSig;
		RootSignature m_emptyRootSig;

		Resource::ShaderLibrary m_shader;
	};
}