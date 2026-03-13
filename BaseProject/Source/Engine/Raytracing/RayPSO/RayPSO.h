#pragma once

namespace Engine::Raytracing
{
	class PSO
	{
	public:

		// パイプラインステート作成
		void Init();

	private:

		ComPtr<ID3D12StateObject> m_cpPSO;
		ComPtr<ID3D12RootSignature> m_cpRootSig;
	};
}