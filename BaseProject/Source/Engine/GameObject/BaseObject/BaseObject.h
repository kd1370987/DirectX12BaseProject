#pragma once
namespace Engine::GameObject
{
	/// <summary>
	/// 引数で持たせる
	/// </summary>
	struct ObjectContext
	{
		float dt;
	};

	/// <summary>
	/// 奥部ジェクトに継承させるベース
	/// </summary>
	class BaseObject
	{
	public:

		BaseObject() = default;
		virtual ~BaseObject() = default;

		virtual void Init(ObjectContext& a_context);
		virtual void Release(ObjectContext& a_context);

		virtual void Update(ObjectContext& a_context);
		virtual void Draw(ObjectContext& a_context);

		bool IsExpired() const { return m_isExpired; }

	protected:

		// 存在フラグ : trueにしたら次フレームの初めにオブジェクトが消去される
		bool m_isExpired = false;
	};
}