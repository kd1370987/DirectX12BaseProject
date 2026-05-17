#pragma once

#include "../../D3D12/D3DObject/RootSignature/RootSignature.h"

namespace Engine::Raytracing
{
	struct RayPSOInit
	{
		RayPSOInit() {};

		void AddShader(const wchar_t* a_entryName, LocalRootSignature a_rootSigType, ShaderCategory a_category)
		{
			shaderDataVec.push_back({ a_entryName,a_rootSigType,a_category });
		}

		void AddHitGroup(const wchar_t* a_name, const wchar_t* a_closestHit, const wchar_t* a_anyHit = nullptr)
		{
			hitGroupVec.push_back({ a_name,a_closestHit,a_anyHit });
		}

		// シェーダー
		std::string shaderPass = "Raytracing";		// パス
		std::vector<RayShaderData> shaderDataVec;	// シェーダーデータ
		std::vector<HitGroup> hitGroupVec;			// ヒットグループデータ

		// レイの最大再帰回数
		UINT maxRecursionDepth = 1;
		
		// グローバルルートシグネチャ
		std::optional<RootSigInit> opGlobalRootSigInit = std::nullopt;

		// ローカルルートシグネチャ
		std::optional<RootSigInit> opRayGenRootSigInit = std::nullopt;	// レイジェネレーション用
		std::optional<RootSigInit> opMissRootSigInit = std::nullopt;	// ミスシェーダー用
		std::optional<RootSigInit> opHitRootSigInit = std::nullopt;		// ヒットシェーダー用
	};

	struct RayPSODesc
	{
		RayPSODesc() {};

		void AddShader(const wchar_t* a_entryName, LocalRootSignature a_rootSigType, ShaderCategory a_category)
		{
			shaderDataVec.push_back({ a_entryName,a_rootSigType,a_category });
		}

		void AddHitGroup(const wchar_t* a_name, const wchar_t* a_closestHit, const wchar_t* a_anyHit = nullptr)
		{
			hitGroupVec.push_back({ a_name,a_closestHit,a_anyHit });
		}

		// シェーダー
		std::string shaderPass = "Raytracing";		// パス
		std::vector<RayShaderData> shaderDataVec;	// シェーダーデータ
		std::vector<HitGroup> hitGroupVec;			// ヒットグループデータ

		// レイの最大再帰回数
		UINT maxRecursionDepth = 1;

		// グローバルルートシグネチャ
		ID3D12RootSignature* pGlobalRootSig = nullptr;

		// ローカルルートシグネチャ
		ID3D12RootSignature* pRayGenRootSig = nullptr;	// レイジェネレーション用
		ID3D12RootSignature* pMissRootSig = nullptr;	// ミスシェーダー用
		ID3D12RootSignature* pHitRootSig = nullptr;		// ヒットシェーダー用
	};

	class RayPSO
	{
	public:

		// パイプラインステート作成
		bool Init(const RayPSOInit& a_init);
		// 生成済みシグネチャ
		bool Init(const RayPSODesc& a_desc);

		const void* GetShaderID(const std::string& a_shaderEntry) const;
		const void* GetShaderID(const const wchar_t* a_shaderEntry) const;

		ID3D12StateObject* Get()
		{
			return m_cpPSO.Get();
		}

		ID3D12RootSignature* GetRootSig()
		{
			//return m_pRootSig;
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

		D3D12::RootSignature m_rootSig;
		ID3D12RootSignature* m_pRootSig;

		D3D12::RootSignature m_rayGenRootSig;
		D3D12::RootSignature m_hitRootSig;
		D3D12::RootSignature m_emptyRootSig;

		Resource::ShaderLibrary m_shader;
	};
}