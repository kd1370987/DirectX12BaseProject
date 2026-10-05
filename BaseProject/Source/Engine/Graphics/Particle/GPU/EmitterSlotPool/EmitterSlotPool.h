#pragma once
namespace Engine::Particle
{
	// 出現位置のワールド行列
	struct EmitterTransform
	{
		Math::Matrix worldMat;
	};

	// パーティクルを出す際のすべての共通の発生源テーブル
	class EmitterSlotPool
	{
	public:

		// 初期化 : ブロック内の最大容量を入れる
		void Init(size_t a_blcokEmitterSize);

		// 席の貸し出し



	private:

		// ブロックの追加
		void Expand();

	private:
		
		// 一ブロック : 上限が来たら次のブロックをはやす、めったにない
		struct EmitterBlock
		{
			explicit EmitterBlock(size_t a_num);
			~EmitterBlock();

			Pool::HandlePool<EmitterTransform> handlePool;
		};

		// 一ブロック当たりの最大収容数
		size_t m_blockSize = 0;
		std::vector<std::unique_ptr<EmitterBlock>> m_upEmitterBlocks;


		// エミッタースロットバッファ : 足りなくなったら伸ばす
		D3D12::StaticStructuredBuffer<EmitterTransform> m_emitterSlotsBuffer;
	};
}