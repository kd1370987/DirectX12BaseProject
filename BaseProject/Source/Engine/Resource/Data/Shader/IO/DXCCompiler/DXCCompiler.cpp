#include "DXCCompiler.h"
namespace Engine::Resource::Compiler
{
	ComPtr<IDxcBlob> ShaderCompile(
		const std::string& a_path, 
		std::vector<LPCWSTR>& a_args,
		const wchar_t* a_targetProfile,
		const wchar_t* a_entoryPointName
	)
	{
		if (a_path.empty()) return nullptr;

		// 変数準備
		std::wstring _filePath = StringUtility::ToWideString(a_path);
		ComPtr<IDxcBlob> _cpResultBlob;

		// DXILライブラリを作成 : シェーダーのテキストファイルから Blob 作成
		ComPtr<IDxcLibrary> _cpDxcLib;
		auto _hr = DxcCreateInstance(CLSID_DxcLibrary,IID_PPV_ARGS(&_cpDxcLib));
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "DxcLIBの作成に失敗 : %s", a_path.c_str());
			return nullptr;
		}

		// DXCコンパイラの作成
		ComPtr<IDxcCompiler> _cpDxcCompiler;
		_hr = DxcCreateInstance(CLSID_DxcCompiler,IID_PPV_ARGS(&_cpDxcCompiler));
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "DXCコンパイラの作成に失敗 : %s", a_path.c_str());
			return nullptr;
		}

		// インクルードハンドル作成
		ComPtr<IDxcIncludeHandler> _cpIncHandler;
		_hr = _cpDxcLib->CreateIncludeHandler(&_cpIncHandler);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "IncludeHandlerの作成に失敗 : %s", a_path.c_str());
			return nullptr;
		}

		// ソースコードのBlobを作成
		uint32_t _codePage = CP_UTF8;
		ComPtr<IDxcBlobEncoding> _cpSourceBlob;
		_hr = _cpDxcLib->CreateBlobFromFile(_filePath.c_str(), &_codePage, &_cpSourceBlob);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false, "シェーダーソースのBlobの作成に失敗 : %s", a_path.c_str());
			return nullptr;
		}

		// コンパイル
		ComPtr<IDxcOperationResult> _cpResult;
		_hr = _cpDxcCompiler->Compile(
			_cpSourceBlob.Get(),					// データ
			_filePath.c_str(),					// ファイルネーム
			a_entoryPointName,								// エントリーポインタ
			a_targetProfile,							// ターゲットプロファイル
			a_args.data(),								// アーギュメントデータ
			static_cast<UINT32>(a_args.size()),					// アーギュメント数
			nullptr,							// デファイン
			0,									// アーギュメント数
			_cpIncHandler.Get(),					// インクルードハンドル
			_cpResult.ReleaseAndGetAddressOf()	// 出力先
		);

		_cpResult->GetStatus(&_hr);

		if (FAILED(_hr))
		{
			if (_cpResult)
			{
				ComPtr<IDxcBlobEncoding> _errorBlob;
				_hr = _cpResult->GetErrorBuffer(&_errorBlob);
				if (SUCCEEDED(_hr) && _errorBlob)
				{
					const char* _pErrorMsg = static_cast<const char*>(_errorBlob->GetBufferPointer());
					ENGINE_ERRLOG(false, "シェーダーコンパイルエラー:\n%s", _pErrorMsg);
					return nullptr;
				}
			}
		}
		_cpResult->GetResult(&_cpResultBlob);


		return _cpResultBlob;
	}
}