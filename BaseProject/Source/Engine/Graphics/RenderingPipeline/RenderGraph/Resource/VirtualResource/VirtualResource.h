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
// 配列の持ち主は ResourceRegistry。
//
//==========================================================================================
#include "../../../Core/Slot.h"

namespace Engine::Graphics::Pipeline
{
	// AllocationInfo が先に実体を持つので、ここで名前だけ通しておく。
	// 今までは先に RenderGraph.h を通した翻訳単位でだけ通っていた
	class VirtualResource;

	/// <summary>
	/// 割り当てられた場所
	/// </summary>
	struct AllocationInfo
	{
		uint64_t offset = 0;
		uint64_t size = 0;
		uint64_t alignment = 0;
		uint32_t slotIndex = 0;			// 割り当てられたスロット番号

		// このスロットを直前に使っていたリソース
		VirtualResource* pPrevVRes = nullptr;
	};

	class VirtualResource
	{
	public:

		VirtualResource() = default;
		~VirtualResource() = default;

		//----------------------------------------------------------------------------------
		// 構築
		//----------------------------------------------------------------------------------
		// 出力スロットから要件を起こす(このリソースを作るパスのスロット)。
		// 識別子と表示名もこのスロットから取る。
		//
		// 描画解像度も一緒に受け取る。スロットのサイズは「0 = 解像度に従う」なので、
		// 土台が無いと実サイズが出せず、占有サイズも出せないため
		void SetupFromOutputSlot(const Slot& a_slot, UINT64 a_baseWidth, UINT a_baseHeight);

		// 外部で作られたリソースとして起こす : サイズやフォーマットは向こうの持ち物
		void SetupAsImported(const std::string& a_name, EPassSlotType a_type, D3D12_RESOURCE_STATES a_initialState);

		// 同じリソースを触っている別のスロットの要件を足し込む。
		// 書き込み側(出力スロット)がフォーマット・サイズの主導権を持ち、
		// 読み込み側(入力スロット)は用途フラグだけを足す
		void MergeSlot(const Slot& a_slot);

		// 描画解像度を受け取り直して、実サイズと占有サイズを出し直す。
		// 宣言が 0 のところだけ「解像度 × scale」で埋まる(明示サイズはそのまま)。
		// 何度呼んでも同じ結果になるので、要件が変わるたびに通してよい
		void ResolveSize(UINT64 a_baseWidth, UINT a_baseHeight);

		//----------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------
		// 同一性はこちら。名前はあくまで表示用のラベル
		const ResourceID& GetResourceID() const { return m_resourceID; }

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
		// あとでエイリアシング(使い回し)を入れるときにここがずれる。
		//
		// 指す先は RenderGraph が持つ物理リソース配列であって、仮想リソースの並びではない。
		// 仮想リソース側の参照は ResourceID なので、番兵もこちらで持つ
		static constexpr uint32_t INVALID_PHYSICAL_INDEX = static_cast<uint32_t>(-1);

		uint32_t GetPhysicalIndex(uint32_t a_slice = 0) const { return m_physicalIndex[a_slice & 1]; }
		void SetPhysicalIndex(uint32_t a_index, uint32_t a_slice = 0) { m_physicalIndex[a_slice & 1] = a_index; }

		//----------------------------------------------------------------------------------
		// 生存区間
		//
		// このリソースに触る最初のパスと最後のパス。
		// 添字は RenderGraph::GetCompiledPasses() の並び = 実行順そのもの。
		//
		// 「触る」は読み書きのどちらでもよい。区間の外ではこのリソースの中身が
		// 誰にも要らないということなので、区間が重ならないリソース同士は
		// 同じ実体を使い回せる(エイリアシング)。
		//
		// 区間は配線から決まるので、実体を作るより前に出せる。
		// RenderGraph が並べ替えを終えた直後に組み立てる
		//----------------------------------------------------------------------------------
		static constexpr uint32_t INVALID_PASS_INDEX = static_cast<uint32_t>(-1);

		uint32_t GetFirstPassIndex() const { return m_firstPassIndex; }
		uint32_t GetLastPassIndex() const { return m_lastPassIndex; }

		// どのパスも触っていないか。
		// 出力しただけで誰も読まないリソースはここが false にならない(書いた本人が触っている)。
		// true になるのは、宣言だけあって配線から外れたものだけ
		bool HasLifetime() const { return m_firstPassIndex != INVALID_PASS_INDEX; }

		// 区間を空にする(組み立て直しの前に呼ぶ)
		void ResetLifetime()
		{
			m_firstPassIndex = INVALID_PASS_INDEX;
			m_lastPassIndex = INVALID_PASS_INDEX;
		}

		// このパスが触ったことを区間へ足す。実行順に呼ばなくても正しく広がる
		void ExtendLifetime(uint32_t a_passIndex)
		{
			if (a_passIndex == INVALID_PASS_INDEX) return;

			if (!HasLifetime())
			{
				m_firstPassIndex = a_passIndex;
				m_lastPassIndex = a_passIndex;
				return;
			}

			if (a_passIndex < m_firstPassIndex) m_firstPassIndex = a_passIndex;
			if (a_passIndex > m_lastPassIndex)  m_lastPassIndex = a_passIndex;
		}

		// アロケーターで割り当てられた場所を覚える
		void SetAllocationInfo(const AllocationInfo& a_info) { m_allocationInfo = a_info; }
		const AllocationInfo& GetAllocationInfo() const { return m_allocationInfo; }

		//----------------------------------------------------------------------------------
		// 実体を使い回せるリソースか
		//
		// ・外部から差し込まれたもの : 実体はグラフの外の持ち物。勝手に他へ貸せない
		// ・履歴つき(Temporal)       : 前フレームに書いた中身を今フレームが読む。
		//                              区間がフレームをまたぐので、この並びだけでは判断できない
		// ・誰も触らないもの         : 区間が無いので比べようがない
		//----------------------------------------------------------------------------------
		bool IsAliasable() const { return !m_isImported && !m_isTemporal && HasLifetime(); }

		// 生存区間が重なっているか。
		// 重なっていなければ、同じ実体を順番に使い回せる
		bool IsLifetimeOverlapped(const VirtualResource& a_other) const
		{
			if (!HasLifetime() || !a_other.HasLifetime()) return false;

			return m_firstPassIndex <= a_other.m_lastPassIndex
				&& a_other.m_firstPassIndex <= m_lastPassIndex;
		}

		// 実体を作ったらヒープをどれだけ食うか。
		// 実サイズ・フォーマット・用途フラグが揃っていないと出せないので、
		// 要件が変わったら ResolveSize から呼び直される
		void CalcAllocationSize();

		uint64_t GetAllocationSize() const { return m_allocationSize; }
		uint64_t GetAllocationAlignment() const { return m_allocationAlignment; }

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

		// 宣言値と土台の解像度から実サイズを決めて、占有サイズまで出す。
		// 土台がまだ無ければ何もしない(サイズを決められないだけで、異常ではない)
		void ResolveSizeAndAllocation();

		// リソーステクスチャの作成情報
		//
		// m_resourceID が同一性。m_name は表示用のラベルでしかないので、
		// 名前が同じでも識別子が違えば別のリソースになる
		ResourceID m_resourceID = {};
		std::string m_name = "none";
		EPassSlotType m_type = EPassSlotType::Texture;

		// --- 要件(出力スロットから起こす) ---
		DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
		Resource::TextureUsage m_usage = Resource::TextureUsage::None;

		//----------------------------------------------------------------------------------
		// サイズ : 宣言値と実サイズを分けて持つ
		//
		// 宣言値は「0 = 描画解像度に従う」の意味を持ったままの、スロットが言ってきた値。
		// 実サイズはそれを土台の解像度で解決した後の値で、実体を作るのはこちら。
		//
		// 分けていないと、一度解決して実サイズが入った後に別の書き手が
		// scale を変えてきても、「もう 0 ではない」と見なされて効かなくなる
		// (描き足しで2つ目の書き手が来るとき)
		//----------------------------------------------------------------------------------
		UINT64 m_declWidth = 0;		// 宣言された横 : バッファのときはバイト数として扱う
		UINT m_declHeight = 0;		// 宣言された縦
		float m_scale = 1.f;		// 宣言が 0 のときに描画解像度へ掛ける倍率

		UINT64 m_baseWidth = 0;		// 土台の描画解像度 : 解決に使う
		UINT m_baseHeight = 0;

		UINT64 m_width = 0;			// 解決後の実サイズ : バッファのときはバイト数
		UINT m_height = 0;

		// --- 予想サイズ ---
		uint64_t m_allocationSize = 0;		// 実体となったときに使用される予定のメモリサイズ
		uint64_t m_allocationAlignment = 0;

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
		uint32_t m_physicalIndex[2] = { INVALID_PHYSICAL_INDEX, INVALID_PHYSICAL_INDEX };
		AllocationInfo m_allocationInfo = {};

		// --- 生存区間(実行順の添字) ---
		// 触るパスが1つもなければどちらも INVALID_PASS_INDEX のまま
		uint32_t m_firstPassIndex = INVALID_PASS_INDEX;
		uint32_t m_lastPassIndex = INVALID_PASS_INDEX;
	};
}
