#pragma once
//==========================================================================================
//
// VirtualResource (Engine::Graphics::Pipeline)
//
// 仮想リソース
// 実体は持たないが、パスの順序解析や、実体の作成時の情報を持つ。
//
// 中身は「パスの出力スロット」から起こす。ノードグラフの配線がそのまま宣言なので、
// パス側から Declare を呼ぶ必要はない(既存の RGResourceManager との一番大きな違い)。
// 外部で作られたリソース(バックバッファ・フレームリソースなど)だけは
// RenderGraph::ImportResource() で明示的に差し込む。
//
// 配列の持ち主は RenderGraph。マネージャークラスは置かない。
//
//==========================================================================================
#include "../../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	class VirtualResource
	{
	public:

		VirtualResource() = default;
		~VirtualResource() = default;

		//----------------------------------------------------------------------------------
		// 構築
		//----------------------------------------------------------------------------------
		// 出力スロットから要件を起こす(このリソースを作るパスのスロット)
		void SetupFromOutputSlot(const std::string& a_name, const Slot& a_slot);

		// 外部で作られたリソースとして起こす : サイズやフォーマットは向こうの持ち物
		void SetupAsImported(const std::string& a_name, EPassSlotType a_type, D3D12_RESOURCE_STATES a_initialState);

		// 同じリソースを触っている別のスロットの要件を足し込む。
		// 書き込み側(出力スロット)がフォーマット・サイズの主導権を持ち、
		// 読み込み側(入力スロット)は用途フラグだけを足す
		void MergeSlot(const Slot& a_slot);

		// width / height が 0 のときに、描画解像度から実サイズを決める。
		// 明示サイズが入っているものはそのまま(scale は掛けない)
		void ResolveSize(UINT64 a_baseWidth, UINT a_baseHeight);

		//----------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------
		const std::string& GetName() const { return m_name; }

		EPassSlotType GetType() const { return m_type; }
		bool IsBuffer() const { return m_type == EPassSlotType::Buffer; }

		DXGI_FORMAT GetFormat() const { return m_format; }
		UINT64 GetWidth() const { return m_width; }
		UINT GetHeight() const { return m_height; }
		float GetScale() const { return m_scale; }

		Resource::TextureUsage GetUsage() const { return m_usage; }
		bool HasUsage(Resource::TextureUsage a_usage) const;

		const Math::Color& GetClearColor() const { return m_clearColor; }
		bool IsClear() const { return m_isClear; }

		// 外部から差し込まれたものは、実体を作らず参照するだけ
		bool IsImported() const { return m_isImported; }

		//----------------------------------------------------------------------------------
		// Temporal(フレーム間の履歴)
		//
		// 立っていると物理リソースを2枚持ち、フレームごとに役を入れ替える。
		//   Current  : このフレームが書く側
		//   Previous : 前のフレームが書いた側(読む側)
		//
		// 反復処理(Denoise を5回など)はパスを並べて表現するので、ここでは扱わない。
		// あくまでフレームをまたぐ履歴のためのもの
		//----------------------------------------------------------------------------------
		bool IsTemporal() const { return m_isTemporal; }

		// 使う物理リソースの枚数
		uint32_t GetPhysicalCount() const { return m_isTemporal ? 2u : 1u; }

		// スロットから、どちらのスライスを触るかを決める。
		//
		// 書き込みは必ず Current(今フレームぶん)。
		// 読み取りは、そのピンが「前フレームを読む」と宣言しているときだけ Previous になる。
		// 履歴を読むのはピンの役割であってリソースの都合ではないので、
		// 同じ2枚組リソースでも、ただの後段が読めば今フレームの結果が返る
		static uint32_t ToSlice(const Slot& a_slot, bool a_isResourceTemporal)
		{
			if (!a_isResourceTemporal) return 0;
			return (a_slot.isIn && a_slot.isTemporal) ? 1u : 0u;
		}

		// フレームの入口でのステート。
		// グラフは毎フレーム同じ手順で回るので、最後にここへ戻しておかないと
		// 次のフレームのバリアの before がずれる
		// ステートはスライスごとに追う。
		// Temporal だと2枚の物理が別々の遷移をたどるため、カーソルも2本要る
		D3D12_RESOURCE_STATES GetInitialState(uint32_t a_slice = 0) const { return m_initialState[a_slice & 1]; }
		void SetInitialState(D3D12_RESOURCE_STATES a_state, uint32_t a_slice = 0) { m_initialState[a_slice & 1] = a_state; }

		// バリア構築中のステート : どこまで遷移させたかを追うカーソル
		D3D12_RESOURCE_STATES GetCurrentState(uint32_t a_slice = 0) const { return m_currentState[a_slice & 1]; }
		void SetCurrentState(D3D12_RESOURCE_STATES a_state, uint32_t a_slice = 0) { m_currentState[a_slice & 1] = a_state; }

		// カーソルをフレーム入口の状態へ戻す
		void ResetStateToInitial()
		{
			m_currentState[0] = m_initialState[0];
			m_currentState[1] = m_initialState[1];
		}

		// 割り当てられた物理リソースの添字。
		// Temporal のときだけ [1] も使う。
		// あとでエイリアシング(使い回し)を入れるときにここがずれる
		uint32_t GetPhysicalIndex(uint32_t a_slice = 0) const { return m_physicalIndex[a_slice & 1]; }
		void SetPhysicalIndex(uint32_t a_index, uint32_t a_slice = 0) { m_physicalIndex[a_slice & 1] = a_index; }

		//----------------------------------------------------------------------------------
		// スロットのアクセスタイプ -> テクスチャの用途フラグ
		//----------------------------------------------------------------------------------
		// 書き込み側は、あとから読めるように SRV も一緒に立てる
		static Resource::TextureUsage ToUsage(EAccessType a_accessType, bool a_isWrite);

		//----------------------------------------------------------------------------------
		// スロットのアクセスタイプ -> リソースステート(バリア構築用)
		//----------------------------------------------------------------------------------
		static D3D12_RESOURCE_STATES ToResourceState(EAccessType a_accessType);

		// 読み取り専用のステートか。
		// 読み取り同士は OR でまとめられるが、書き込みが混ざったら順序を決めないといけない
		static bool IsReadOnlyState(D3D12_RESOURCE_STATES a_state);

	private:

		// リソーステクスチャの作成情報
		std::string m_name = "none";
		EPassSlotType m_type = EPassSlotType::Texture;

		// --- 要件(出力スロットから起こす) ---
		DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
		UINT64 m_width = 0;			// バッファのときはバイト数として扱う
		UINT m_height = 0;
		float m_scale = 1.f;		// width / height が 0 のときに描画解像度へ掛ける倍率
		Resource::TextureUsage m_usage = Resource::TextureUsage::None;

		// --- クリア ---
		Math::Color m_clearColor = { 0.f, 0.f, 0.f, 1.f };
		bool m_isClear = false;		// どれかのパスが ELoadOp::Clear を指定したか

		// --- 状態管理 ---
		bool m_isImported = false;
		bool m_isTemporal = false;		// フレーム間で入れ替える(物理を2枚持つ)

		// スライスごと([0]=Current / [1]=Previous)。Temporal でなければ [0] だけ使う
		D3D12_RESOURCE_STATES m_initialState[2] = { D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON };
		D3D12_RESOURCE_STATES m_currentState[2] = { D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON };

		// --- 物理リソースとの紐付け ---
		uint32_t m_physicalIndex[2] = { ResourceHandle::INVALID_INDEX, ResourceHandle::INVALID_INDEX };
	};
}
