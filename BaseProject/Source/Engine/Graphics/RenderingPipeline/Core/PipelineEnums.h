#pragma once
//==========================================================================================
//
// PipelineEnums (Engine::Graphics::Pipeline)
//
// スロットとパスの振る舞いを決める列挙。
// 値を持つだけで依存が無いので、パスからもグラフからも気軽に読める
//
//==========================================================================================
namespace Engine::Graphics::Pipeline
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
	//======================================================================================
	// パスを編集した結果
	//
	// 設計図をエディターで触ったとき、カメラが回している実行インスタンスへ
	// どこまで反映し直す必要があるかを表す
	//======================================================================================
	enum class EPassEditResult : uint8_t
	{
		None,		// 何も変わっていない
		Param,		// パラメータだけ : 実行インスタンスへ値を写せばよい
		Structure	// リソースの要件が変わった : グラフごと組み直す
	};
	// パスが使うパイプラインの種類
	enum class EPassPipelineType : uint8_t
	{
		Graphics,
		Compute
	};

	// 実行前にグラフが張るディスクリプタヒープ
	enum class EPassHeapMode : uint8_t
	{
		None,					// パス側で張る
		Default,				// 通常のヒープ
		BindlessWithSampler		// バインドレス + サンプラー
	};
}
