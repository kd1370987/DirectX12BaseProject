#pragma once
namespace Engine
{
	template<typename T>
	class ResourceRef
	{
	public:
		// 実装はリソースマネージャーに書かれている
		ResourceRef() = default;
		ResourceRef(Handle<T> a_h);
		~ResourceRef();
		
		// コピー
		ResourceRef(const ResourceRef& a_other);			// コピーコンストラクタ
		ResourceRef& operator=(const ResourceRef& a_other); // コピー代入

		// ムーブ
		ResourceRef(ResourceRef&& a_other) noexcept;			// ムーブコンストラクタ
		ResourceRef& operator=(ResourceRef&& a_other) noexcept; // ムーブ代入演算子

		Handle<T> GetRaw() const { return m_handle; }

		explicit operator bool() const { return m_handle.IsValid(); } // if(myModel) みたいに書けるようになる

	private:
		Handle<T> m_handle;
	};
}