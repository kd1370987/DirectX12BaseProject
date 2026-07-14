#include "RasterizerImport.h"
#include "../../DXCCompiler/DXCCompiler.h"

ComPtr<ID3DBlob> Engine::Resource::RequestShader(
	const std::string& a_path,
	const wchar_t* a_plofileVersion, 
	std::vector<LPCWSTR> a_setting
)
{
	namespace fs = std::filesystem;

	// パスの正規化（.hlsl と .cso の両方のパスを割り出す）
	fs::path _inputPath(a_path);
	fs::path _hlslPath = _inputPath;
	fs::path _csoPath = _inputPath;

	_hlslPath.replace_extension(".hlsl");
	_csoPath.replace_extension(".cso");

	std::string _fileName = _inputPath.stem().string();

	// コンパイルが必要かどうかの判定フラグ
	bool _needsCompile = false;

	if (!fs::exists(_csoPath))
	{
		// CSOが存在しない場合は絶対コンパイルが必要
		_needsCompile = true;
	}
	else if (fs::exists(_hlslPath))
	{
		// .hlsl 自体のタイムスタンプを取得
		auto _hlslTime = fs::last_write_time(_hlslPath);

		// インクルードディレクトリ内の最新の .hlsli のタイムスタンプを取得
		auto _newestHlsliTime = fs::file_time_type::min();
		std::vector<fs::path> _includeDirs = {
			"Asset\\Shader\\Common",
			"Asset\\Shader\\Mesh"
		};

		for (const auto& _dir : _includeDirs)
		{
			if (!fs::exists(_dir)) continue;
			for (const auto& _entry : fs::recursive_directory_iterator(_dir))
			{
				if (_entry.is_regular_file() && _entry.path().extension() == ".hlsli")
				{
					auto _time = fs::last_write_time(_entry);
					if (_time > _newestHlsliTime)
					{
						_newestHlsliTime = _time;
					}
				}
			}
		}

		// 比較するタイムスタンプを決定（.hlsl と最新の .hlsli で新しい方）
		auto _targetTime = std::max(_hlslTime, _newestHlsliTime);
		auto _csoTime = fs::last_write_time(_csoPath);

		if (_targetTime > _csoTime)
		{
			// HLSL または 依存する hlsli の方が新しければ再コンパイル
			_needsCompile = true;
			ENGINE_LOG("シェーダー（またはヘッダー）の更新を検知。再コンパイルします: %s", _fileName.c_str());
		}
	}

	// CSOが最新なら、そのままロードして返す
	if (!_needsCompile && fs::exists(_csoPath))
	{
		ComPtr<ID3DBlob> _cpBlob;
		std::wstring _wstrCso = StringUtility::ToWideString(_csoPath.string());

		if (SUCCEEDED(D3DReadFileToBlob(_wstrCso.c_str(), &_cpBlob)))
		{
			return _cpBlob;
		}
		// 読み込みに失敗した場合は念のためコンパイルにフォールバック
		_needsCompile = true;
		ENGINE_LOG("シェーダーの読み込みに失敗。再コンパイルします: %s", _fileName.c_str());
	}

	// ==========================================
	// DXCによるコンパイル処理

	// HLSLチェック
	if (!fs::exists(_hlslPath))
	{
		ENGINE_ERRLOG(false,"HLSL,CSOファイルがともに見つかりません : %s",a_path.c_str());
		return nullptr;
	}

	// ファイル名からシェーダーの種類を推論する
	std::wstring _profile = L"ps_6_5";	// デフォルト
	std::wstring _entryPoint = L"main";

	if (_fileName.length() >= 2)
	{
		std::string _suffix = _fileName.substr(_fileName.length() - 2);
		if (_suffix == "MS") { _profile = std::wstring(L"ms_") + a_plofileVersion; _entryPoint = L"MSMain"; }
		else if (_suffix == "AS") { _profile = std::wstring(L"as_") + a_plofileVersion; _entryPoint = L"ASMain"; }
		else if (_suffix == "VS") { _profile = std::wstring(L"vs_") + a_plofileVersion; _entryPoint = L"VSMain"; }
		else if (_suffix == "PS") { _profile = std::wstring(L"ps_") + a_plofileVersion; _entryPoint = L"PSMain"; }
		else if (_suffix == "CS") { _profile = std::wstring(L"cs_") + a_plofileVersion; _entryPoint = L"CSMain"; }
		else if (_suffix == "LB") { _profile = std::wstring(L"lib_") + a_plofileVersion; _entryPoint = L""; }
	}

	// コンパイル引数の設定
	std::vector<LPCWSTR> _args = {};
	if(a_setting.empty())
	{
		_args = {
			L"-I", L"Asset\\Shader\\Common", // 共通ヘッダーのインクルードパス
			L"-I", L"Asset\\Shader\\Mesh",   // メッシュ用パス
			L"-Zi",                          // デバッグ情報
			L"-Qembed_debug"                 // PDB埋め込み
		};
	}
	else
	{
		_args = a_setting;
	}

	// 前に作った DXCコンパイラの呼び出し
	auto _compiledBlob = Engine::Resource::Compiler::ShaderCompile(
		_hlslPath.string(),
		_args,
		_profile.c_str(),
		_entryPoint.c_str()
	);
	if (!_compiledBlob)
	{
		ENGINE_ERRLOG(false, "シェーダーのコンパイルに失敗しました : %s",a_path.c_str());
		return nullptr;
	}

	// ==========================================
	// コンパイル結果を .cso としてファイルに保存
	std::ofstream _outFile(_csoPath, std::ios::binary);
	if (_outFile.is_open())
	{
		_outFile.write(
			reinterpret_cast<const char*>(_compiledBlob->GetBufferPointer()),
			_compiledBlob->GetBufferSize()
		);
		_outFile.close();
		ENGINE_LOG("CSOキャッシュを作成しました: %s", _csoPath.string().c_str());
	}
	else
	{
		ENGINE_ERRLOG(false, "CSOファイルの保存に失敗: %s", _csoPath.string().c_str());
	}

	// DXCが返す IDxcBlob は ID3DBlob と互換性がないため、
	// IDxcBlob の中身を ID3DBlob としてメモリにコピーして返す
	ComPtr<ID3DBlob> _finalBlob;
	D3DCreateBlob(_compiledBlob->GetBufferSize(), &_finalBlob);
	memcpy(_finalBlob->GetBufferPointer(), _compiledBlob->GetBufferPointer(), _compiledBlob->GetBufferSize());

	return _finalBlob;
}

namespace Engine::Resource::Import
{
	ComPtr<ID3DBlob> Engine::Resource::Import::CompileShader(const std::string& a_path)
	{
		// ファイルパスから拡張子を除いたファイル名を抽出
		// 例: "Asset/Shader/Test_MS.cso" -> "Test_MS"
		std::filesystem::path _fsPath(a_path);
		std::string _fileName = _fsPath.stem().string();

		// ファイル名の末尾が "MS" で終わっているかチェック
		//if (_fileName.length() >= 2 && _fileName.substr(_fileName.length() - 2) == "MS")
		//{
		//	std::vector<LPCWSTR> _msArgs = {
		//		L"-I", L"Asset\\Shader\\Mesh", // インクルードパス
		//		L"-Zi",                        // デバッグ情報を含める
		//		L"-Qembed_debug"              // PDBを埋め込む（VS警告対策）
		//		//L"-O3"                         // 最適化レベル（リリース時）
		//	};

		//	auto _msBlob = Engine::Resource::Compiler::ShaderCompile(
		//		a_path,
		//		_msArgs,
		//		L"ms_6_5",
		//		L"MSMain"
		//	);
		//	ENGINE_LOG("MS Size = %llu", _msBlob->GetBufferSize());
		//}

		// ==========================================
		// 以下、通常のシェーダー（VS/PS等）の読み込み処理
		// ==========================================
		ComPtr<ID3DBlob> _cpBlob = {};

		// 文字列変換
		std::wstring _wstr = StringUtility::ToWideString(a_path);

		auto _hr = D3DReadFileToBlob(
			_wstr.c_str(),
			_cpBlob.ReleaseAndGetAddressOf()
		);

		if (FAILED(_hr))
		{
			assert(0 && "シェーダーの読み込みに失敗");
			return _cpBlob;
		}

		return _cpBlob;
	}
	D3D12_SHADER_BYTECODE CreateShaderByteCode(ID3DBlob* a_pBlob)
	{
		return CD3DX12_SHADER_BYTECODE(a_pBlob);
	}
	EShaderStage ReflectShaderStage(const std::string& a_path)
	{
		// .cso ファイルをバイナリとして読み込む
		std::ifstream _file(a_path, std::ios::binary | std::ios::ate);
		if (!_file.is_open())
		{
			// ファイル読み込み失敗時の処理
			ENGINE_ERRLOG(false,"シェーダーファイルの読み込みに失敗");
			return EShaderStage::Unknown;
		}

		size_t _fileSize = (size_t)_file.tellg();
		_file.seekg(0, std::ios::beg);

		std::vector<char> _fileData(_fileSize);
		_file.read(_fileData.data(), _fileSize);
		_file.close();

		// D3DReflect を使ってシェーダーの情報を覗き見る
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> _pReflector;
		HRESULT _hr = D3DReflect(
			_fileData.data(),
			_fileData.size(),
			IID_PPV_ARGS(_pReflector.GetAddressOf())
		);

		if (FAILED(_hr))
		{
			// リフレクション失敗時
			ENGINE_ERRLOG(false, "シェーダーファイルのリフレクション読み込みに失敗");
			return EShaderStage::Unknown;
		}

		// シェーダーの詳細情報を取得
		D3D12_SHADER_DESC _desc = {};
		_pReflector->GetDesc(&_desc);

		// Version情報からシェーダータイプを抽出するマクロを使用
		UINT _shaderType = D3D12_SHVER_GET_TYPE(_desc.Version);

		// EShaderStage に変換
		switch (_shaderType)
		{
		case D3D12_SHVER_VERTEX_SHADER:   return EShaderStage::VS;
		case D3D12_SHVER_PIXEL_SHADER:    return EShaderStage::PS;
		case D3D12_SHVER_GEOMETRY_SHADER: return EShaderStage::GS;
		case D3D12_SHVER_HULL_SHADER:     return EShaderStage::HS;
		case D3D12_SHVER_DOMAIN_SHADER:   return EShaderStage::DS;
		case D3D12_SHVER_COMPUTE_SHADER:  return EShaderStage::CS;
		default:                          return EShaderStage::Unknown;
		}
	}

}
