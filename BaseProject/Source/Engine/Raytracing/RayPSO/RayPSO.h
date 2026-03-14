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

		ComPtr<ID3D12StateObject> m_cpPSO;
		ComPtr<ID3D12RootSignature> m_cpRootSig;
		ComPtr<IDxcBlob> m_dxcBlob;				// DXCコンパイラを使用したときのシェーダーデータ

		RootSignature m_rootSig;
	};
}