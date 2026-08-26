#pragma once

#include "Core/Light.h"

namespace Engine::Graphics
{
	// ライト数上限 : FrameLightData の GPU バッファ要素数と、割り当て時の上限判定に使う
	inline constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
	inline constexpr uint32_t MAX_POINT_LIGHTS = 128;

	// 主光源をシェーダーへ渡すための定数バッファ。
	//
	// レイトレの影とGIは平行光へレイを1本しか飛ばさない(影マスクも1チャンネルしかない)ので、
	// あちらには配列ではなく「先頭の1つ」だけをこの形で渡す。
	// ※ HLSL 側 LightData.hlsli の SunLightData と並びを合わせること。
	//    cbuffer なので16バイト行に揃える
	struct SunLightCB
	{
		Math::Vector3 dir = { 0.0f, -1.0f, 0.0f };	// 方向(光の進む向き)
		float brightness = 0.0f;					// color に掛ける強さ
		Math::Color color = {};						// 色
		uint32_t enable = 0;						// 0 なら平行光なし
		uint32_t pad[3] = {};
	};

	// ライト用データ : フレームで使う用
	// 中身は LightManager::BuildFrameData() が毎フレーム詰め直す
	struct FrameLightData
	{
		D3D12::DynamicStructuredBuffer<DirectionalLight> dlBuffer = {};
		D3D12::DynamicStructuredBuffer<PointLight> plBuffer = {};

		// 今フレームで書き込んだ数
		// StructuredBuffer は要素数を持たないので、この数を別途CBでシェーダーへ渡すこと
		uint32_t dlCount = 0;
		uint32_t plCount = 0;

		// 作成 解放
		bool Create(D3D12::Device* a_pDevice);
		void Release();
	};

	/// <summary>
	/// ライトを管理するマネージャー
	/// </summary>
	class LightManager
	{
	public:

		// 初期化 解放
		void Init();
		void Release();

		// ライトの割り当て : 上限に達している場合は無効なハンドルが返る
		Handle<DirectionalLight>	AllocateDL();		// ディレクショナルライト割り当て
		Handle<PointLight>			AllocatePL();		// ポイントライト割り当て

		// ライトの解放
		template<typename T>
		void RemoveLight(const Handle<T>& a_handle);

		// ライト取得
		template<typename T>
		T* RefLight(const Handle<T>& a_handle);

		/// <summary>
		/// プールの中身を GPU 用バッファへ詰め直す : 描画前に毎フレーム1回だけ呼ぶ
		/// </summary>
		/// <param name="a_frameData">書き込み先 : Create() 済みのものを渡すこと</param>
		void BuildFrameData(FrameLightData& a_frameData);

		/// <summary>
		/// 直近の BuildFrameData() で詰めたディレクショナルライト
		/// </summary>
		/// <returns>GPUバッファと同じ中身・同じ添字の並び</returns>
		const std::vector<DirectionalLight>& GetFrameDirectionalLights() const;

		/// <summary>
		/// 主光源(先頭のディレクショナルライト)を定数バッファの形で取り出す
		/// </summary>
		/// <returns>1つも無ければ enable = 0 のものが返る</returns>
		SunLightCB GetSunLightCB() const;

	private:

		// プールの取得
		template<typename T>
		Pool::ItemPool<T>* RefPool();

		/// <summary>
		/// プールの中身を隙間なく作業配列へ集める
		/// ItemPool は Remove しても添字を詰めない(std::optional の穴が空く)ため、
		/// GetAll() をそのまま送ると穴の分だけシェーダー側の添字がずれる
		/// </summary>
		/// <param name="a_maxCount">GPUバッファの要素数 : ここで打ち切る</param>
		/// <returns>集めた数</returns>
		template<typename T>
		static uint32_t GatherLights(const Pool::ItemPool<T>& a_pool, std::vector<T>& a_outVec, uint32_t a_maxCount);

		// プール内で生きているライトの数 : 割り当て時の上限判定用
		template<typename T>
		static uint32_t CountAliveLights(const Pool::ItemPool<T>& a_pool);

	private:

		// ライトデータ
		Pool::ItemPool<DirectionalLight> m_directionalLightPool = {};
		Pool::ItemPool<PointLight> m_pointLightPool = {};

		// 詰め直し用の作業配列 : 毎フレーム走るので確保済みの領域を使い回す
		std::vector<DirectionalLight> m_dlWorkVec = {};
		std::vector<PointLight> m_plWorkVec = {};
	};

	//---------------------------------------------------------------------------------------
	// 外部用関数
	//---------------------------------------------------------------------------------------

	template<typename T>
	inline void LightManager::RemoveLight(const Handle<T>& a_handle)
	{
		RefPool<T>()->Remove(a_handle);
	}

	// 個別にライトを取得
	template<typename T>
	inline T* LightManager::RefLight(const Handle<T>& a_handle)
	{
		return RefPool<T>()->Ref(a_handle);
	}

	//---------------------------------------------------------------------------------------
	// 内部用関数
	//---------------------------------------------------------------------------------------

	// プールの取得
	// 特殊化していない型で呼ばれたら、リンクエラーではなくここで理由付きで止める
	template<typename T>
	inline Pool::ItemPool<T>* LightManager::RefPool()
	{
		static_assert(sizeof(T) == 0, "LightManager が管理していないライト型です");
		return nullptr;
	}
	template<>
	inline Pool::ItemPool<DirectionalLight>* LightManager::RefPool<DirectionalLight>()
	{
		return &m_directionalLightPool;
	}
	template<>
	inline Pool::ItemPool<PointLight>* LightManager::RefPool<PointLight>()
	{
		return &m_pointLightPool;
	}

	// プールの中身を隙間なく作業配列へ集める
	template<typename T>
	inline uint32_t LightManager::GatherLights(const Pool::ItemPool<T>& a_pool, std::vector<T>& a_outVec, uint32_t a_maxCount)
	{
		a_outVec.clear();

		for (const auto& _slot : a_pool.GetAll())
		{
			// Remove 済みのスロットは飛ばす
			if (!_slot.has_value()) continue;

			// バッファの要素数を超えると AllocateAndWrite が書き込みごと拒否して
			// そのフレームのライトが丸ごと消えるので、手前で打ち切る
			if (a_outVec.size() >= a_maxCount) break;

			a_outVec.push_back(_slot.value());
		}

		return static_cast<uint32_t>(a_outVec.size());
	}

	// プール内で生きているライトの数
	template<typename T>
	inline uint32_t LightManager::CountAliveLights(const Pool::ItemPool<T>& a_pool)
	{
		uint32_t _count = 0;
		for (const auto& _slot : a_pool.GetAll())
		{
			if (_slot.has_value()) ++_count;
		}
		return _count;
	}
}
