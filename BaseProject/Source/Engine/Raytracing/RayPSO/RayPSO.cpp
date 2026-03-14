#include "RayPSO.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

void Engine::Raytracing::RayPSO::Init()
{
	std::array<D3D12_STATE_SUBOBJECT, 14> _subObjects;
	uint32_t _index = 0;
	std::wstring _filePath = L"Asset/Shader/Ray/Raytracing.hlsl";

	// DXILライブラリを作成
	// レイトレーシング用のシェーダーをロード
	std::ifstream _shaderFile(_filePath.c_str());
	if (_shaderFile.good() == false)
	{
		std::wstring _errorMsg = L"シェーダーファイルが開けません\n";
		MessageBoxW(nullptr, _errorMsg.c_str(), L"エラー", MB_OK);
		std::abort();
	}

	std::stringstream _strStream;
	_strStream << _shaderFile.rdbuf();
	std::string _shader = _strStream.str();

	// シェーダーのテキストファイルから、BLOBを作成する
	ComPtr<IDxcLibrary> _dxclib;
	auto _hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&_dxclib));
	if (FAILED(_hr))
	{
		assert(0 && "DxCLIBの作成に失敗しました");
		return;
	}
	ComPtr<IDxcIncludeHandler> _incHandler;
	_hr = _dxclib->CreateIncludeHandler(&_incHandler);
	if (FAILED(_hr))
	{
		assert(0 && "CreateIncludeHandlerに失敗しました");
		return;
	}

	// dxcコンパイラの作成
	ComPtr<IDxcCompiler> _dxcCompiler;
	_hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_dxcCompiler));
	if (FAILED(_hr))
	{
		assert(0 && "dxcコンパイラの作成に失敗しました");
		return;
	}

	// ソースコードのBLOBを作成する
	uint32_t _codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> _sourceBlob;
	_hr = _dxclib->CreateBlobFromFile(_filePath.c_str(), &_codePage, &_sourceBlob);
	if (FAILED(_hr))
	{
		assert(0 && "シェーダーソースのBlobの作成に失敗しました");
		return;
	}

	ComPtr<IDxcIncludeHandler> _dxcIncHandler;
	_dxclib->CreateIncludeHandler(&_dxcIncHandler);
	const wchar_t* _args[] = {
		L"-I Asset\\Shader\\Ray"
	};

	// コンパイル
	ComPtr<IDxcOperationResult> _result;
	_hr = _dxcCompiler->Compile(
		_sourceBlob.Get(),			// ソースポインタ
		_filePath.c_str(),			// ソースの名前
		L"",						// エントリーポイント
		L"lib_6_3",					// ターゲットプロファイル
		_args, 1,					// アーギュメント、アーギュメント数
		nullptr, 0,					// デファイン
		_dxcIncHandler.Get(),		// インクルードハンドル
		_result.GetAddressOf()		// 出力先
	);

	// コンパイル結果取得
	HRESULT _status;
	_result->GetStatus(&_status);
	if (FAILED(_status))
	{
		ComPtr<IDxcBlobEncoding> _errors;
		_result->GetErrorBuffer(&_errors);
		if (_errors)
		{
			assert(0 && "エラー");
			return;
		}

		assert(false);
	}
	else
	{
		_result->GetResult(&m_dxcBlob);
	}

	D3D12_EXPORT_DESC exports[] =
	{
		{ L"RayGen",      nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"Miss",        nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"ClosestHit",  nullptr, D3D12_EXPORT_FLAG_NONE }
	};


	// DXIL Libraryを登録
	D3D12_DXIL_LIBRARY_DESC _dxilLibDesc = {};
	_dxilLibDesc.DXILLibrary.BytecodeLength = m_dxcBlob->GetBufferSize();
	_dxilLibDesc.DXILLibrary.pShaderBytecode = m_dxcBlob->GetBufferPointer();
	_dxilLibDesc.NumExports = _countof(exports);
	_dxilLibDesc.pExports = exports;

	D3D12_STATE_SUBOBJECT _dxilibSubobject = {};
	_dxilibSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	_dxilibSubobject.pDesc = &_dxilLibDesc;

	_subObjects[_index++] = _dxilibSubobject;

	D3D12_HIT_GROUP_DESC _hitGroupDesc = {};
	_hitGroupDesc.ClosestHitShaderImport = L"ClosestHit";
	_hitGroupDesc.HitGroupExport = L"HitGroup";
	_hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

	D3D12_STATE_SUBOBJECT _hitGroupSubobject = {};
	_hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	_hitGroupSubobject.pDesc = &_hitGroupDesc;

	_subObjects[_index++] = _hitGroupSubobject;

	// シェーダー設定
	// ペイロードサイズ
	D3D12_RAYTRACING_SHADER_CONFIG _shaderConfig = {};
	_shaderConfig.MaxPayloadSizeInBytes = 32;
	_shaderConfig.MaxAttributeSizeInBytes = 8;

	D3D12_STATE_SUBOBJECT _shaderConfigSubobject = {};
	_shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	_shaderConfigSubobject.pDesc = &_shaderConfig;

	_subObjects[_index] = _shaderConfigSubobject;
	auto* _shaderConfigPtr = &_subObjects[_index];
	_index++;

	const wchar_t* _exports[] =
	{
		L"RayGen",
		L"Miss",
		L"ClosestHit"
	};

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
	association.NumExports = _countof(_exports);
	association.pExports = _exports;
	association.pSubobjectToAssociate = _shaderConfigPtr;

	D3D12_STATE_SUBOBJECT associationSubobject = {};
	associationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	associationSubobject.pDesc = &association;

	_subObjects[_index++] = associationSubobject;

	// ルートシグネチャ
	m_rootSig.Create(
		{
			{RootParameterType::RootCBV,{}},
			{RootParameterType::DescriptorTable,{RangeType::SRV}},
			{RootParameterType::DescriptorTable,{RangeType::UAV}}
		}
	);

	ID3D12RootSignature* _pRootSig = m_rootSig.Get();

	D3D12_STATE_SUBOBJECT _rootSig = {};
	_rootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	_rootSig.pDesc = &_pRootSig;

	_subObjects[_index++] = _rootSig;

	// パイプラインコンフィグ
	// 再帰回数などの設定。絶対に０にはしない。落ちちゃう
	D3D12_RAYTRACING_PIPELINE_CONFIG _pipelineConfig = {};
	_pipelineConfig.MaxTraceRecursionDepth = 1;
	
	D3D12_STATE_SUBOBJECT _pipelineSubObject = {};
	_pipelineSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	_pipelineSubObject.pDesc = &_pipelineConfig;

	_subObjects[_index++] = _pipelineSubObject;

	// StateObuject生成
	D3D12_STATE_OBJECT_DESC _stateObjectDesc = {};
	_stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	_stateObjectDesc.NumSubobjects = _index;
	_stateObjectDesc.pSubobjects = _subObjects.data();

	auto _pDevice5 = D3D12Wrapper::Instance().GetDevice5();

	_hr = _pDevice5->CreateStateObject(
		&_stateObjectDesc,
		IID_PPV_ARGS(&m_cpPSO)
	);
	assert(SUCCEEDED(_hr) && "レイトレーシング用PSOの作成に失敗");
}

void* Engine::Raytracing::RayPSO::GetShaderID(const std::string& a_shaderEntry)
{
	ComPtr<ID3D12StateObjectProperties> _props;
	m_cpPSO.As(&_props);

	std::wstring _entry = StringUtility::ToWideString(a_shaderEntry);

	return _props->GetShaderIdentifier(_entry.c_str());
}
