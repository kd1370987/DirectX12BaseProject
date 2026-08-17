#pragma once

#include "../../D3D12/D3DObject/RootSignature/RootSignature.h"

// このヘッダーは EngineCommon の早い段階で読まれるので、実体は持ち込まず前方宣言で済ませる
namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Raytracing
{
	// レイ用PSO作成構造体
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

		template<typename T>
		void SetPayload()
		{
			payloadSize = sizeof(T);
		}

		// シェーダー
		std::string shaderPass = "Raytracing";		// パス
		std::vector<RayShaderData> shaderDataVec;	// シェーダーデータ
		std::vector<HitGroup> hitGroupVec;			// ヒットグループデータ

		// レイの最大再帰回数
		UINT maxRecursionDepth = 1;

		// グローバルルートシグネチャ
		Handle<ID3D12RootSignature> globalRootSig = {};

		// ローカルルートシグネチャ
		Handle<ID3D12RootSignature> rayGenRootSig = {};	// レイジェネレーション用
		Handle<ID3D12RootSignature> missRootSig = {};	// ミスシェーダー用
		Handle<ID3D12RootSignature> hitRootSig = {};	// ヒットシェーダー用

		// ペイロードサイズ
		size_t payloadSize = 0;
	};

	// レイ用PSOクラス
	class RayPSO
	{
	public:

		~RayPSO() { Release(); }

		// 解放
		void Release();

		// パイプラインステート作成
		// ルートシグネチャの実体はサブオブジェクトの組み立てに要るので、
		// ここでマネージャーから引く(保持するのはハンドルのまま)
		bool Init(D3D12::Device* a_pDevice, D3D12::PipelineStateManager* a_pPSOManager, RayPSODesc& a_desc);

		const void* GetShaderID(const std::string& a_shaderEntry) const;
		const void* GetShaderID(const wchar_t* a_shaderEntry) const;

		ID3D12StateObject* Get()
		{
			return m_cpPSO.Get();
		}

		// グローバルルートシグネチャはハンドルで返す。
		// 張るときは RenderContext::SetComputeRootSignature(ハンドル版)へ渡す
		const Handle<ID3D12RootSignature>& GetRootSigHandle() const
		{
			return m_globalRootSigHandle;
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

		Handle<ID3D12RootSignature> m_globalRootSigHandle = {};

		Resource::Shader m_shader;
	};
}