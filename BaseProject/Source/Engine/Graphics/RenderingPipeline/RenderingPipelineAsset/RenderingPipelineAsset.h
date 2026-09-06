#pragma once
//==========================================================================================
//
// RenderingPipelineAsset (Engine::Graphics::Pipeline)
//
// レンダーグラフ1本ぶんを持つアセット。カメラごとに参照する。
//
// パス・つなぎ・実行順は RenderGraph の持ち物で、ここは
// 「グラフを1つ抱える」役に徹する。
// 編集UIは Editor 側(RenderingPipelineEditor)の責務で、ここは ImNodes を知らない。
//
// 中身は unique_ptr と生ポインタで持つだけなので、
// RenderGraph も Pass もこのヘッダーでは前方宣言で足りる
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
{
	class Pass;
	class RenderGraph;
	class PassMetaRegistry;

	// レンダーグラフ1本ぶんを持つアセット
	// カメラごとに参照する
	//
	// パス・つなぎ・実行順は RenderGraph の持ち物で、ここは
	// 「グラフを1つ抱える」役に徹する
	class RenderingPipelineAsset
	{
	public:

		RenderingPipelineAsset();
		~RenderingPipelineAsset();

		// RenderGraph を unique_ptr で抱えるのでコピー禁止。
		// ムーブは許可する : ResourceManager のプールは Add(T&&) で実体をムーブ代入するので、
		// ムーブできないとプールに載せられない
		RenderingPipelineAsset(const RenderingPipelineAsset&) = delete;
		RenderingPipelineAsset& operator=(const RenderingPipelineAsset&) = delete;
		RenderingPipelineAsset(RenderingPipelineAsset&& a_other) noexcept;
		RenderingPipelineAsset& operator=(RenderingPipelineAsset&& a_other) noexcept;

		//----------------------------------------------------------------------------------
		// シリアライズ
		//----------------------------------------------------------------------------------
		// パス構成と配線を保存・読込する。
		// 読込はレジストリから型IDでパスを作り直すので、必ず SetMetaRegistry の後に呼ぶこと
		void Archive(Persistence::Archive& a_arch);

		// 拡張子なしのベースパスへ書き出す
		void Save(const std::string& a_baseFilePath);

		// 表示名 : アセットのファイル名がそのまま入る
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& a_name) { m_name = a_name; }

		// このアセットの拡張子(.oj / .ob の後ろに付く)
		static constexpr const char* kExtension = "rpipe";

		// 生成できるパスの一覧をもらい受ける : 持ち主から渡す(シングルトン直引きはしない)
		void SetMetaRegistry(PassMetaRegistry* a_pRegistry) { m_pMetaRegistry = a_pRegistry; }

		// 編集UI(エディター側)がパスを足すのに要る
		PassMetaRegistry* RefMetaRegistry() const { return m_pMetaRegistry; }

		//----------------------------------------------------------------------------------
		// グラフの出口
		//
		// FinalOutputPass はどのパイプラインにも必ず1つ常駐する。
		// 無ければここで足すので、読み込み直後と編集の入口で必ず通すこと
		//----------------------------------------------------------------------------------
		void EnsureFinalPass();

		// このパスがグラフの出口か(削除させないための判定)
		bool IsFinalPass(const Pass& a_pass) const;

		// 中身のグラフ
		RenderGraph* RefRenderGraph() { return m_upRenderGraph.get(); }
		const RenderGraph* GetRenderGraph() const { return m_upRenderGraph.get(); }

		// 並べ替える : 中身は RenderGraph 側。
		// 通すと Dirty は下りる
		void Compile();

		//----------------------------------------------------------------------------------
		// 構成が変わったかどうか
		//
		// 構成をいじれるのはエディターだけなので、いじったところで立てて
		// Compile ボタンで下ろすだけの簡単なものにしてある。
		// ランタイム中にグラフを組み替えることは想定していない
		//----------------------------------------------------------------------------------
		void SetDirty() { m_isDirty = true; ++m_structureVersion; }
		bool IsDirty() const { return m_isDirty; }

		// パラメータだけが変わった : 組み直しは要らず、カメラ側へ値を写すだけで済む
		void SetParamDirty() { ++m_paramVersion; }

		// 構成が変わるたびに上がる版。
		// カメラごとの実行インスタンスは、これが変わったときだけ組み直す。
		// 0 は「まだ何も組んでいない」印として使うので 1 から始める
		uint32_t GetStructureVersion() const { return m_structureVersion; }

		// パラメータだけが変わるたびに上がる版。
		// カメラ側はこれが変わったら、組み直さずに値を写すだけで済む
		uint32_t GetParamVersion() const { return m_paramVersion; }

	private:

		// 表示名
		std::string m_name = "RenderingPipeline";

		// 生成できるパスの一覧(所有しない)
		PassMetaRegistry* m_pMetaRegistry = nullptr;

		// パス・つなぎ・実行順を持つグラフ本体
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

		// 構成が変わってからコンパイルを通していない。
		// 読み込み直後は一度通す必要があるので true から始める
		bool m_isDirty = true;

		// 構成の版
		uint32_t m_structureVersion = 1;

		// パラメータの版
		uint32_t m_paramVersion = 1;
	};
}
