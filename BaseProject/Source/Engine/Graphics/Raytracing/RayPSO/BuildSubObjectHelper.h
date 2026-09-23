#pragma once
namespace Engine::Raytracing
{
	//==========================================================================================
	// 
	// サブオブジェクト生成用ヘルパー
	//
	//==========================================================================================
	namespace BuildSubObjectHelper
	{
		//==========================================================================================
		// DXILライブラリのサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct DXILLibrarySubObject
		{
			DXILLibrarySubObject() {};
			void Init(const D3D12_DXIL_LIBRARY_DESC& a_desc)
			{
				desc = a_desc;
				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
				subObject.pDesc = &desc;
			}
			// バイトコードの実体さえ取れればよいので、
			// DXC が返す IDxcBlob でも ID3DBlob でも構わない
			void Init(ID3DBlob* a_pBlob,const std::vector<RayShaderData>& a_rayShader)
			{
				// エクスポートの説明構造体を作成
				exports.clear();
				for (auto& _shader : a_rayShader)
				{
					exports.push_back({ _shader.entryName,nullptr,D3D12_EXPORT_FLAG_NONE });
				}

				// ブロブを保存
				pBlob = a_pBlob;

				// DXILライブラリの説明構造体を作成
				desc = {};
				desc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
				desc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
				desc.NumExports = static_cast<UINT>(exports.size());
				desc.pExports = exports.data();

				// サブオブジェクトの説明構造体を作成
				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
				subObject.pDesc = &desc;
			}
			std::vector<D3D12_EXPORT_DESC> exports;
			ID3DBlob* pBlob = nullptr;
			D3D12_DXIL_LIBRARY_DESC desc = {};
			D3D12_STATE_SUBOBJECT subObject = {};
		};

		//==========================================================================================
		// ローカルルートシグネチャのサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct LocalRootSignatureSubObject
		{
			LocalRootSignatureSubObject() {};

			void Init(ID3D12RootSignature* a_pData)
			{
				pRoot = a_pData;
				subObject.pDesc = &pRoot;
				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			}
			ID3D12RootSignature* pRoot = nullptr;
			D3D12_STATE_SUBOBJECT subObject = {};
		};

		//==========================================================================================
		// グローバルルートシグネチャのサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct GlobalRootSignatureSubObject
		{
			GlobalRootSignatureSubObject() {};
			void Init(ID3D12RootSignature* a_pData)
			{
				pRoot = a_pData;
				subObject.pDesc = &pRoot;
				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			}
			ID3D12RootSignature* pRoot = nullptr;
			D3D12_STATE_SUBOBJECT subObject = {};
		};

		//==========================================================================================
		// ExportAssociationのサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct ExportAssociationSubObject
		{
			void Init(
				const WCHAR* a_exprtNmaes[], uint32_t a_exportCount,
				const D3D12_STATE_SUBOBJECT* a_pSubObjectToAssociate
			)
			{
				association.NumExports = a_exportCount;
				association.pExports = a_exprtNmaes;
				association.pSubobjectToAssociate = a_pSubObjectToAssociate;

				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
				subObject.pDesc = &association;
			}

			void Init(
				const std::vector<RayShaderData>& a_rayShader, 
				const D3D12_STATE_SUBOBJECT* a_pSubObjectToAssociate
			)
			{
				exprtNmaes.clear();
				for (auto& _shader : a_rayShader)
				{
					exprtNmaes.push_back(_shader.entryName);
				}

				association.NumExports = static_cast<UINT>(exprtNmaes.size());
				association.pExports = exprtNmaes.data();
				association.pSubobjectToAssociate = a_pSubObjectToAssociate;

				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
				subObject.pDesc = &association;
			}

			std::vector<const WCHAR*> exprtNmaes;
			D3D12_STATE_SUBOBJECT subObject = {};
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
		};

		//==========================================================================================
		// シェーダーコンフィグのサブオブジェクト作成ヘルパー構造体
		//==========================================================================================
		struct ShaderConfigSubObject
		{
			void Init(uint32_t a_maxAttributeSizeInBytes, uint32_t a_maxPayloadSizeInBaytes)
			{
				shaderConfig.MaxAttributeSizeInBytes = a_maxAttributeSizeInBytes;
				shaderConfig.MaxPayloadSizeInBytes = a_maxPayloadSizeInBaytes;

				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
				subObject.pDesc = &shaderConfig;
			}
			D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
			D3D12_STATE_SUBOBJECT subObject = {};
		};

		//==========================================================================================
		// パイプライン設定のサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct PipelineConfigSubObject
		{
			PipelineConfigSubObject(UINT a_max = 1)
			{
				config.MaxTraceRecursionDepth = a_max;

				subObjcet.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
				subObjcet.pDesc = &config;
			}
			D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
			D3D12_STATE_SUBOBJECT subObjcet = {};
		};

		//==========================================================================================
		// ヒットグループのサブオブジェクト作成のヘルパー構造体
		//==========================================================================================
		struct HitGroupSubObject
		{
			HitGroupSubObject() {}
			void Init(const HitGroup& a_hitGroup)
			{
				desc = {};
				desc.AnyHitShaderImport = a_hitGroup.anyHitShader;
				desc.ClosestHitShaderImport = a_hitGroup.closestHit;
				desc.HitGroupExport = a_hitGroup.name;
				desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

				subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
				subObject.pDesc = &desc;
			}
			D3D12_HIT_GROUP_DESC desc = {};
			D3D12_STATE_SUBOBJECT subObject = {};
		};
	};
}