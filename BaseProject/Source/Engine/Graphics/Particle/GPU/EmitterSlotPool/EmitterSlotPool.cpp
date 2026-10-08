#include "Engine/Graphics/Particle/GPU/EmitterSlotPool/EmitterSlotPool.h"

#include "Engine/MainEngine.h"

namespace Engine::Graphics::Particle
{
	void EmitterSlotPool::Init(uint32_t a_blockSize)
	{
		m_blockSize = (std::max)(a_blockSize, 1u);

		m_transforms.clear();
		m_pendingReturns.clear();
		m_usedCount = 0;
		m_liveCount = 0;
		m_pendingCount = 0;

		// 作り直して空にする。
		// HandlePool::Create は配った数(m_currentCount)を戻さないので、
		// 使い回すと Release → Init の後に範囲外の世代を引いてしまう
		m_handlePool = Pool::HandlePool<EmitterTransform>{};
		m_handlePool.Create();

		// 席 0 は単位行列で予約する。ワールド空間の粒はここを指すので、
		// 描画側は分岐なしで同じ掛け算を通せる。二度と返さない
		m_identitySlot = Acquire();
	}
	void EmitterSlotPool::Release()
	{
		m_transforms.clear();
		m_pendingReturns.clear();
		m_usedCount = 0;
		m_liveCount = 0;
		m_pendingCount = 0;
		m_identitySlot = {};

		// 終了時は GPU の完了待ちより前に呼ばれる(MainEngine::Release → GraphicsEngine::Release)。
		// 最後のフレームがまだ読んでいるかもしれないので、完了待ちの後に流れる遅延解放へ回す。
		// 壊すだけではディスクリプタ(SRV)が返らないので Release() を呼ぶ
		if (m_upGPUBuffer)
		{
			std::shared_ptr<Graphics::D3D12::StaticStructuredBuffer<EmitterTransform>> _spOld(std::move(m_upGPUBuffer));
			MainEngine::Instance().ReserveRelease([_spOld]() { _spOld->Release(); });
		}
		m_gpuCapacity = 0;
	}
	Handle<EmitterTransform> EmitterSlotPool::Acquire()
	{
		// 席番号は uint16 なので、配りきったらそれ以上は出せない。
		// 出せなかった発生源はワールド空間で出る(席 0)
		if (m_liveCount >= SLOT_INDEX_MAX)
		{
			ENGINE_WARNING("[Particle] 発生源の席が上限(%u)に達しました。ワールド空間で出します", SLOT_INDEX_MAX);
			return {};
		}

		auto _handle = m_handlePool.Allocate();
		if (!_handle.IsValid()) return {};

		++m_liveCount;

		// 現在の使用数を求める
		m_usedCount = (std::max)(m_usedCount, static_cast<uint32_t>(_handle.GetIndex()) + 1u);

		// 配列の要素数が足りていなければブロック数分リサイズ
		while (m_transforms.size() < m_usedCount)
		{
			const size_t _nextSize = m_transforms.size() + static_cast<size_t>(m_blockSize);
			m_transforms.resize(_nextSize);
			m_pendingReturns.resize(_nextSize);
		}

		// 前の持ち主の行列が残っているので、単位行列から始める
		m_transforms[_handle.GetIndex()] = {};
		m_pendingReturns[_handle.GetIndex()] = {};

		return _handle;
	}
	void EmitterSlotPool::SetTransform(const Handle<EmitterTransform>& a_handle, const Math::Matrix& a_world)
	{
		// 世代、ハンドルチェック
		if (!m_handlePool.IsValid(a_handle)) return;

		// 席 0 は単位行列のまま
		if (a_handle == m_identitySlot) return;

		// ワールド座標を設定
		m_transforms[a_handle.GetIndex()].worldMat = StripScale(a_world);
	}
	void EmitterSlotPool::ReserveReturn(const Handle<EmitterTransform>&a_handle, float a_holdSeconds)
	{
		// 世代、ハンドルチェック
		if (!m_handlePool.IsValid(a_handle)) return;

		// 席 0 は返さない
		if (a_handle == m_identitySlot) return;

		// 使用時間を設定。
		// 二重に予約されたら残り時間だけ取り直す(数は増やさない)
		PendingReturn& _return = m_pendingReturns[a_handle.GetIndex()];
		if (!_return.handle.IsValid())
		{
			++m_pendingCount;
		}
		_return.handle = a_handle;
		_return.remain = a_holdSeconds;
	}
	void EmitterSlotPool::BeginFrame(float a_dt)
	{
		// 使用時間を減らしていき、尽きたものを空きへ戻す
		for (auto& _return : m_pendingReturns)
		{
			if (!_return.handle.IsValid()) continue;

			_return.remain -= a_dt;
			if (_return.remain > 0.0f) continue;

			m_handlePool.Remove(_return.handle);
			_return = {};

			--m_liveCount;
			--m_pendingCount;
		}
	}
	void EmitterSlotPool::Upload(
		Graphics::D3D12::Device* a_pDevice,
		Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
		Graphics::D3D12::GraphicsCommandList* a_pCmdList,
		UINT a_frameIndex
	)
	{
		if (m_usedCount == 0 || !a_pCmdList) return;

		//----------------------------------------------------------------------
		// 足りなければ作り直す
		//
		// 行列は毎フレーム全部送り直すので、中身を写す必要はない。
		// 古いバッファは前のフレームの描画がまだ読んでいるかもしれないので、
		// GPU が使い終わるまで遅延させて捨てる
		//----------------------------------------------------------------------
		const uint32_t _needCapacity = static_cast<uint32_t>(m_transforms.size());
		if (!m_upGPUBuffer || m_gpuCapacity < _needCapacity)
		{
			if (m_upGPUBuffer)
			{
				std::shared_ptr<Graphics::D3D12::StaticStructuredBuffer<EmitterTransform>> _spOld(std::move(m_upGPUBuffer));
				// 壊すだけではディスクリプタ(SRV)が返らないので、Release() を呼んでから手放す
				MainEngine::Instance().ReserveRelease([_spOld]() { _spOld->Release(); });
			}

			m_upGPUBuffer = std::make_unique<Graphics::D3D12::StaticStructuredBuffer<EmitterTransform>>();
			m_upGPUBuffer->Create(a_pDevice, a_pHeapManager, a_pCmdList, _needCapacity, nullptr);
			m_gpuCapacity = _needCapacity;
		}

		// 配ったことのある範囲だけ送る(返却済みの席も混ざるが、どの粒も指していないので害はない)
		m_upGPUBuffer->UploadFrame(
			a_pCmdList,
			m_transforms.data(),
			sizeof(EmitterTransform) * m_usedCount,
			a_frameIndex);
	}
	UINT EmitterSlotPool::GetSRVIndex() const
	{
		if (!m_upGPUBuffer) return (std::numeric_limits<UINT>::max)();
		return m_upGPUBuffer->GetSRVHandle().GetIndex();
	}
	uint32_t EmitterSlotPool::ToGPUIndex(const Handle<EmitterTransform>& a_handle)
	{
		// 世代までは見ない(見るのは SetTransform 側)。無効なら単位行列の席 0
		return a_handle.IsValid() ? static_cast<uint32_t>(a_handle.GetIndex()) : 0u;
	}

	Math::Matrix EmitterSlotPool::StripScale(const Math::Matrix& a_world)
	{
		Math::Matrix _mat = a_world;

		Math::Vector3 _axisX(_mat._11, _mat._12, _mat._13);
		Math::Vector3 _axisY(_mat._21, _mat._22, _mat._23);
		Math::Vector3 _axisZ(_mat._31, _mat._32, _mat._33);

		if (_axisX.LengthSquared() > 1e-12f) _axisX.Normalize();
		if (_axisY.LengthSquared() > 1e-12f) _axisY.Normalize();
		if (_axisZ.LengthSquared() > 1e-12f) _axisZ.Normalize();

		_mat._11 = _axisX.x; _mat._12 = _axisX.y; _mat._13 = _axisX.z;
		_mat._21 = _axisY.x; _mat._22 = _axisY.y; _mat._23 = _axisY.z;
		_mat._31 = _axisZ.x; _mat._32 = _axisZ.y; _mat._33 = _axisZ.z;

		return _mat;
	}
}
