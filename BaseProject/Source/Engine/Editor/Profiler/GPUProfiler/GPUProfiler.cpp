#include "GPUProfiler.h"

#include "../../Internal/EditorTimer.h"

#include "../../../MainEngine.h"
#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Editor
{
	void GPUProfiler::Init()
	{
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		// クエリーヒープ作成
		D3D12_QUERY_HEAP_DESC _desc = {};
		_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		_desc.Count = m_maxQueries;
		_desc.NodeMask = 0;

		auto _hr = _pDevice->CreateQueryHeap(&_desc,IID_PPV_ARGS(&m_upQueryHeap));
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false,"クエリヒープ失敗");
			return;
		}

		// リードバックバッファ作成 : フレームぶんだけ用意する
		for (UINT _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			D3D12::GPUBufferDesc _readBackDesc = {};
			_readBackDesc.elementNum = m_maxQueries;
			_readBackDesc.strideSize = sizeof(uint64_t);
			_readBackDesc.flags = D3D12_RESOURCE_FLAG_NONE;
			_readBackDesc.heapType = D3D12_HEAP_TYPE_READBACK;

			if (!m_readBackBuffers[_i].Create(_pDevice, _readBackDesc))
			{
				ENGINE_ERRLOG(false, "リードバックバッファ作成失敗");
				return;
			}

			// マップしておく
			// 受け取り先のポインタ変数のアドレスを渡す
			m_readBackBuffers[_i].Map(reinterpret_cast<void**>(&m_mapData[_i]));
		}
	}
	void GPUProfiler::Releasse()
	{
		for (UINT _i = 0; _i < CPU_FRAME_COUNT; ++_i)
		{
			if (!m_mapData[_i]) continue;

			m_readBackBuffers[_i].Unmap();
			m_mapData[_i] = nullptr;
		}
	}
	void GPUProfiler::BeginFrame()
	{
		// 0番は「GPU計測なし」の予約枠なので1番から配る
		m_nextQuery = 1;
	}

	void GPUProfiler::EndFrame(
		D3D12::CommandQueue* a_pCmdQueue,
		std::unordered_map<std::string, Timer>& a_timers
	)
	{
		// GPUの計測結果を取得する、GPUの処理が完了している必要があり
		// (呼び出し側がGPU待機を抜けた直後に呼ぶことで保証している)

		// 初期化に失敗しているとマップ先がないので何もしない
		if (!a_pCmdQueue) return;

		const UINT _frameIdx = CurrentFrameIndex();
		const uint64_t* _pData = m_mapData[_frameIdx];
		if (!_pData) return;

		// GPUタイムスタンプ周波数を取得
		uint64_t _gpuFrequency = 0;
		if (FAILED(a_pCmdQueue->GetTimestampFrequency(&_gpuFrequency)) || _gpuFrequency == 0)
		{
			return;
		}

		// データの計算
		for (auto& [_name, _timer] : a_timers)
		{
			// GPU計測を積んでいない項目(コマンドリストを渡していないCPU専用の計測)は0にする
			if (_timer.startIndex == INVALID_QUERY || _timer.endIndex == INVALID_QUERY)
			{
				_timer.gpuTime = 0.0;
				continue;
			}

			const uint64_t _begin = _pData[_timer.startIndex];
			const uint64_t _end = _pData[_timer.endIndex];

			// まだResolveが届いていないフレームでは前後が逆転しうるので弾く
			if (_end <= _begin)
			{
				_timer.gpuTime = 0.0;
				continue;
			}

			_timer.gpuTime = ((_end - _begin) * 1000.0) / static_cast<double>(_gpuFrequency);
			_timer.gpuAccumulatedTime += _timer.gpuTime;
		}
	}

	void GPUProfiler::Start(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer)
	{
		// 開始と終了で2つ使うので、2つぶん空いていなければ計測しない
		// (開始だけ積んで終了が積めないと、範囲が壊れたまま読まれる)
		if (!m_upQueryHeap || m_nextQuery + 2 > m_maxQueries)
		{
			a_timer.startIndex = INVALID_QUERY;
			a_timer.endIndex = INVALID_QUERY;
			return;
		}

		// タイマーに開始インデックスを記憶し、カウンタを進める
		a_timer.startIndex = m_nextQuery++;
		a_timer.endIndex = INVALID_QUERY;

		// コマンドリストにタイムスタンプの記録
		a_pCmdList->EndQuery(
			m_upQueryHeap.Get(),
			D3D12_QUERY_TYPE_TIMESTAMP,
			a_timer.startIndex
		);
	}

	void GPUProfiler::End(D3D12::GraphicsCommandList * a_pCmdList, Timer & a_timer)
	{
		// Startが積めていなければ何もしない
		if (!m_upQueryHeap || a_timer.startIndex == INVALID_QUERY) return;
		if (m_nextQuery >= m_maxQueries) return;

		// タイマーに終了インデックスを記憶し、カウンタを進める
		a_timer.endIndex = m_nextQuery++;

		// コマンドリストにタイムスタンプの記録
		a_pCmdList->EndQuery(
			m_upQueryHeap.Get(),
			D3D12_QUERY_TYPE_TIMESTAMP,
			a_timer.endIndex
		);

		// 計測したデータをリードバックバッファに書きだし
		//
		// 開始と終了は隣り合うとは限らない。
		// 計測が入れ子になると間に別の計測のクエリが挟まるので、
		// まとめて2個解決するのではなく、それぞれ自分のインデックスの位置へ1個ずつ書く。
		// こうしておくと バッファのn番目 = クエリのn番目 で常に一致する。
		auto* _pBuffer = m_readBackBuffers[CurrentFrameIndex()].GetResource();
		if (!_pBuffer) return;

		a_pCmdList->ResolveQueryData(
			m_upQueryHeap.Get(),
			D3D12_QUERY_TYPE_TIMESTAMP,
			a_timer.startIndex,
			1,
			_pBuffer,
			a_timer.startIndex * sizeof(uint64_t)
		);
		a_pCmdList->ResolveQueryData(
			m_upQueryHeap.Get(),
			D3D12_QUERY_TYPE_TIMESTAMP,
			a_timer.endIndex,
			1,
			_pBuffer,
			a_timer.endIndex * sizeof(uint64_t)
		);
	}

	//======================================================================================
	// 記録・読み出しに使うフレームスロット
	//
	// D3D12Wrapper::BeginFrame() がこのインデックスのGPU完了を待ってから進むので、
	// 待機直後に読む値はこのスロットのものだけが確定している
	//======================================================================================
	UINT GPUProfiler::CurrentFrameIndex() const
	{
		return D3D12::D3D12Wrapper::Instance().CurrentCPUFrameIndex() % CPU_FRAME_COUNT;
	}
}
