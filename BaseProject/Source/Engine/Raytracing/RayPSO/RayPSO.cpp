#include "RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Raytracing
{
	namespace 
	{
		ComPtr<ID3D12RootSignature> CreateRootSignature(
			const D3D12_ROOT_SIGNATURE_DESC& a_desc,const std::string& a_name
		)
		{
			std::wstring _name = StringUtility::ToWideString(a_name);

			auto _pDevice5 = D3D12Wrapper::Instance().GetDevice5();
			ComPtr<ID3DBlob> _cpSigBlob;
			ComPtr<ID3DBlob> _cpErrorBlob;
			HRESULT _hr = D3D12SerializeRootSignature(&a_desc,D3D_ROOT_SIGNATURE_VERSION_1,&_cpSigBlob,&_cpErrorBlob);
			if (FAILED(_hr))
			{
				assert(0 && "レイトレ用ローカルルートシグネチャ生成に失敗");
				return nullptr;
			}
			ComPtr<ID3D12RootSignature> _cpRootSig;
			_pDevice5->CreateRootSignature(
				0,_cpSigBlob->GetBufferPointer(),_cpSigBlob->GetBufferSize(),IID_PPV_ARGS(&_cpRootSig)
			);
			_cpRootSig->SetName(_name.c_str());
			return _cpRootSig;
		}
	}


	// サブオブジェクト生成用ヘルパー
	namespace BuildSubObjectHelper
	{
		// ローカルルートシグネチャのサブオブジェクト作成のヘルパー構造体
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

		// ExportAssociationのサブオブジェクト作成のヘルパー構造体
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

			D3D12_STATE_SUBOBJECT subObject = {};
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
		};

		// シェーダーコンフィグのサブオブジェクト作成ヘルパー構造体
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

		// パイプライン設定のサブオブジェクト作成のヘルパー構造体
		struct PipelineConfigSubObject
		{
			PipelineConfigSubObject()
			{
				config.MaxTraceRecursionDepth = 4;

				subObjcet.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
				subObjcet.pDesc = &config;
			}
			D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
			D3D12_STATE_SUBOBJECT subObjcet = {};
		};

		// ヒットグループのサブオブジェクト作成のヘルパー構造体
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


	void RayPSO::Init()
	{
		// 変数用意
		auto* _pDevice5 = D3D12Wrapper::Instance().GetDevice5();
		std::array<D3D12_STATE_SUBOBJECT, 14> _subObjects;
		uint32_t _index = 0;

		// エラー出力
		ComPtr<ID3D12InfoQueue> infoQueue;
		_pDevice5->QueryInterface(IID_PPV_ARGS(&infoQueue));


		// DXILライブラリを用いてシェーダーをロード
		m_shader.Load("Asset/Shader/Ray/Raytracing.hlsl");

		// シェーダー情報定義（後で可変式にする予定とりまテスト）
		RayShader _exportRayShader[] = {
			{L"RayGen",LocalRootSignature::RayGen,ShaderCategory::RayGenerator},
			{L"Miss",LocalRootSignature::Empty,ShaderCategory::Miss},
			{L"ClosestHit",LocalRootSignature::PBRMaterialHit,ShaderCategory::ClosestHit}
		};
		D3D12_EXPORT_DESC exports[] =
		{
			{ L"RayGen",      nullptr, D3D12_EXPORT_FLAG_NONE },
			{ L"Miss",        nullptr, D3D12_EXPORT_FLAG_NONE },
			{ L"ClosestHit",  nullptr, D3D12_EXPORT_FLAG_NONE }//ClosestHit
		};


		// DXIL Libraryを登録
		D3D12_DXIL_LIBRARY_DESC _dxilLibDesc = {};
		auto _blob = m_shader.GetIDxcBlob();
		_dxilLibDesc.DXILLibrary.BytecodeLength = _blob->GetBufferSize();
		_dxilLibDesc.DXILLibrary.pShaderBytecode = _blob->GetBufferPointer();
		_dxilLibDesc.NumExports = _countof(exports);
		_dxilLibDesc.pExports = exports;

		_subObjects[_index].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		_subObjects[_index].pDesc = &_dxilLibDesc;

		_index++;
		
		// ヒットグループのサブオブジェクトの作成
		std::vector<BuildSubObjectHelper::HitGroupSubObject> _hitGroupSOVce;
		HitGroup _hitGroup = {
			L"HitGroup", L"ClosestHit",nullptr
		};
		_hitGroupSOVce.resize(1);		// 今のところカメラレイのみ
		for (int _i = 0; _i < 1; ++_i)
		{
			_hitGroupSOVce[_i].Init(_hitGroup);
			_subObjects[_index++] = _hitGroupSOVce[_i].subObject; 
		}

		// ルートシグネチャとシェーダーの関連付けを行うサブオブジェクトを作っていく
		m_rayGenRootSig.Create(
			{
				//{RootParameterType::DescriptorTable,{RangeType::UAV,RangeType::SRV,RangeType::CBV}}
			}
		);
		m_hitRootSig.Create(
			{
				//{RootParameterType::DescriptorTable,{RangeType::SRV}}
			}
		);
		m_emptyRootSig.Create({
			
			});
		auto BuildAndRegistRootSignatureAndAssSubObjectFunc = [&]
		(
			BuildSubObjectHelper::LocalRootSignatureSubObject& a_rsSO,
			BuildSubObjectHelper::ExportAssociationSubObject& a_eaSO,
			LocalRootSignature a_rootSig,
			const WCHAR* a_exportNames[]
		) 
		{
			if (a_rootSig == LocalRootSignature::RayGen)
			{
				a_rsSO.Init(m_rayGenRootSig.Get());
			}
			if (a_rootSig == LocalRootSignature::PBRMaterialHit)
			{
				a_rsSO.Init(m_hitRootSig.Get());
			}
			if (a_rootSig == LocalRootSignature::Empty)
			{
				a_rsSO.Init(m_emptyRootSig.Get());
			}
			_subObjects[_index] = a_rsSO.subObject;
			uint32_t _rgSOIndex = _index++;

			int _useRootSig = 0;
			for (auto& _shaderData : _exportRayShader)
			{
				if (_shaderData.rootsigType == a_rootSig)
				{
					a_exportNames[_useRootSig] = _shaderData.entryName;
					_useRootSig++;
				}
			}
			a_eaSO.Init(a_exportNames, _useRootSig, &(_subObjects[_rgSOIndex]));
			_subObjects[_index++] = a_eaSO.subObject;
		};

		// 関連付け
		BuildSubObjectHelper::LocalRootSignatureSubObject _rayGenSigSO, _modelSigSO, _emptySigSO;
		BuildSubObjectHelper::ExportAssociationSubObject _rayGenAssSO, _modelAssSO, _emptyAssSO;
		const WCHAR* _rayGenExportName[3];
		const WCHAR* _modelExportName[3];
		const WCHAR* _emptyExportName[3];
		BuildAndRegistRootSignatureAndAssSubObjectFunc(_rayGenSigSO,_rayGenAssSO,LocalRootSignature::RayGen,_rayGenExportName);
		BuildAndRegistRootSignatureAndAssSubObjectFunc(_modelSigSO,_modelAssSO,LocalRootSignature::PBRMaterialHit,_modelExportName);
		BuildAndRegistRootSignatureAndAssSubObjectFunc(_emptySigSO,_emptyAssSO,LocalRootSignature::Empty,_emptyExportName);

		// シェーダー設定
		// ペイロードサイズ
		BuildSubObjectHelper::ShaderConfigSubObject _shaderConfig;
		struct RayPayload
		{
			DXSM::Vector3 color;
			float pad;
		};
		_shaderConfig.Init(8,sizeof(RayPayload));
		_subObjects[_index] = _shaderConfig.subObject;
		
		uint32_t _shaderConfigIndex = _index++;
		BuildSubObjectHelper::ExportAssociationSubObject _configAssociationSO;
		const WCHAR* _entoryPointNames[4];
		for (int _i = 0; _i < 3; ++_i)
		{
			_entoryPointNames[_i] = _exportRayShader[_i].entryName;
		}
		_entoryPointNames[3] = L"HitGroup";
		_configAssociationSO.Init(_entoryPointNames,4,&_subObjects[_shaderConfigIndex]);
		_subObjects[_index++] = _configAssociationSO.subObject;

		// パイプライン設定のオブジェクトを作成
		BuildSubObjectHelper::PipelineConfigSubObject _config;
		_subObjects[_index++] = _config.subObjcet;

		// グローバルルートシグネチャのサブオブジェクト作成
		m_rootSig.Create({
			{RootParameterType::RootCBV,{}},
			{RootParameterType::DescriptorTable,{RangeType::SRV}},
			{RootParameterType::DescriptorTable,{RangeType::UAV}}
			});
		BuildSubObjectHelper::LocalRootSignatureSubObject _gRootSig;
		_gRootSig.Init(m_rootSig.Get());
		_gRootSig.subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		_subObjects[_index++] = _gRootSig.subObject;

		// パイプライン作成
		D3D12_STATE_OBJECT_DESC _desc;
		_desc.NumSubobjects = _index;
		_desc.pSubobjects = _subObjects.data();
		_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		auto _hr = _pDevice5->CreateStateObject(&_desc,IID_PPV_ARGS(&m_cpPSO));
		if (FAILED(_hr))
		{
			UINT64 count = infoQueue->GetNumStoredMessages();

			for (UINT64 i = 0; i < count; i++)
			{
				SIZE_T size = 0;
				infoQueue->GetMessage(i, nullptr, &size);
				std::vector<char> data(size);
				auto msg = (D3D12_MESSAGE*)data.data();
				infoQueue->GetMessage(i, msg, &size);

				OutputDebugStringA(msg->pDescription);
			}

			assert(0 && "レイトレ用パイプラインステートの作成に失敗");
			return;
		}
	}

	void* Engine::Raytracing::RayPSO::GetShaderID(const std::string& a_shaderEntry)
	{
		ComPtr<ID3D12StateObjectProperties> _props;
		m_cpPSO.As(&_props);

		std::wstring _entry = StringUtility::ToWideString(a_shaderEntry);

		return _props->GetShaderIdentifier(_entry.c_str());
	}
}