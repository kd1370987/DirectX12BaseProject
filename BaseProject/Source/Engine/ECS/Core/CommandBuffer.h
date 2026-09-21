#pragma once
namespace Engine::ECS
{
	// コンポーネントごとの初期値(バイト列)
	using ComponentDataMap = std::unordered_map<ComponentTypeID, std::vector<uint8_t>>;

	// エンティティの移動用
	struct ChangeEntityCmd
	{
		Entity entity;		// エンティティ
		Signature toSig;	// 変更予定シグネチャ

		// 指定したデータに書き換え
		ComponentDataMap dataMap = {};
	};

	// データ付きエンティティ生成用(プレハブ実体化など)
	// シグネチャで生成し、dataMap のバイト列を各コンポーネントへ流し込む。
	struct CreateEntityWithDataCmd
	{
		Signature sig;
		ComponentDataMap dataMap = {};
	};

	//==========================================================================================
	// 構造変更の予約置き場
	//
	// 反復(ForEach / システム)の最中はチャンクを動かせないので、
	// 生成・削除・引っ越し・作り直しはいったんここへ積み、World がまとめて反映する。
	//
	// ここは積むのと取り出すだけで、反映のしかたは知らない。
	// 取り出し(Take*)は中身を渡して空にするので、反映中に積まれたものは次の反映へ回る。
	//==========================================================================================
	class CommandBuffer
	{
	public:

		//------------------------------------------------------------------------------------------
		// 予約
		//------------------------------------------------------------------------------------------
		void ReserveCreate(const Signature& a_sig) { m_createVec.push_back(a_sig); }
		void ReserveCreateWithData(CreateEntityWithDataCmd a_cmd) { m_createWithDataVec.push_back(std::move(a_cmd)); }
		void ReserveRemove(const Entity& a_entity) { m_removeVec.push_back(a_entity); }
		void ReserveChange(ChangeEntityCmd a_cmd) { m_changeVec.push_back(std::move(a_cmd)); }
		void ReserveRefresh(const Entity& a_entity) { m_refreshVec.push_back(a_entity); }

		//------------------------------------------------------------------------------------------
		// 取り出し : 積まれていたものを返して空にする
		//------------------------------------------------------------------------------------------
		std::vector<Signature>					TakeCreate()			{ return Take(m_createVec); }
		std::vector<CreateEntityWithDataCmd>	TakeCreateWithData()	{ return Take(m_createWithDataVec); }
		std::vector<Entity>						TakeRemove()			{ return Take(m_removeVec); }
		std::vector<ChangeEntityCmd>			TakeChange()			{ return Take(m_changeVec); }
		std::vector<Entity>						TakeRefresh()			{ return Take(m_refreshVec); }

		bool HasChange() const { return !m_changeVec.empty(); }

	private:

		template<typename T>
		static std::vector<T> Take(std::vector<T>& a_vec)
		{
			std::vector<T> _out = {};
			_out.swap(a_vec);
			return _out;
		}

	private:

		std::vector<Signature>					m_createVec = {};			// 生成
		std::vector<CreateEntityWithDataCmd>	m_createWithDataVec = {};	// データ付き生成(プレハブ実体化など)
		std::vector<Entity>						m_removeVec = {};			// 削除
		std::vector<ChangeEntityCmd>			m_changeVec = {};			// 引っ越し(シグネチャ変更)
		std::vector<Entity>						m_refreshVec = {};			// 作り直し
	};
}
