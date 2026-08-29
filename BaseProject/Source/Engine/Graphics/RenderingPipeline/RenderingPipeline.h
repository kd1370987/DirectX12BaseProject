#pragma once
namespace Engine::Graphics
{
	// リソースのタイプ
	enum class EPassSlotType : UINT
	{
		Texture,
		Buffer
	};

	// レンダーターゲットをクリアするかどうか
	enum class ELoadOp : UINT
	{
		Load,			// クリアしない
		Clear			// クリアする
	};

	// レンダーターゲットがこのパス以降にしようされるかどうか
	// これは外部から設定はしない。基本的にDontCareで次に使われるパスがつながった際に動的にStorになる
	// まだ使用するかは未定だが、エイリアシング用に置いているため未使用
	enum class EStoreOp : UINT
	{
		Store,			// この先で使うことがある
		DontCare		// これ以上後ろで使われることがない
	};

	// アクセスタイプ : 同じリソースでも、アクセスタイプが異なる
	enum class EAccessType
	{
		None,
		SRV,
		RTV,
		Depth_Read,
		Depth_Write,
		UAV,
		CopySrc,
		CopyDst
	};

	// リソース、パスの入出力データ
	struct Slot
	{
		// リソースのステート
		std::string name;		// 名前
		EPassSlotType type;		// リソースタイプ
		EAccessType accessType;	// アクセスタイプ
		DXGI_FORMAT format;		// リソースフォーマット
		
		// リソースサイズ
		UINT64 width = 0;		// 横指定
		UINT height = 0;		// 縦指定
		float scale = 1.f;		// スケール指定 : 上記のサイズに対してかかる

		// リソースのパス間の設定
		ELoadOp loadOp = ELoadOp::Load;				// クリアは明示的に設定する必要がある
		EStoreOp storeOp = EStoreOp::DontCare;		// 後続が出れば自動で Store に変換

		// フレーム間でテンポラルするかどうか : パス間ではパスのつなぎ方で判別するためいらない
		bool isTemporal = false;

		// ランタイム時用
		Handle<Slot> handle = {};		// 自身に割り当てられたメモリ領域参照用

		// エディター用
		bool isIn = true;			// InputPin側かどうか
		std::string pinName = "";	// InputPinのどのピンか

	};

	// グラフの構成要素 : 一つのシェーダーで一回のみの処理となる最小単位
	// 継承先で定数バッファのデータや、アーカイブ処理、GPU処理を入れる
	class Pass
	{
	public:


		Pass() = default;
		virtual ~Pass() = default;

		// 初期化 : 各参照マネージャーなどをもらい受ける予定
		void Init();

		// システム上で前のパスができて、パスのアウトプットスロットから接続された際に呼ばれる
		// 名前とデータを指定して、コンパイル時ようにためておく。
		// データは前のパスに依存するため、このパスでフォーマットなどは指定できない。
		void SetInput(const std::string& a_name,const Slot& a_slotData);

		// システム上でアウトプットを取得したいときに呼ばれる
		// アウトプットスロットはパス内で定義する
		const Slot& GetSlot(const std::string& a_name);

		// コンパイル : パスの設定されている情報からランタイムデータを構築する
		virtual void Compile() = 0;

		// ランタイム中はこの関数のみで処理する
		virtual void Update() = 0;

		// エディター用
		virtual void EditUpdate() = 0;		// パスの情報を編集する用
		virtual void EditNode() = 0;		// パスのノード情報を編集する用

		// シリアライズ
		virtual void Archive() = 0;			// パスの保存、読込関数


	protected:

		// ---- パス情報 ----
		// メタ
		std::string m_name = "";
		

		// シェーダー
		Engine::GUID m_shaderGUID = {};
		Handle<Resource::Shader> m_shaderHandle = {};

		// リソース
		std::vector<Slot> m_inputSlots = {};		// 入力
		std::vector<Slot> m_outputSlots = {};		// 出力

		// ---- エディター用情報 ----
		// ノード
		UINT m_nameHash = 0;
		Math::Vector2 editorPos = {};
		int nodeID = 0;			// ノード自身のID

		struct Pin
		{
			int id = 0;
			std::string name;
		};
		std::vector<Pin> inputPins = {};
		std::vector<Pin> outputPins = {};

	};

	// パスの配列が入ったアセット
	// カメラごとに参照する
	class RenderingPipelineAsset
	{
	public:

	private:

		// パスのインスタンス配列
		std::vector<Pass> m_passes = {};

		// ---- ランタイム時 ----

		// コンパイル後のパス配列
		std::vector<Pass*> m_cmpilePasses = {};

	};
}