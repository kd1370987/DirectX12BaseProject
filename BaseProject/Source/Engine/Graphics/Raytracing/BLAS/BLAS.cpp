#include "Engine/Graphics/Raytracing/BLAS/BLAS.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Raytracing/BLASCompactor/BLASCompactor.h"

// CD3DX12_* のヘルパーはここだけで使う。
// プリコンパイル済みヘッダーへ置くと全翻訳単位に広がるため
#pragma warning(push, 0)
#include "d3dx12.h"
#pragma warning(pop)

Engine::Graphics::Raytracing::BLAS::~BLAS()
{
	// Release() を通っていれば空なので何もしない
	DeferReleaseResources("デストラクタ");
}

Engine::Graphics::Raytracing::BLAS& Engine::Graphics::Raytracing::BLAS::operator=(BLAS&& a_other) noexcept
{
	if (this == &a_other) return *this;

	// 中身を持ったまま上書きされると、既定のムーブ代入では古いリソースがその場で消える
	DeferReleaseResources("ムーブ代入");

	m_cpResource = std::move(a_other.m_cpResource);
	m_spCompactionTarget = std::move(a_other.m_spCompactionTarget);
	m_cpUpdateScratch = std::move(a_other.m_cpUpdateScratch);
	m_geometryDescVec = std::move(a_other.m_geometryDescVec);
	m_isDynamic = a_other.m_isDynamic;

	a_other.m_isDynamic = false;
	return *this;
}

void Engine::Graphics::Raytracing::BLAS::DeferReleaseResources(const char* a_pUnexpected)
{
	// 圧縮の対象なら、共有している実体をこちらへ引き取る。
	// 空にしておけば、圧縮の途中でも BLASCompactor はそれを見て手を引く
	ComPtr<ID3D12Resource> _cpShared = nullptr;
	if (m_spCompactionTarget)
	{
		std::lock_guard<std::mutex> _lock(m_spCompactionTarget->mutex);
		_cpShared = std::move(m_spCompactionTarget->cpResource);
	}
	m_spCompactionTarget.reset();

	if (!m_cpResource && !m_cpUpdateScratch && !_cpShared) return;

	if (a_pUnexpected)
	{
		ENGINE_WARNING("[BLAS] Release() を通らずに%sで中身が捨てられました(%s)。遅延解放へ回します",
			a_pUnexpected, m_isDynamic ? "動的" : "静的");
	}

	// 終了処理でデバイスが片付いた後は GPU が止まっているので、その場で手放してよい。
	// (キューを掃く人がもう居ないので、積んでも誰も解放しない)
	auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
	if (!_pGE || !_pGE->RefRenderDevice()->RefDevice())
	{
		m_cpResource.Reset();
		m_cpUpdateScratch.Reset();
		return;
	}

	// TLASがこのBLASのGPUアドレスを参照したまま実行中の可能性があるため、
	// ここでは解放せず、ComPtrをゴミ箱にムーブして寿命だけを延ばす。
	// ラムダは中身が空でよく、キューがクリアされた時点でリソースが解放される
	MainEngine::Instance().ReserveRelease(
		[_cpResource = std::move(m_cpResource), _cpUpdateScratch = std::move(m_cpUpdateScratch), _cpShared = std::move(_cpShared)]() {}
	);

	// ムーブ済みだが、状態を明示的に空にしておく
	m_cpResource.Reset();
	m_cpUpdateScratch.Reset();
}

void Engine::Graphics::Raytracing::BLAS::Release()
{
	DeferReleaseResources(nullptr);

	m_geometryDescVec.clear();
	m_isDynamic = false;
}

void Engine::Graphics::Raytracing::BLAS::UAVBarrier(D3D12::GraphicsCommandList* a_pCmdList) const
{
	std::unique_lock<std::mutex> _lock = {};
	ID3D12Resource* _pResource = RefResultResource(&_lock);
	if (!_pResource) return;

	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	_barrier.UAV.pResource = _pResource;
	a_pCmdList->ResourceBarrier(1, &_barrier);
}

void Engine::Graphics::Raytracing::BLAS::SetName(LPCWSTR a_name)
{
	std::unique_lock<std::mutex> _lock = {};
	ID3D12Resource* _pResource = RefResultResource(&_lock);
	if (_pResource) _pResource->SetName(a_name);
}

D3D12_GPU_VIRTUAL_ADDRESS Engine::Graphics::Raytracing::BLAS::GetGPUAddress() const
{
	std::unique_lock<std::mutex> _lock = {};
	ID3D12Resource* _pResource = RefResultResource(&_lock);
	return _pResource ? _pResource->GetGPUVirtualAddress() : 0;
}

ID3D12Resource* Engine::Graphics::Raytracing::BLAS::RefResultResource(std::unique_lock<std::mutex>* a_pLock) const
{
	if (!m_spCompactionTarget) return m_cpResource.Get();

	*a_pLock = std::unique_lock<std::mutex>(m_spCompactionTarget->mutex);
	return m_spCompactionTarget->cpResource.Get();
}

bool Engine::Graphics::Raytracing::BLAS::BuildInternal(
	D3D12::Device* a_pDevice,
	D3D12::GraphicsCommandList* a_pCmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS a_buildFlags, 
	bool a_isUpdate
)
{
	// 組み直し : 前のBLASとスクラッチはまだGPUが使っているかもしれない。
	// 下の ReleaseAndGetAddressOf で上書きするとその場で最終解放されるので、先に逃がす
	DeferReleaseResources("組み直し");

	m_geometryDescVec = a_geometryDescVec;

	// スクラッチリソース構築
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS _inputs = {};
	_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_inputs.Flags = a_buildFlags;
	_inputs.NumDescs = static_cast<UINT>(a_geometryDescVec.size());
	_inputs.pGeometryDescs = a_geometryDescVec.data();
	_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	// BLASのサイズを問い合わせる
	// ResultDataMaxSizeBytes == BLASサイズ
	// ScratchDataSizeInBytes == ビルド用スクラッチ
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO _info;
	a_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&_inputs, &_info);

	// 動的な場合、初回ビルド用と更新用の大きいほうを採用する
	UINT64 _scratchSizeInBytes = a_isUpdate ?
		std::max(_info.ScratchDataSizeInBytes, _info.UpdateScratchDataSizeInBytes) :
		_info.ScratchDataSizeInBytes;

	// ヒーププロパティとバッファ設定（CD3DX12ヘルパーでスッキリ記述）
	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto _scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		_scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	// 生成
	a_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_scratchDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_cpUpdateScratch.ReleaseAndGetAddressOf())
	);
	if (m_cpUpdateScratch) m_cpUpdateScratch->SetName(L"BLAS_Scratch");	// リーク調査用
	D3D12::VideoMemoryTracker::TrackResource(m_cpUpdateScratch.Get(), D3D12::EVideoMemoryCategory::BLASScratch);

	auto barrierAS2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_cpUpdateScratch.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	a_pCmdList->ResourceBarrier(1, &barrierAS2);

	// BLASバッファ作成
	auto _blasDesc = CD3DX12_RESOURCE_DESC::Buffer(
		_info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);

	a_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_blasDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
	);
	if (m_cpResource) m_cpResource->SetName(L"BLAS_Result");	// リーク調査用
	D3D12::VideoMemoryTracker::TrackResource(m_cpResource.Get(), D3D12::EVideoMemoryCategory::BLAS);

	// Buildコマンド発行
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _buildDesc{};
	_buildDesc.Inputs = _inputs;
	_buildDesc.ScratchAccelerationStructureData = m_cpUpdateScratch->GetGPUVirtualAddress();
	_buildDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	a_pCmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);

	// UAVバリア
	D3D12_RESOURCE_BARRIER _barrier = {};
	_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	_barrier.UAV.pResource = m_cpResource.Get();

	a_pCmdList->ResourceBarrier(1, &_barrier);

	

	return true;
}

void Engine::Graphics::Raytracing::BLAS::CreateStatic(
	D3D12::Device* a_pDevice, 
	D3D12::GraphicsCommandList* a_pCmdList,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_geometryDescVec,
	const BLASStaticBuildOption& a_option
)
{
	// スタティックモデル
	m_isDynamic = false;

	// 圧縮できるのは、ビルドの完了を知らせてもらえて、圧縮の係がいるときだけ
	const bool _isCompactable = (a_option.pOnBuildComplete != nullptr && a_option.pCompactor != nullptr);
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS _flags = a_option.buildFlags;
	if (_isCompactable) _flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;

	// 初回ビルドの実行
	if (!BuildInternal(a_pDevice, a_pCmdList, a_geometryDescVec, _flags, false)) return;

	// 静的 BLAS は更新しないので、スクラッチはビルドが終われば要らない。
	// 終わるまではバッチに預けておき、終わったところで手放してもらう
	if (a_option.pKeepAlive && m_cpUpdateScratch)
	{
		a_option.pKeepAlive->push_back(std::move(m_cpUpdateScratch));
		m_cpUpdateScratch.Reset();
	}

	// 圧縮を頼む : 実体を共有の置き場へ移し、ビルドの完了で BLASCompactor へ渡るようにする
	if (_isCompactable && m_cpResource)
	{
		m_spCompactionTarget = std::make_shared<BLASCompactionTarget>();
		m_spCompactionTarget->cpResource = std::move(m_cpResource);
		m_cpResource.Reset();

		a_option.pOnBuildComplete->push_back(a_option.pCompactor->MakeBuildCompleteNotifier(m_spCompactionTarget));
	}
}

void Engine::Graphics::Raytracing::BLAS::CreateDynamic(
	D3D12::Device* a_pDevice, 
	D3D12::GraphicsCommandList* a_pCmdList, 
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& a_animatedGeometries
)
{
	// ALLOW_UPDATE フラグを立てて、更新とビルド速度を優先する構成
	// FAST_TRACE にすると更新負荷が跳ね上がるから、動的モデルは FAST_BUILD
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS _flags =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

	// ダイナミックモデル
	m_isDynamic = true;

	// 初回ビルドの実行
	BuildInternal(a_pDevice, a_pCmdList, a_animatedGeometries, _flags, false);
}

void Engine::Graphics::Raytracing::BLAS::Update(D3D12::GraphicsCommandList* a_pCmdList)
{
	if (!m_isDynamic) 
	{
		ENGINE_WARNING("静的BLASを更新しようとしています");
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC _buildDesc = {};
	_buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	_buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	_buildDesc.Inputs.NumDescs = static_cast<UINT>(m_geometryDescVec.size());
	_buildDesc.Inputs.pGeometryDescs = m_geometryDescVec.data();

	// 更新フラグを立てる
	_buildDesc.Inputs.Flags =
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

	// In-PlaceUpdate : SourceとDestinationに同じBLASアドレスを指定して上書き更新をする
	_buildDesc.SourceAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();
	_buildDesc.DestAccelerationStructureData = m_cpResource->GetGPUVirtualAddress();

	// 更新用のスクラッチバッファを使用する
	_buildDesc.ScratchAccelerationStructureData = m_cpUpdateScratch->GetGPUVirtualAddress();
	a_pCmdList->BuildRaytracingAccelerationStructure(&_buildDesc, 0, nullptr);
}
