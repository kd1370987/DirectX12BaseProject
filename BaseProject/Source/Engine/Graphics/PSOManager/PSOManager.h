#pragma once

class RootSignature;
class PipelineState;

class PSOManager
{
public:

	void Init();
	

private:

	// ルートシグネチャ
	std::shared_ptr<RootSignature> m_spRootSignature;

	// パイプラインステート
	std::shared_ptr<PipelineState> m_spPipelineState;

	

private:
	// シングルトン
	PSOManager() = default;
	~PSOManager() = default;
public:
	static PSOManager& Instance()
	{
		static PSOManager _instance;
		return _instance;
	}
};