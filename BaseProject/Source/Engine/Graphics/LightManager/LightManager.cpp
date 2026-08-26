#include "LightManager.h"

namespace Engine::Graphics
{
	//---------------------------------------------------------------------------------------
	// FrameLightData
	//---------------------------------------------------------------------------------------

	bool FrameLightData::Create(D3D12::Device* a_pDevice)
	{
		// 要素数は上限固定で確保する
		// ライトが増えるたびにバッファを作り直すと、GPU が読んでいる最中のリソースを
		// 開放することになるため、最初から最大数ぶん取っておく
		if (!dlBuffer.Create(a_pDevice, MAX_DIRECTIONAL_LIGHTS)) return false;
		if (!plBuffer.Create(a_pDevice, MAX_POINT_LIGHTS)) return false;

		return true;
	}

	void FrameLightData::Release()
	{
		dlBuffer.Release();
		plBuffer.Release();

		dlCount = 0;
		plCount = 0;
	}

	//---------------------------------------------------------------------------------------
	// LightManager
	//---------------------------------------------------------------------------------------

	void LightManager::Init()
	{
		// 上限ぶんを先に取っておき、ライト追加のたびに再確保が走らないようにする
		m_directionalLightPool.Reserve(MAX_DIRECTIONAL_LIGHTS);
		m_pointLightPool.Reserve(MAX_POINT_LIGHTS);

		// 詰め直しは毎フレーム走るので、作業配列も同じく先に確保しておく
		m_dlWorkVec.reserve(MAX_DIRECTIONAL_LIGHTS);
		m_plWorkVec.reserve(MAX_POINT_LIGHTS);
	}

	void LightManager::Release()
	{
		// プールを空にする
		// ここを通した時点で配り済みのハンドルはすべて無効になるため、
		// 呼ぶのはシーンの切れ目など、ライトの持ち主ごと消えるタイミングに限ること
		m_directionalLightPool.Release();
		m_pointLightPool.Release();

		m_dlWorkVec.clear();
		m_plWorkVec.clear();
	}

	Handle<DirectionalLight> LightManager::AllocateDL()
	{
		// 上限を超えたぶんは BuildFrameData() で GPU バッファに載らず、
		// 無言で描画から落ちる。ここで弾いて呼び出し側に気づかせる
		if (CountAliveLights(m_directionalLightPool) >= MAX_DIRECTIONAL_LIGHTS)
		{
			ENGINE_WARNING("ディレクショナルライトが上限(%u)に達しています", MAX_DIRECTIONAL_LIGHTS);
			return {};
		}

		return m_directionalLightPool.Add(DirectionalLight{});
	}

	Handle<PointLight> LightManager::AllocatePL()
	{
		// 上限の扱いは AllocateDL() と同じ
		if (CountAliveLights(m_pointLightPool) >= MAX_POINT_LIGHTS)
		{
			ENGINE_WARNING("ポイントライトが上限(%u)に達しています", MAX_POINT_LIGHTS);
			return {};
		}

		return m_pointLightPool.Add(PointLight{});
	}

	const std::vector<DirectionalLight>& LightManager::GetFrameDirectionalLights() const
	{
		return m_dlWorkVec;
	}

	SunLightCB LightManager::GetSunLightCB() const
	{
		SunLightCB _cb = {};

		// 先頭が主光源。
		// 平行光を置くのはシーンに1つ(SceneAmbientObject)なので、ここは実質そのライト。
		// 2つ目以降を足した場合、影を落とすのはあくまで先頭の1つだけになる
		if (m_dlWorkVec.empty()) return _cb;

		const DirectionalLight& _dl = m_dlWorkVec.front();
		_cb.dir        = _dl.dir;
		_cb.brightness = _dl.brightness;
		_cb.color      = _dl.color;
		_cb.enable     = 1;

		return _cb;
	}

	void LightManager::BuildFrameData(FrameLightData& a_frameData)
	{
		// プールの穴を詰めて作業配列へ集める
		a_frameData.dlCount = GatherLights(m_directionalLightPool, m_dlWorkVec, MAX_DIRECTIONAL_LIGHTS);
		a_frameData.plCount = GatherLights(m_pointLightPool, m_plWorkVec, MAX_POINT_LIGHTS);

		// 書き込みオフセットを毎フレーム先頭へ戻す
		// 戻さないと AllocateAndWrite がフレームごとに後方へ書き進み、
		// 先頭(element0)からバインドしているシェーダーは初回フレームのライトを読み続ける
		// (RenderContext のボーン/UIバッファと同じ運用)
		a_frameData.dlBuffer.ResetForNewFrame();
		a_frameData.plBuffer.ResetForNewFrame();

		// 0件なら書き込まない : シェーダーへは Count = 0 が渡るので参照されない
		if (a_frameData.dlCount > 0)
		{
			a_frameData.dlBuffer.AllocateAndWrite(m_dlWorkVec);
		}
		if (a_frameData.plCount > 0)
		{
			a_frameData.plBuffer.AllocateAndWrite(m_plWorkVec);
		}
	}
}

