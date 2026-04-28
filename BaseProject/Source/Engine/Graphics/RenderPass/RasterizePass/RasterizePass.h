#pragma once

#include "../BaseRenderPass.h"

namespace Engine::Graphics
{
	struct PassData
	{
		D3D12::GraphicsPipelineDesc psoDesc = {};
		RenderQueueType type;
	};

	// 描画用パスクラス
	class RasterizePass : public BaseRenderPass
	{
	public:

		RasterizePass() = default;
		virtual ~RasterizePass() override = default;

		void Init(const PassInitDesc& a_initDesc) override;

	protected:
		// パスの処理
		void Begine(RenderContext* a_pCtx);
		void End(RenderContext* a_pCtx);

		// 特定キューの描画
		void DrawQueue(RenderContext* a_pCtx);
		void DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type, Resource::Handle<D3D12::PipelineState> a_handle);
	
		// ヘルパー構造体
		D3D12::GraphicsPipelineDesc& AddPSODesc(const ERenderType& a_type, const RenderQueueType& a_queueType);

		// PSOを指定してインプットレイアウトの指定
		void SetInputLayout(const ERenderType& a_type, const D3D12_INPUT_LAYOUT_DESC& a_desc);

		// VSのセット
		void SetVS(const ERenderType& a_type, const std::string& a_filePath);

		// PSのセット
		void SetPS(const ERenderType& a_type, const std::string& a_filePath);		// 片方ずつ
		void SetPS(const std::string& a_filePath);									// 二つ同時に
		
		// ルートシグネチャセット
		void SetRootSig(const std::string& a_rootName);

		// 書き込み依存
		Engine::Resource::ID AddWrite(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp) override;

	protected:
		// 生成時データ
		// パスが使用するPSO
		std::unordered_map<ERenderType, PassData> m_psoMap = {};

		// ランタイム時データ
		UINT m_rootSigID = 0;												// パスが使用するルートシグネチャ
		//std::unordered_map<ERenderType, Resource::Handle<D3D12::PipelineState>> m_psoHandle = {};
		std::vector<std::pair<Resource::Handle<D3D12::PipelineState>, RenderQueueType>> m_psoHandle = {};
	};
}