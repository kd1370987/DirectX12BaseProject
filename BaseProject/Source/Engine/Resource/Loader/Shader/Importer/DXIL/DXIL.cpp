#include "DXIL.h"

ComPtr<IDxcBlob> Engine::Resource::Import::DXIL(const std::string& a_path)
{
	std::wstring _filePath = StringUtility::ToWideString(a_path);
	ComPtr<IDxcBlob> _resultBlob;

	// DXILライブラリを作成
	// シェーダーのテキストファイルから、BLCBを作成
	ComPtr<IDxcLibrary> _dxclib;
	HRESULT _hr = DxcCreateInstance(CLSID_DxcLibrary,IID_PPV_ARGS(&_dxclib));
	if (FAILED(_hr))
	{
		assert(0 && "DxCLIBの作成に失敗しました");
		return nullptr;
	}

	// DXCコンパイラの作成
	ComPtr<IDxcCompiler> _dxcCompiler;
	_hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_dxcCompiler));
	if (FAILED(_hr))
	{
		assert(0 && "DXCコンパイラの作成に失敗しました");
		return nullptr;
	}

	// インクルードハンドル作成
	ComPtr<IDxcIncludeHandler> _incHandler;
	_hr = _dxclib->CreateIncludeHandler(&_incHandler);
	if (FAILED(_hr))
	{
		assert(0 && "IncludeHandler作成に失敗しました");
		return nullptr;
	}

	// ソースコードのBLOBを作成する
	uint32_t _codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> _sourceBlob;
	_hr = _dxclib->CreateBlobFromFile(_filePath.c_str(), &_codePage, &_sourceBlob);
	if (FAILED(_hr))
	{
		assert(0 && "シェーダーソースのBlobの作成に失敗しました");
		return nullptr;
	}

	// コンパイル時に渡す命令文
	const wchar_t* _args[] = {
		L"-I", L"Asset\\Shader\\Ray", // exeに渡すインクルードディレクトリ
		L"-Zi",
		L"-Qembed_debug"
	};

	// コンパイル
	ComPtr<IDxcOperationResult> _result;
	_hr = _dxcCompiler->Compile(
		_sourceBlob.Get(),					// データ
		_filePath.c_str(),					// ファイルネーム
		L"",								// エントリーポインタ
		L"lib_6_6",							// ターゲットプロファイル
		_args,								// アーギュメントデータ
		_countof(_args),					// アーギュメント数
		nullptr,							// デファイン
		0,									// アーギュメント数
		_incHandler.Get(),					// インクルードハンドル
		_result.ReleaseAndGetAddressOf()	// 出力先
	);

	_result->GetStatus(&_hr);

	if (FAILED(_hr))
	{
		if (_result)
		{
			ComPtr<IDxcBlobEncoding> _errorBlob;
			_hr = _result->GetErrorBuffer(&_errorBlob);
			if (SUCCEEDED(_hr) && _errorBlob)
			{
				assert(0 && "シェーダーライブラリコンパイルエラー");
				return nullptr;
			}
		}
	}	
	_result->GetResult(&_resultBlob);
	

	return _resultBlob;
}
