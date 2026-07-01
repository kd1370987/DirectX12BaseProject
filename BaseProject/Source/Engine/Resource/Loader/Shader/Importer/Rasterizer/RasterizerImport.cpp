#include "RasterizerImport.h"

namespace Engine::Resource::Import
{
	ComPtr<ID3DBlob> Engine::Resource::Import::CompileShader(const std::string& a_path)
	{
		ComPtr<ID3DBlob> _cpBlob = {};

		// 文字列変換
		std::wstring _wstr = StringUtility::ToWideString(a_path);

		// シェーダー読み込み
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
