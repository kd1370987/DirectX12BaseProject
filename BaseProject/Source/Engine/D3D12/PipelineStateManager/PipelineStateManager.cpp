#include "PipelineStateManager.h"
namespace Engine::D3D12
{
	void PipelineStateManager::Init(D3D12::Device* a_pDevice)
	{
		m_pDevice = a_pDevice;
		m_rootSigMap = {};
		m_psoMap = {};
	}
	void PipelineStateManager::Release()
	{
		m_pDevice = nullptr;

		// ルートシグネチャ解放
		for (auto& [_hash, _cpRootSig] : m_rootSigMap)
		{
			if (_cpRootSig)
			{
				_cpRootSig.Reset();
			}
		}

		// PSO解放
		m_pPsoVec.clear();
		for (auto& [_hash, _cpPSO] : m_psoMap)
		{
			if (_cpPSO)
			{
				_cpPSO.Reset();
			}
		}

	}
	ID3D12RootSignature* PipelineStateManager::Request(const D3D12::RootSignatureDesc& a_desc)
	{
		// ハッシュを求める
		uint64_t _hash = CalcHash(a_desc);

		// キャッシュ検索
		auto _it = m_rootSigMap.find(_hash);
		if(_it != m_rootSigMap.end())
		{
			return _it->second.Get();
		}

		// キャッシュになければ作成
		ComPtr<ID3D12RootSignature> _rootSig = D3D12::RootSignatureBuilder::Create(a_desc);

		// 名前があれば
		if (!a_desc.name.empty())
		{
			_rootSig->SetName(StringUtility::ToWideString(a_desc.name).c_str());
		}

		// マップに保存して返す
		m_rootSigMap[_hash] = _rootSig;
		return _rootSig.Get();
	}
	ID3D12RootSignature* PipelineStateManager::Request(const std::string& a_shaderPath)
	{
		// バイナリデータ
		ComPtr<ID3DBlob> _cpBlob = nullptr;
		ComPtr<ID3DBlob> _cpRootSigBlob = nullptr;

		// シェーダーバイトコードの読み込み
		auto _hr = D3DReadFileToBlob(
			StringUtility::ToWideString(a_shaderPath).c_str(),
			_cpBlob.ReleaseAndGetAddressOf()
		);
		if (FAILED(_hr))
		{
			assert(0 && "ルートシグネチャ生成用のシェーダーファイル読み込みに失敗");
			return nullptr;
		}

		// シェーダーバイトコードの読み込み
		_hr = D3DGetBlobPart(
			_cpBlob->GetBufferPointer(),
			_cpBlob->GetBufferSize(),
			D3D_BLOB_ROOT_SIGNATURE,
			0,
			&_cpRootSigBlob
		);
		if (SUCCEEDED(_hr))
		{
			// シェーダーからルートシグネチャの抽出に成功
			// ルートシグネチャの部分からハッシュ値を求める
			uint64_t _hash = CalcHash((void*)_cpRootSigBlob->GetBufferPointer(), _cpRootSigBlob->GetBufferSize());

			// すでに構築されているルートシグネチャならポインタを返す
			auto _it = m_rootSigMap.find(_hash);
			if (_it != m_rootSigMap.end())
			{
				return _it->second.Get();
			}

			// なければ生成
			ComPtr<ID3D12RootSignature> _rootSig = D3D12::RootSignatureBuilder::Create(_cpRootSigBlob);
			if (!_rootSig)
			{
				assert(0 && ".cso内にRootSignatureが見つかりませんでした");
				return nullptr;
			}

			// 名前を付ける
			_rootSig->SetName(StringUtility::ToWideString(a_shaderPath).c_str());

			// マップに保存して返す
			m_rootSigMap[_hash] = _rootSig;
			return _rootSig.Get();
		}
	}
	ID3D12PipelineState* PipelineStateManager::Request(const D3D12::GraphicsPipelineDesc& a_desc)
	{
		// ハッシュを求める
		uint64_t _hash = CalcHash(&a_desc.desc,sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		// キャッシュ検索
		if (m_psoMap.contains(_hash))
		{
			return m_psoMap[_hash].Get();
		}

		// キャッシュになければ、ComPtrを作成
		ComPtr<ID3D12PipelineState> _pso;
		auto _hr = m_pDevice->CreateGraphicsPipelineState(&a_desc.desc,IID_PPV_ARGS(&_pso));
		if (FAILED(_hr))
		{
			assert(0 && "PSOの作成失敗");
			return nullptr;
		}

		// 名前があれば
		if (!a_desc.name.empty())
		{
			_pso->SetName(StringUtility::ToWideString(a_desc.name).c_str());
		}

		// マップに保存して生ポインタを返す
		m_psoMap[_hash] = _pso;
		return _pso.Get();
	}

	ID3D12PipelineState* PipelineStateManager::Request(const D3D12::ComputePipelineDesc& a_desc)
	{
		// ハッシュを求める
		uint64_t _hash = CalcHash(&a_desc.desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		// キャッシュ検索
		if (m_psoMap.contains(_hash))
		{
			return m_psoMap[_hash].Get();
		}

		// キャッシュになければ、ComPtrを作成
		ComPtr<ID3D12PipelineState> _pso;
		auto _hr = m_pDevice->CreateComputePipelineState(&a_desc.desc, IID_PPV_ARGS(&_pso));
		if (FAILED(_hr))
		{
			assert(0 && "PSOの作成失敗");
			return nullptr;
		}

		// 名前があれば
		if (!a_desc.name.empty())
		{
			_pso->SetName(StringUtility::ToWideString(a_desc.name).c_str());
		}

		// マップに保存して生ポインタを返す
		m_psoMap[_hash] = _pso;
		return _pso.Get();
	}

	ID3D12PipelineState* PipelineStateManager::Request(const D3D12::MeshPipelineBuilder& a_builder)
	{
		// ハッシュ計算 (builder.desc の中身でハッシュを作る)
		uint64_t _hash = CalcHash(&a_builder.desc, sizeof(a_builder.desc));

		// キャッシュにあれば返す
		if (m_psoMap.contains(_hash)) {
			return m_psoMap[_hash].Get();
		}

		// ストリーム構造体の組み立て
		MeshShaderPipelineStateStream _streamDesc = {};
		_streamDesc.RootSignature = a_builder.desc.pRootSignature;
		_streamDesc.PrimitiveTopologyType = a_builder.desc.PrimitiveTopologyType;
		_streamDesc.MS = a_builder.desc.MS;
		_streamDesc.PS = a_builder.desc.PS;
		_streamDesc.Blend = CD3DX12_BLEND_DESC(a_builder.desc.BlendState);
		_streamDesc.Rasterizer = CD3DX12_RASTERIZER_DESC(a_builder.desc.RasterizerState);
		_streamDesc.DepthStencil = CD3DX12_DEPTH_STENCIL_DESC(a_builder.desc.DepthStencilState);
		// RTVフォーマットはコピーして渡す
		D3D12_RT_FORMAT_ARRAY _rtvFormatArray = {};
		_rtvFormatArray.NumRenderTargets = a_builder.desc.NumRenderTargets;
		for (UINT i = 0; i < a_builder.desc.NumRenderTargets; ++i)
		{
			_rtvFormatArray.RTFormats[i] = a_builder.desc.RTVFormats[i];
		}
		_streamDesc.RTVFormats = _rtvFormatArray;
		_streamDesc.DSVFormat = a_builder.desc.DSVFormat;

		// ストリーム構造体をDirectX側に登録
		D3D12_PIPELINE_STATE_STREAM_DESC _streamInfo = {};
		_streamInfo.SizeInBytes = sizeof(_streamDesc);
		_streamInfo.pPipelineStateSubobjectStream = &_streamDesc;

		ComPtr<ID3D12PipelineState> _pPso;
		HRESULT hr = m_pDevice->CreatePipelineState(&_streamInfo, IID_PPV_ARGS(&_pPso));

		if (FAILED(hr)) {
			ENGINE_ERRLOG(false,"メッシュシェーダー用パイプラインステートの生成に失敗");
			return nullptr;
		}

		// マップに登録して返す
		m_psoMap[_hash] = _pPso;
		return _pPso.Get();
	}

	Handle<ID3D12PipelineState> PipelineStateManager::RequestHandle(
		const D3D12::GraphicsPipelineDesc& a_desc
	)
	{
		auto _handle = m_psoHandlePool.Allocate();
		if (m_pPsoVec.size() <= _handle.GetIndex())
		{
			m_pPsoVec.resize(_handle.GetIndex() + 1);
		}
		m_pPsoVec[_handle.GetIndex()] = Request(a_desc);

		return _handle;
	}

	Handle<ID3D12PipelineState> PipelineStateManager::RequestHandle(
		const D3D12::ComputePipelineDesc& a_desc
	)
	{
		auto _handle = m_psoHandlePool.Allocate();
		if (m_pPsoVec.size() <= _handle.GetIndex())
		{
			m_pPsoVec.resize(_handle.GetIndex() + 1);
		}
		m_pPsoVec[_handle.GetIndex()] = Request(a_desc);

		return _handle;
	}

	Handle<ID3D12PipelineState> PipelineStateManager::RequestHandle(const D3D12::MeshPipelineBuilder& a_desc)
	{
		auto _handle = m_psoHandlePool.Allocate();
		if (m_pPsoVec.size() <= _handle.GetIndex())
		{
			m_pPsoVec.resize(_handle.GetIndex() + 1);
		}
		m_pPsoVec[_handle.GetIndex()] = Request(a_desc);

		return _handle;
	}

	ID3D12PipelineState* PipelineStateManager::GetPSO(Handle<ID3D12PipelineState> a_handle)
	{
		if (m_psoHandlePool.IsValid(a_handle))
		{
			return m_pPsoVec[a_handle.GetIndex()];
		}
		return nullptr;
	}

	ID3D12PipelineState* PipelineStateManager::GetPSO(uint8_t a_rawIdx8bit)
	{
		return m_pPsoVec[a_rawIdx8bit];
	}

	uint64_t PipelineStateManager::CalcHash(const void* a_pData, size_t a_size)
	{
		const uint8_t* _ptr = static_cast<const uint8_t*>(a_pData);
		uint64_t _hash = 14695981039346656037ull;
		for (size_t _i = 0; _i < a_size; ++_i)
		{
			_hash ^= _ptr[_i];
			_hash *= 1099511628211ull;
		}
		return _hash;
	}
	uint64_t PipelineStateManager::CalcHash(const D3D12::RootSignatureDesc& a_desc)
	{
		uint64_t _hash = 14695981039346656037ull; // FNV offset basis

		// ヘルパー：既存の CalcHash のロジックを再利用してハッシュを更新する
		auto UpdateHash = [&](const void* p, size_t size) {
			const uint8_t* ptr = static_cast<const uint8_t*>(p);
			for (size_t i = 0; i < size; ++i) {
				_hash ^= ptr[i];
				_hash *= 1099511628211ull; // FNV prime
			}
		};

		// 基本メンバを混ぜる
		UpdateHash(&a_desc.flags, sizeof(a_desc.flags));
		UpdateHash(&a_desc.isUseStaticSampler, sizeof(a_desc.isUseStaticSampler));

		// vector の中身をループして、実体データを混ぜる
		for (const auto& param : a_desc.paramVec) {
			// Paramそのものをハッシュ化
			UpdateHash(&param.paramType, sizeof(param.paramType));
			UpdateHash(&param.shaderRegisterIndex, sizeof(param.shaderRegisterIndex));

			// rangeVec も vector なので、さらにループして中身を混ぜる
			for (const auto& range : param.rangeVec) {
				UpdateHash(&range, sizeof(range)); // rangeInitの中身が単純なら一気にいける
			}
		}

		return _hash;
	}
}