#include "RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "BuildSubObjectHelper.h"

namespace Engine::Raytracing
{
	bool RayPSO::Init(const RayPSOInit& a_init)
	{
		// エラー出力
		auto* _pDevice5 = D3D12Wrapper::Instance().GetDevice5();
		ComPtr<ID3D12InfoQueue> infoQueue;
		_pDevice5->QueryInterface(IID_PPV_ARGS(&infoQueue));

		// 変数用意
		std::vector<D3D12_STATE_SUBOBJECT> _subObjects;
		_subObjects.reserve(14);

		// DXILライブラリを用いてシェーダーをロード
		m_shader.Load(a_init.shaderPass);

		// DXIL Libraryを登録
		BuildSubObjectHelper::DXILLibrarySubObject _dxilLibSO;
		_dxilLibSO.Init(m_shader.GetIDxcBlob(), a_init.shaderDataVec);
		_subObjects.push_back(_dxilLibSO.subObject);

		// ヒットグループのサブオブジェクトの作成
		std::vector<BuildSubObjectHelper::HitGroupSubObject> _hitGroupSOVce;
		_hitGroupSOVce.resize(a_init.hitGroupVec.size());
		for (int _i = 0; _i < a_init.hitGroupVec.size(); ++_i)
		{
			_hitGroupSOVce[_i].Init(a_init.hitGroupVec[_i]);
			_subObjects.push_back(_hitGroupSOVce[_i].subObject);
		}


		// ルートシグネチャとシェーダーの関連付けを行うサブオブジェクトを作っていく
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
			_subObjects.push_back(a_rsSO.subObject);
			uint32_t _rgSOIndex = _subObjects.size() - 1;

			int _useRootSig = 0;
			for (auto& _shaderData : a_init.shaderDataVec)
			{
				if (_shaderData.rootsigType == a_rootSig)
				{
					a_exportNames[_useRootSig] = _shaderData.entryName;
					_useRootSig++;
				}
			}
			a_eaSO.Init(a_exportNames, _useRootSig, &(_subObjects[_rgSOIndex]));
			_subObjects.push_back(a_eaSO.subObject);
		};
		BuildSubObjectHelper::LocalRootSignatureSubObject _rayGenSigSO, _modelSigSO, _emptySigSO;
		BuildSubObjectHelper::ExportAssociationSubObject _rayGenAssSO, _modelAssSO, _emptyAssSO;
		const WCHAR* _rayGenExportName[5];
		const WCHAR* _modelExportName[5];
		const WCHAR* _emptyExportName[5];
		// レイジェネレーションシェーダーにルートシグネチャがあれば関連付ける
		if (a_init.opRayGenRootSigInit.has_value())
		{
			m_rayGenRootSig.Create(a_init.opRayGenRootSigInit.value_or(RootSigInit()));
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_rayGenSigSO, _rayGenAssSO, LocalRootSignature::RayGen, _rayGenExportName);
		}
		// ヒットシェーダーにルートシグネチャがあれば関連付ける
		if (a_init.opHitRootSigInit.has_value())
		{
			m_hitRootSig.Create(a_init.opHitRootSigInit.value_or(RootSigInit()));
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_modelSigSO, _modelAssSO, LocalRootSignature::PBRMaterialHit, _modelExportName);
		}
		// ミスシェーダーにルートシグネチャがあれば関連付ける
		if (a_init.opMissRootSigInit.has_value())
		{
			m_emptyRootSig.Create(a_init.opMissRootSigInit.value_or(RootSigInit()));
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_emptySigSO, _emptyAssSO, LocalRootSignature::Empty, _emptyExportName);
		}

		// シェーダー設定(ペイロード)
		BuildSubObjectHelper::ShaderConfigSubObject _shaderConfig;
		struct RayPayload
		{
			DXSM::Vector3 color;
			float pad;
			int hit;
			DXSM::Vector3 pad3_0;
			int depth;
			DXSM::Vector3 pad3_1;
		};
		_shaderConfig.Init(8, sizeof(RayPayload));
		_subObjects.push_back(_shaderConfig.subObject);
		
		// シェーダー設定とシェーダーの関連付け
		uint32_t _shaderConfigIndex = _subObjects.size() - 1;
		BuildSubObjectHelper::ExportAssociationSubObject _configAssociationSO;
		_configAssociationSO.Init(a_init.shaderDataVec, &_subObjects[_shaderConfigIndex]);
		_subObjects.push_back(_configAssociationSO.subObject);
		
		// パイプライン設定のオブジェクトを作成
		BuildSubObjectHelper::PipelineConfigSubObject _config(a_init.maxRecursionDepth);
		_subObjects.push_back(_config.subObjcet);
		
		// グローバルルートシグネチャのサブオブジェクト作成
		if (a_init.opGlobalRootSigInit.has_value())
		{
			m_rootSig.Create(a_init.opGlobalRootSigInit.value_or(RootSigInit()));
		}
		else
		{
			m_rootSig.Create(RootSigInit());
		}
		BuildSubObjectHelper::GlobalRootSignatureSubObject _gRootSig;
		_gRootSig.Init(m_rootSig.Get());
		_subObjects.push_back(_gRootSig.subObject);
		
		// パイプライン作成
		D3D12_STATE_OBJECT_DESC _desc;
		_desc.NumSubobjects = static_cast<UINT>(_subObjects.size());
		_desc.pSubobjects = _subObjects.data();
		_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		auto _hr = _pDevice5->CreateStateObject(&_desc, IID_PPV_ARGS(&m_cpPSO));
		if (FAILED(_hr))
		{
			assert(0 && "レイトレ用パイプラインステートの作成に失敗");
			return false;
		}

		return true;
	}

	bool RayPSO::Init(const RayPSODesc& a_desc)
	{
		// エラー出力
		auto* _pDevice5 = D3D12Wrapper::Instance().GetDevice5();
		ComPtr<ID3D12InfoQueue> infoQueue;
		_pDevice5->QueryInterface(IID_PPV_ARGS(&infoQueue));

		// 変数用意
		std::vector<D3D12_STATE_SUBOBJECT> _subObjects;
		_subObjects.reserve(14);

		// DXILライブラリを用いてシェーダーをロード
		m_shader.Load(a_desc.shaderPass);

		// DXIL Libraryを登録
		BuildSubObjectHelper::DXILLibrarySubObject _dxilLibSO;
		_dxilLibSO.Init(m_shader.GetIDxcBlob(), a_desc.shaderDataVec);
		_subObjects.push_back(_dxilLibSO.subObject);

		// ヒットグループのサブオブジェクトの作成
		std::vector<BuildSubObjectHelper::HitGroupSubObject> _hitGroupSOVce;
		_hitGroupSOVce.resize(a_desc.hitGroupVec.size());
		for (int _i = 0; _i < a_desc.hitGroupVec.size(); ++_i)
		{
			_hitGroupSOVce[_i].Init(a_desc.hitGroupVec[_i]);
			_subObjects.push_back(_hitGroupSOVce[_i].subObject);
		}


		// ルートシグネチャとシェーダーの関連付けを行うサブオブジェクトを作っていく
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
					a_rsSO.Init(a_desc.pRayGenRootSig);
				}
				if (a_rootSig == LocalRootSignature::PBRMaterialHit)
				{
					a_rsSO.Init(a_desc.pHitRootSig);
				}
				if (a_rootSig == LocalRootSignature::Empty)
				{
					a_rsSO.Init(a_desc.pMissRootSig);
				}
				_subObjects.push_back(a_rsSO.subObject);
				uint32_t _rgSOIndex = _subObjects.size() - 1;

				int _useRootSig = 0;
				for (auto& _shaderData : a_desc.shaderDataVec)
				{
					if (_shaderData.rootsigType == a_rootSig)
					{
						a_exportNames[_useRootSig] = _shaderData.entryName;
						_useRootSig++;
					}
				}
				a_eaSO.Init(a_exportNames, _useRootSig, &(_subObjects[_rgSOIndex]));
				_subObjects.push_back(a_eaSO.subObject);
			};
		BuildSubObjectHelper::LocalRootSignatureSubObject _rayGenSigSO, _modelSigSO, _emptySigSO;
		BuildSubObjectHelper::ExportAssociationSubObject _rayGenAssSO, _modelAssSO, _emptyAssSO;
		const WCHAR* _rayGenExportName[5];
		const WCHAR* _modelExportName[5];
		const WCHAR* _emptyExportName[5];
		// レイジェネレーションシェーダーにルートシグネチャがあれば関連付ける
		if (a_desc.pRayGenRootSig)
		{
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_rayGenSigSO, _rayGenAssSO, LocalRootSignature::RayGen, _rayGenExportName);
		}
		// ヒットシェーダーにルートシグネチャがあれば関連付ける
		if(a_desc.pHitRootSig)
		{
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_modelSigSO, _modelAssSO, LocalRootSignature::PBRMaterialHit, _modelExportName);
		}
		// ミスシェーダーにルートシグネチャがあれば関連付ける
		if(a_desc.pMissRootSig)
		{
			BuildAndRegistRootSignatureAndAssSubObjectFunc(
				_emptySigSO, _emptyAssSO, LocalRootSignature::Empty, _emptyExportName);
		}

		// シェーダー設定(ペイロード)
		BuildSubObjectHelper::ShaderConfigSubObject _shaderConfig;
		struct RayPayload
		{
			DXSM::Vector3 color;
			float pad;
			int hit;
			DXSM::Vector3 pad3_0;
			int depth;
			DXSM::Vector3 pad3_1;
		};
		_shaderConfig.Init(8, sizeof(RayPayload));
		_subObjects.push_back(_shaderConfig.subObject);

		// シェーダー設定とシェーダーの関連付け
		uint32_t _shaderConfigIndex = _subObjects.size() - 1;
		BuildSubObjectHelper::ExportAssociationSubObject _configAssociationSO;
		_configAssociationSO.Init(a_desc.shaderDataVec, &_subObjects[_shaderConfigIndex]);
		_subObjects.push_back(_configAssociationSO.subObject);

		// パイプライン設定のオブジェクトを作成
		BuildSubObjectHelper::PipelineConfigSubObject _config(a_desc.maxRecursionDepth);
		_subObjects.push_back(_config.subObjcet);

		// グローバルルートシグネチャのサブオブジェクト作成
		BuildSubObjectHelper::GlobalRootSignatureSubObject _gRootSig;
		_gRootSig.Init(a_desc.pGlobalRootSig);
		_subObjects.push_back(_gRootSig.subObject);

		// パイプライン作成
		D3D12_STATE_OBJECT_DESC _desc;
		_desc.NumSubobjects = static_cast<UINT>(_subObjects.size());
		_desc.pSubobjects = _subObjects.data();
		_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		auto _hr = _pDevice5->CreateStateObject(&_desc, IID_PPV_ARGS(&m_cpPSO));
		if (FAILED(_hr))
		{
			assert(0 && "レイトレ用パイプラインステートの作成に失敗");
			return false;
		}

		return true;
	}

	const void* Engine::Raytracing::RayPSO::GetShaderID(const std::string& a_shaderEntry) const
	{
		ComPtr<ID3D12StateObjectProperties> _props;
		m_cpPSO.As(&_props);

		std::wstring _entry = StringUtility::ToWideString(a_shaderEntry);

		return _props->GetShaderIdentifier(_entry.c_str());
	}
	const void* RayPSO::GetShaderID(const const wchar_t* a_shaderEntry) const
	{
		ComPtr<ID3D12StateObjectProperties> _props;
		m_cpPSO.As(&_props);

		return _props->GetShaderIdentifier(a_shaderEntry);
	}
}