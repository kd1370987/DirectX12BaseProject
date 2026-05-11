#pragma once
namespace Engine::D3D12
{
	// ルートシグネチャ作成構造体
	struct RootSignatureDesc
	{
		RootSignatureDesc() {};

		void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS a_flgas)
		{
			flags = a_flgas;
		}

		void AddDescriptorHeap(std::vector<RootRangeInit> a_rangeVec)
		{
			RootParamInit _init = {};
			_init.paramType = RootParameterType::DescriptorTable;
			_init.rangeVec = a_rangeVec;
			_init.shaderRegisterIndex = 0;
			paramVec.push_back(_init);
		}
		void AddRoot(RootParameterType a_type, UINT a_shaderIndex)
		{
			RootParamInit _init = {};
			_init.paramType = a_type;
			_init.rangeVec = {};
			_init.shaderRegisterIndex = a_shaderIndex;
			paramVec.push_back(_init);
		}

		// ルートシグネチャフラグ
		D3D12_ROOT_SIGNATURE_FLAGS flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		std::vector<RootParamInit> paramVec = {};	// ルートパラメーター構築情報

		bool isUseStaticSampler = true;				// スタティックサンプラーを使用するかどうか

		std::string name;
	};

	// ビルダー
	class RootSignatureBuilder
	{
	public:

		static D3D12_ROOT_SIGNATURE_DESC CreateDesc(const RootSignatureDesc& a_desc);
		static ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& a_desc);
		static ComPtr<ID3D12RootSignature> Create(const RootSignatureDesc& a_desc);
		static ComPtr<ID3D12RootSignature> Create(const std::string& a_path);
		static ComPtr<ID3D12RootSignature> Create(ComPtr<ID3DBlob> a_cpBlob);
	};
}