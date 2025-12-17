#pragma once

class RootSignature;
class PipelineState;

class PSOManager
{
public:

	void Init();
	
	ID3D12PipelineState* GetPipelineState(const std::string& a_name);

	void SetPipelienStaet(const std::string& a_name);

private:

	std::shared_ptr<RootSignature> m_spRootSignature = nullptr;

	// パイプラインステート
	std::unordered_map<std::string, std::shared_ptr<PipelineState>> m_pipelineMap = {};

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