#pragma once

#include "SystemManager.h"
#include "../Resource/ResourceTypeManager.h"

namespace Engine::ECS
{
	//==========================================================================================
	// タスクの依存の追加宣言
	//
	// RegisterTask / RegisterCustomTask が返す。クエリ(絞り込み)には使わず、
	// 依存(実行順の辺とジョブの待ち合わせ)にだけ数える読み書きをここで足す。
	//
	//   a_world.ActiveTask<const A, B>(...)
	//       .Reads<C>()                    // RefData で読む(持っていないかもしれない・別エンティティ)
	//       .Writes<D>()                   // RefData で書く(子エンティティへの配信など)
	//       .ReadsResource<HitEvents>()    // GetResource で読む
	//       .WritesResource<DeathEvents>();// GetResource で書く(積む・消す)
	//
	// ・クエリの型に足すとアーキタイプが狭まり、持っていないエンティティが対象から外れる。
	//   「持っていれば使う」「別のエンティティのものを引く」読み書きはこちらで宣言する。
	// ・宣言は実際の読み書きと一致させること。宣言に無い読み書きは実行順にも待ち合わせにも出ない。
	// ・クエリの型でも、RefData で別のエンティティの分を読み書きするならここでも宣言する。
	//   ここで宣言した読み書きが絡む衝突は、アーキタイプが重ならなくても待ち合わせる(対象を絞れないため)。
	// ・読み書きが往復すると RAW だけでは循環するので、After / Before で向きを決める。
	//   ECS プロファイラの Systems で、ソートの成否と前後の決まっていない組を確かめること。
	// ・登録の直後(次の BeginFrame のソートより前)に呼ぶこと。
	//==========================================================================================
	class TaskAccess
	{
	public:

		TaskAccess() = default;
		explicit TaskAccess(SystemTask* a_pTask) : m_pTask(a_pTask) {}

		// コンポーネントを読む(絞り込みには使わない)
		template<typename... Comps>
		TaskAccess& Reads()
		{
			(AddComponent<Comps>(true), ...);
			return *this;
		}

		// コンポーネントを書く(絞り込みには使わない)
		template<typename... Comps>
		TaskAccess& Writes()
		{
			(AddComponent<Comps>(false), ...);
			return *this;
		}

		// リソースを読む
		template<typename... Resources>
		TaskAccess& ReadsResource()
		{
			(AddResource<Resources>(true), ...);
			return *this;
		}

		// リソースを書く(値の書き換え・積む・消す)
		template<typename... Resources>
		TaskAccess& WritesResource()
		{
			(AddResource<Resources>(false), ...);
			return *this;
		}

		//--------------------------------------------------------------------------------------
		// 明示の順序(同じフェーズのタスク名で指定する)
		//
		// 読み書きから決まる並び(RAW)より優先する。次の2つに使う。
		//   ・書き手同士や「読んだ後に書く」組の前後を決める(RAW では辺が張られない)
		//   ・読み書きが往復して RAW だけでは循環する組の向きを決める
		// 同じ名前のタスクが複数あれば全部に掛かる。見つからない名前は Sort で警告する。
		//--------------------------------------------------------------------------------------

		// このタスクより先に走らせるもの
		TaskAccess& After(std::string_view a_taskName)
		{
			if (m_pTask) m_pTask->afterNames.emplace_back(a_taskName);
			return *this;
		}
		TaskAccess& After(std::initializer_list<std::string_view> a_taskNames)
		{
			for (std::string_view _name : a_taskNames) After(_name);
			return *this;
		}

		// このタスクより後に走らせるもの
		TaskAccess& Before(std::string_view a_taskName)
		{
			if (m_pTask) m_pTask->beforeNames.emplace_back(a_taskName);
			return *this;
		}
		TaskAccess& Before(std::initializer_list<std::string_view> a_taskNames)
		{
			for (std::string_view _name : a_taskNames) Before(_name);
			return *this;
		}

	private:

		template<typename Comp>
		void AddComponent(bool a_isRead)
		{
			if (!m_pTask) return;

			using _CompType = std::remove_const_t<Comp>;

			// 問い合わせ専用のタグは依存に数えない(RegisterTask と同じ扱い)
			if constexpr (IsQueryOnlyTag_v<_CompType>)
			{
				return;
			}
			else
			{
				const ComponentTypeID _typeID = ComponentMetaRegistry::GetTypeID<_CompType>();
				if (!IsValidTypeID(_typeID))
				{
					ENGINE_WARNING("[ECS] %s : 未登録のコンポーネントを依存に含めようとしました (%s)",
						m_pTask->name.c_str(), std::string(TypeInfo::GetTypeName<_CompType>()).c_str());
					return;
				}

				(a_isRead ? m_pTask->readSig : m_pTask->writeSig).set(_typeID);

				// 別のエンティティに届きうるので、この読み書きが絡む衝突はアーキタイプで見分けない
				(a_isRead ? m_pTask->lookupReadSig : m_pTask->lookupWriteSig).set(_typeID);
			}
		}

		template<typename Resource>
		void AddResource(bool a_isRead)
		{
			if (!m_pTask) return;

			const ResourceTypeID _id = ResourceTypeManager::GetID<std::remove_const_t<Resource>>();
			if (_id >= Limits::MAX_RESOURCE_TYPES)
			{
				ENGINE_WARNING("[ECS] %s : リソースの型が多すぎて依存に含められません (%s)",
					m_pTask->name.c_str(), std::string(TypeInfo::GetTypeName<Resource>()).c_str());
				return;
			}

			(a_isRead ? m_pTask->resReadSig : m_pTask->resWriteSig).set(_id);
		}

	private:

		SystemTask* m_pTask = nullptr;
	};
}
