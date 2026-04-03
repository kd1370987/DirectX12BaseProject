#pragma once

namespace Engine::D3D12
{
	class GraphicsPSOManager
	{
	public:

		// 初期化
		void Init(ID3D12Device* a_pDevice);

		// PSOがすでに登録されていればそれを返す。なければ生成
		// パイプラインステートの名前で判断するため、セットネーム必須
		Resource::Handle<PipelineState> Request(const GraphicsPipelineDesc& a_desc);

		// アクセサ
		const ID3D12PipelineState* Get(const Resource::Handle<PipelineState>& a_handle) const;
		ID3D12PipelineState* Ref(const Resource::Handle<PipelineState>& a_handle);

	private:

		// 依存デバイス
		ID3D12Device* m_pDevice = nullptr;

		// 重なり防止
		std::unordered_map<std::string, Resource::Handle<PipelineState>> m_handleMap = {};

		Storage::HandleStorage<PipelineState> m_handleStorage;
		std::vector<PipelineState> m_pipelineData = {};
	};
}