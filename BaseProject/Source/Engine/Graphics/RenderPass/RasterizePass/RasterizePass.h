#pragma once

#include "../BaseRenderPass.h"

namespace Engine::Graphics
{
	// 描画用パスクラス
	class RasterizePass : public BaseRenderPass
	{
	public:

		RasterizePass() = default;
		virtual ~RasterizePass() override = default;

	protected:
		// パスの処理
		void Begine(RenderContext* a_pCtx);
		void End(RenderContext* a_pCtx);

		// 特定キューの描画
		void DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type);
		void DrawAnimeQueue(RenderContext* a_pCtx, RenderQueueType a_type);

		// ヘルパー構造体
		D3D12::GraphicsPipelineDesc& AddPSODesc(const ERenderType& a_type);

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
		std::unordered_map<ERenderType, D3D12::GraphicsPipelineDesc> m_psoMap = {};

		// ランタイム時データ
		UINT m_rootSigID = 0;												// パスが使用するルートシグネチャ
		std::vector<Resource::Handle<D3D12::PipelineState>> m_psoHandle;	// ソート済みPSOハンドル
	};
}