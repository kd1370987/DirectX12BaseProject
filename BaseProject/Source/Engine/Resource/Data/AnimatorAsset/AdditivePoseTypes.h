#pragma once
//==========================================================================================
//
// 加算ポーズ(AdditivePose)の共通型
//
// 「どのボーンに、どの配分で加算するか」はモデルごとに決まる構造情報なので、
// AnimatorAsset が設計図として保持する(= ここで定義する AdditiveBoneDef)。
//
// 実行時はノード名ハッシュからノードインデックスへ解決し、
// Application 側の AdditiveBoneEntry としてプールへ展開する。
//
//==========================================================================================

namespace Engine::Resource
{
	//--------------------------------------------------------------------------------------
	// 加算の系統
	//--------------------------------------------------------------------------------------
	enum class EAdditiveChannel : uint8_t
	{
		Aim = 0,	// 照準方向への上半身追従
		LagArm,		// 加速による腕の遅れ
		LagLeg,		// 加速による脚の遅れ

		// ---- 空中(接地していない間)だけ効くもの ----
		// 地上用(Aim / LagArm / LagLeg)とは AdditivePoseComponent::airBlend で切り替える。
		// 保存値は番号なので、足すときは末尾へ
		AimArm,		// 照準方向への腕追従(空中では上半身の Aim の代わりに腕だけで狙う)
		LagBody,	// 速度による体全体の流れ(腰を支点に、進行方向の逆へ脚が流れる)

		Count,
	};

	/// 表示用
	inline const char* ToString(EAdditiveChannel a_channel)
	{
		switch (a_channel)
		{
		case EAdditiveChannel::Aim:		return "Aim";
		case EAdditiveChannel::LagArm:	return "LagArm";
		case EAdditiveChannel::LagLeg:	return "LagLeg";
		case EAdditiveChannel::AimArm:	return "AimArm";
		case EAdditiveChannel::LagBody:	return "LagBody";
		default:						return "Unknown";
		}
	}

	//--------------------------------------------------------------------------------------
	// 加算対象ボーン1本分の定義(設計図側)
	//--------------------------------------------------------------------------------------
	struct AdditiveBoneDef
	{
		std::string			nodeName;							// 対象ノード名(保存用)
		UINT				nodeNameHash = 0;					// ロード時に nodeName から張り直す
		float				share = 1.0f;						// チェーン内での配分(合計1.0が目安)
		Math::Vector3		axisScale = { 1.0f, 1.0f, 1.0f };	// Lag用: 各軸の効き(符号で左右反転)
		EAdditiveChannel	channel = EAdditiveChannel::Aim;

		void Archive(Persistence::Archive& a_arch);
	};
}
