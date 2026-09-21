#pragma once

//==========================================================================================
// Engine::TypeInfo
//
// RTTI(typeid / dynamic_cast / std::type_index)を使わずに型を扱うための小物。
// このプロジェクトは RTTI を切ってビルドする(/GR-)ので、型が絡むところは全部ここを通す。
//
//   TypeKey      : 型ごとに1つのアドレス。表のキーや一致判定に使う
//   GetTypeName  : ログ用の型名
//   TypeChain    : 実体の型 → 親の型 と辿れる連鎖。基底への変換(Cast)に使う
//
// TypeKey は実行中だけ通じる値。ファイルには書かないこと(保存は登録名で行う)
//==========================================================================================
namespace Engine::TypeInfo
{
	using TypeKey = const void*;

	namespace Internal
	{
		// 型ごとに1つ置く変数。値は使わず、アドレスだけを使う。
		// const にしないこと : 中身が同じ読み取り専用データはリンカの畳み込み(/OPT:ICF)で
		// 1つにまとめられることがあり、別の型が同じキーになってしまう
		template<typename T>
		struct KeyHolder
		{
			inline static char s_tag = 0;
		};
	}

	/// <summary>型ごとのキー</summary>
	template<typename T>
	constexpr TypeKey GetTypeKey()
	{
		return &Internal::KeyHolder<std::remove_cv_t<T>>::s_tag;
	}

	//--------------------------------------------------------------------------------------
	// 型名(ログ用)
	//
	// コンパイラが埋める関数シグネチャから型名を切り出す。
	// 見た目は MSVC の表記(struct Foo など)。識別子として保存・比較には使わないこと
	//--------------------------------------------------------------------------------------
	template<typename T>
	constexpr std::string_view GetTypeName()
	{
		constexpr std::string_view _funcSig = __FUNCSIG__;
		constexpr std::string_view _prefix = "GetTypeName<";
		constexpr std::string_view _suffix = ">(void)";

		const size_t _begin = _funcSig.find(_prefix);
		const size_t _end = _funcSig.rfind(_suffix);
		if (_begin == std::string_view::npos || _end == std::string_view::npos) return _funcSig;

		const size_t _nameBegin = _begin + _prefix.size();
		if (_end <= _nameBegin) return _funcSig;

		return _funcSig.substr(_nameBegin, _end - _nameBegin);
	}

	//======================================================================================
	// 型の連鎖
	//
	// 実体の型から、親の型を順に辿れるようにしたもの。
	// 基底側に「今の実体の連鎖」を1本持たせておき、Cast はそれを辿って調べる。
	//
	// 連鎖に載るのは ENGINE_TYPE_CHAIN_ROOT / ENGINE_TYPE_CHAIN_BASE を書いた型と、
	// 連鎖の先頭になる実体の型だけ。宣言していない途中の型は飛ばされる。
	// そのため、Cast の行き先にできるのは
	//   ・宣言を書いた型(継承されることがある型)
	//   ・final の型(継承されないので、実体の型と一致するかだけ見ればよい)
	// のどちらかに限っている(IsCastTarget_v)。書き忘れはコンパイルで止まる
	//======================================================================================
	struct TypeChain
	{
		TypeKey				key		= nullptr;
		const TypeChain*	pParent	= nullptr;
		std::string_view	name	= {};		// ログ用
	};

	namespace Internal
	{
		// 連鎖の上で T の1つ上に来る型。
		// T 自身が宣言していれば宣言した親、していなければ宣言を持つ一番近い祖先
		// (宣言の別名は派生へそのまま引き継がれるため、TypeChainSelf が T と違えば未宣言)
		template<typename T>
		using ChainParent_t = std::conditional_t<
			std::is_same_v<typename T::TypeChainSelf, T>,
			typename T::TypeChainSuper,
			typename T::TypeChainSelf>;

		template<typename T>
		constexpr const TypeChain* ChainOf();

		template<typename T>
		struct ChainHolder
		{
			inline static const TypeChain s_chain{ GetTypeKey<T>(), ChainOf<ChainParent_t<T>>(), GetTypeName<T>() };
		};

		template<typename T>
		constexpr const TypeChain* ChainOf()
		{
			if constexpr (std::is_void_v<T>) return nullptr;
			else return &ChainHolder<T>::s_chain;
		}
	}

	/// <summary>T を実体としたときの連鎖</summary>
	template<typename T>
	constexpr const TypeChain* GetTypeChain()
	{
		return Internal::ChainOf<std::remove_cv_t<T>>();
	}

	/// <summary>連鎖のどこかに a_key が居るか(＝その型として扱えるか)</summary>
	inline bool IsKindOf(const TypeChain* a_pChain, TypeKey a_key)
	{
		for (const TypeChain* _p = a_pChain; _p != nullptr; _p = _p->pParent)
		{
			if (_p->key == a_key) return true;
		}
		return false;
	}

	/// <summary>Cast の行き先にできる型か</summary>
	template<typename T>
	inline constexpr bool IsCastTarget_v =
		std::is_final_v<T> || std::is_same_v<typename T::TypeChainSelf, T>;

	//--------------------------------------------------------------------------------------
	// dynamic_cast の代わり
	//
	// a_pSource は GetTypeChain() で実体の連鎖を返すこと。
	// 型が合わなければ nullptr
	//--------------------------------------------------------------------------------------
	template<typename Target, typename Source>
	Target* Cast(Source* a_pSource)
	{
		static_assert(std::is_base_of_v<std::remove_cv_t<Source>, std::remove_cv_t<Target>>,
			"Target は Source の派生である必要があります");
		static_assert(IsCastTarget_v<std::remove_cv_t<Target>>,
			"Target には ENGINE_TYPE_CHAIN_BASE を書くか、final を付けてください");

		if (a_pSource == nullptr) return nullptr;
		if (!IsKindOf(a_pSource->GetTypeChain(), GetTypeKey<Target>())) return nullptr;

		return static_cast<Target*>(a_pSource);
	}
}

//------------------------------------------------------------------------------------------
// 連鎖の宣言(クラスの public 部に書く)
//
//   ROOT : 連鎖の根(BaseObject / Pass など、実体の連鎖を持つ基底)
//   BASE : 継承されることがある途中の型。Cast の行き先にしたいときに書く
//          (継承されない型は代わりに final を付ければよい)
//------------------------------------------------------------------------------------------
#define ENGINE_TYPE_CHAIN_ROOT(Self)		\
	using TypeChainSelf = Self;				\
	using TypeChainSuper = void

#define ENGINE_TYPE_CHAIN_BASE(Self, Super)	\
	using TypeChainSelf = Self;				\
	using TypeChainSuper = Super
