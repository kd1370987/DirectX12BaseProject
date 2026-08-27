#include "RadialBlurSpeedSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "../../../../Components/Camera/RadialBlurComponent.h"

//==========================================================================================
// RadialBlurSpeedSystem
//
// 自機の速さに応じてラジアルブラーの強さを動かす。
//
// 速さは自前で測らず、TPSSystem が既に作っている正規化済みの値
// (TPSCameraStateComponent::currentSpeed01)をそのまま使う。
// あちらは MovementComponent の実速度を上下成分の重み込みで合成し、
// speedReference で 0..1 へ正規化してから指数減衰でなましたもの。
// 同じ元から引くことで、画角の広がり(fovBoost)とブラーの効きが足並みを揃える。
//
//   t        = speedThreshold から 1 の区間を 0..1 へ引き伸ばしたもの
//   goal     = strengthAtSpeed * t
//   current → goal へ responseRate でなまして寄る
//
// しきい値を切っているのは、巡航中にうっすら滲み続けると画面が汚いため。
// 「速いときだけ効く」ほうが速度差が分かりやすい。
//
// TPSCameraStateComponent を読むので、それを書く TPSSystem より後ろに回る。
// この2つを両方持つカメラだけが対象になるので、TPSでないカメラは素通りする
// (その場合 currentStrength は 0 のまま = baseStrength だけが効く)。
//==========================================================================================
namespace
{
	//--------------------------------------------------------------------------------------
	// 指数減衰の補間係数。rate が 0 以下なら追従しない。
	//--------------------------------------------------------------------------------------
	float DampFactor(float a_rate, float a_dt)
	{
		if (!(a_rate > 0.0f) || !(a_dt > 0.0f)) return 0.0f;
		return 1.0f - std::exp(-a_rate * a_dt);
	}
}

void RadialBlurSpeedSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<const TPSCameraStateComponent, RadialBlurComponent>(
		Engine::ECS::ESystemType::Camera,
		"RadialBlurSpeedSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const TPSCameraStateComponent* a_tpsStatArray,
			RadialBlurComponent* a_radialArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TPSCameraStateComponent&	_statComp	= a_tpsStatArray[_i];
				RadialBlurComponent&			_radialComp	= a_radialArray[_i];

				// 無効なら残っている強さを戻しておく。
				// 0 にしないと、切った瞬間の値のまま次に入れたとき飛んで見える
				if (!_radialComp.enable)
				{
					_radialComp.currentStrength = 0.0f;
					continue;
				}

				//============================================================
				// しきい値から上を 0..1 へ引き伸ばす
				//------------------------------------------------------------
				// speedThreshold が 1 以上(=事実上どの速さでも掛けない)でも
				// 0 除算しないよう、分母を先に見る。
				//============================================================
				const float _threshold	= std::clamp(_radialComp.speedThreshold, 0.0f, 1.0f);
				const float _range		= 1.0f - _threshold;

				const float _t = (_range > 1e-4f)
					? std::clamp((_statComp.currentSpeed01 - _threshold) / _range, 0.0f, 1.0f)
					: 0.0f;

				// 目標の引きずり量。baseStrength は速度に関係なく常に乗るので、
				// ここでは速度ぶんだけを作る(送るときに CamSetShaderSystem が足す)
				const float _goalStrength = _radialComp.strengthAtSpeed * _t;

				_radialComp.currentStrength +=
					(_goalStrength - _radialComp.currentStrength)
					* DampFactor(_radialComp.responseRate, a_ctx.dt);
			}
		}
	);
}
