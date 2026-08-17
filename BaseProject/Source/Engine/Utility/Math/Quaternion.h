#pragma once

//==========================================================================================
// Math::Quaternion
//
// ECS のコンポーネントにそのまま置ける POD の回転。
//
// ・掛け算の順序は DirectXMath に合わせる。
//     a * b  =  「a を適用してから b を適用する」
//   数学の記法(b*a)とは逆なので注意。SimpleMath と同じ並びなので、
//   既存コードをそのまま置き換えても結果は変わらない。
// ・DXSM::Quaternion は XMFLOAT4 の派生なので、変換演算子は DXSM 側にだけ用意する
//   (詳しくは Vector3.h のコメント)。
// ・Vector3 / Matrix に依存する処理は前方宣言にして .cpp で実装する。
//==========================================================================================
namespace Math
{
	struct Vector3;
	struct Matrix;

	struct Quaternion
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float x;
		float y;
		float z;
		float w;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ : 既定は単位クォータニオン(無回転)
		constexpr Quaternion() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
		constexpr Quaternion(float a_x, float a_y, float a_z, float a_w) noexcept
			: x(a_x), y(a_y), z(a_z), w(a_w) {}

		//-----------------------------------------------------------------------------------------------------
		// DirectX 相互変換
		constexpr Quaternion(const DirectX::XMFLOAT4& a_value) noexcept
			: x(a_value.x), y(a_value.y), z(a_value.z), w(a_value.w) {}
		operator DXSM::Quaternion() const noexcept { return DXSM::Quaternion(x, y, z, w); }

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr bool operator==(const Quaternion& a_other) const noexcept
		{
			return x == a_other.x && y == a_other.y && z == a_other.z && w == a_other.w;
		}
		constexpr bool operator!=(const Quaternion& a_other) const noexcept { return !(*this == a_other); }

		/// <summary>回転の合成。this を適用してから a_other を適用する回転になる</summary>
		Quaternion operator*(const Quaternion& a_other) const noexcept;
		Quaternion& operator*=(const Quaternion& a_other) noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Length
		constexpr float LengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }
		float Length() const noexcept { return std::sqrt(LengthSquared()); }

		//-----------------------------------------------------------------------------------------------------
		// Normalize
		// 補間や行列化の前に必ず通す。長さ 0 のときは単位クォータニオンへ戻す
		// (0 のまま行列にすると全成分 0 の行列ができて描画が消えるため)
		void Normalize() noexcept;
		[[nodiscard]] Quaternion Normalized() const noexcept;

		//-----------------------------------------------------------------------------------------------------
		// 逆回転
		// ※ どちらも「その場で反転」ではなく、反転したものを返す。
		//    SimpleMath の Conjugate() は破壊的だったので、戻り値を捨てると
		//    黙って無回転のまま進む。捨てたらコンパイルエラーになるようにしてある
		[[nodiscard]] constexpr Quaternion Conjugate() const noexcept { return { -x, -y, -z, w }; }
		[[nodiscard]] Quaternion Inverse() const noexcept;

		constexpr float Dot(const Quaternion& a_other) const noexcept
		{
			return x * a_other.x + y * a_other.y + z * a_other.z + w * a_other.w;
		}

		//-----------------------------------------------------------------------------------------------------
		// 生成 : 他の構造体に依存するものは .cpp 側
		//-----------------------------------------------------------------------------------------------------

		/// <summary>ヨー/ピッチ/ロール(ラジアン)から作る</summary>
		/// <remarks>
		/// 角度は必ずラジアン。プロジェクトの LookAngleComponent 等は度で持っているので、
		/// 呼ぶ側で XMConvertToRadians を通すこと(SimpleMath 時代と同じ)。
		/// </remarks>
		static Quaternion CreateFromYawPitchRoll(float a_yaw, float a_pitch, float a_roll) noexcept;

		/// <summary>軸まわりの回転(軸は正規化済みであること / 角度はラジアン)</summary>
		static Quaternion CreateFromAxisAngle(const Vector3& a_axis, float a_angle) noexcept;

		/// <summary>回転行列から作る</summary>
		static Quaternion CreateFromRotationMatrix(const Matrix& a_matrix) noexcept;

		/// <summary>指定の向きを向く回転を作る(前方は +Z、上は a_up)</summary>
		static Quaternion LookRotation(const Vector3& a_forward, const Vector3& a_up) noexcept;

		/// <summary>オイラー角(ラジアン / x=Pitch, y=Yaw, z=Roll)へ戻す</summary>
		/// <remarks>エディターで角度として見せる用。表示のたびに往復させると誤差が乗るので、
		/// 触られたときだけ組み直すこと。</remarks>
		[[nodiscard]] Vector3 ToEuler() const noexcept;

		//-----------------------------------------------------------------------------------------------------
		// 補間
		//-----------------------------------------------------------------------------------------------------

		/// <summary>球面線形補間。向きの補間は必ずこちらを使う</summary>
		/// <remarks>
		/// 成分ごとの Lerp + 正規化は、2つがほぼ反対を向いた瞬間に結果がゼロ近傍を通って
		/// 破綻し、180度反転する。Slerp は最短経路で回るのでその破綻が無い。
		/// </remarks>
		static Quaternion Slerp(const Quaternion& a_a, const Quaternion& a_b, float a_t) noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Quaternion Identity() noexcept { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
	};
}

//==========================================================================================
// 保存データとの互換チェック
//------------------------------------------------------------------------------------------
// Archive のバイナリは構造体をそのまま memcpy している。
// XMFLOAT系と1バイトでもズレると、既存の .ob* が静かに壊れる(例外も出ない)ので、
// 並びが変わったらここでコンパイルエラーにする。
//==========================================================================================
static_assert(sizeof(Math::Quaternion) == sizeof(DirectX::XMFLOAT4), "Math::Quaternion のサイズが XMFLOAT4 と違う");
static_assert(offsetof(Math::Quaternion, w) == offsetof(DirectX::XMFLOAT4, w), "Math::Quaternion の並びが XMFLOAT4 と違う");
