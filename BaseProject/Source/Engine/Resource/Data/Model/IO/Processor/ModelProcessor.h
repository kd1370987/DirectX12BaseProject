#pragma once

#include "../Parser/ParserStruct.h"

namespace Engine::Resource::Processor
{
	//====================================================================================================
	// 加工層
	//
	// パーサが出した「読み込んだままの中間素材(RawModel)」に対して、
	// エンジンの仕様へ合わせるための加工を行う層。
	// ここはCPU処理だけで完結し、GPUリソースもResourceManagerも触らないこと。
	//
	// 座標系の変換をこの層に集約しているので、フォーマットが増えても
	// パーサを増やすだけで済み、Zミラーを再実装する必要がない。
	//====================================================================================================

	/// <summary>
	/// 座標系
	/// </summary>
	enum class ECoordinateSystem : uint8_t
	{
		RightHanded_YUp,		// 右手座標系 Y-Up : GLTF/FBXなど一般的なDCCの出力
		LeftHanded_YUp,			// 左手座標系 Y-Up : このエンジンの標準(前方 +Z)
	};

	/// <summary>
	/// インポート設定
	/// アセットごとに変えたい値はここに集約する。
	/// ゆくゆくは .meta に保存してアセット単位で持たせる想定。
	/// </summary>
	struct ModelImportSettings
	{
		// 変換元・変換先の座標系
		ECoordinateSystem	sourceCoordinate = ECoordinateSystem::RightHanded_YUp;
		ECoordinateSystem	targetCoordinate = ECoordinateSystem::LeftHanded_YUp;

		// 接線が入っていない場合に生成するかどうか
		bool				generateTangents = true;
	};

	class ModelProcessor
	{
	public:
		ModelProcessor() = delete;

		/// <summary>
		/// 中間素材へインポート設定を適用する
		/// </summary>
		/// <param name="a_model">加工対象 : 破壊的に書き換える</param>
		/// <param name="a_settings">インポート設定</param>
		static void Process(Parse::RawModel& a_model, const ModelImportSettings& a_settings);

	private:

		/// <summary>
		/// Z軸ミラーリング
		/// 右手系と左手系の相互変換。頂点・法線・巻き順・行列・アニメーションキーの全てが対象。
		/// </summary>
		static void MirrorZ(Parse::RawModel& a_model);

		/// <summary>
		/// 接線の生成 : 接線を持っていない頂点にだけ与える
		/// 法線を使うため、座標系の変換より後に実行すること
		/// </summary>
		static void GenerateTangents(Parse::RawModel& a_model);
	};
}
