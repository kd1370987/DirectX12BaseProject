#pragma once

#include "ResourceBuildContext.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	/// <summary>
	/// 単体ロード用のスコープバッチ
	///
	/// モデル経由ではなく単体でリソースを読み込むとき(エディタからのメッシュ差し替えなど)、
	/// 呼び出し元がビルドコンテキストを持っていないことがある。
	/// そのときだけこのクラスでその場でバッチを開き、スコープを抜けるところで実行する。
	///
	/// 逆に言うと、まとめて読むべきものは必ず呼び出し元でバッチを開いてコンテキストを渡すこと。
	/// これを多用するとメッシュ1個ごとのsubmitに逆戻りする。
	/// </summary>
	class ScopedResourceBuild
	{
	public:

		explicit ScopedResourceBuild(bool a_useCopy = true, bool a_useCompute = true);
		~ScopedResourceBuild();

		// コピーもムーブも禁止 : スコープと寿命を一致させる
		ScopedResourceBuild(const ScopedResourceBuild&) = delete;
		ScopedResourceBuild& operator=(const ScopedResourceBuild&) = delete;
		ScopedResourceBuild(ScopedResourceBuild&&) = delete;
		ScopedResourceBuild& operator=(ScopedResourceBuild&&) = delete;

		// ビルドコンテキストの取得
		const ResourceBuildContext& GetContext() const { return m_context; }

	private:

		D3D12::AsyncBuildBatch	m_batch = {};
		ResourceBuildContext	m_context = {};
	};

	/// <summary>
	/// 渡されたコンテキストが使えるならそれを、使えないなら自前のスコープバッチを使う
	///
	/// ローダ経路はコンテキストを省略できるようにしてあるため、
	/// 「渡されていればそれに積む、渡されていなければ自分で開く」を1箇所にまとめる。
	/// </summary>
	class ResourceBuildScope
	{
	public:

		explicit ResourceBuildScope(const ResourceBuildContext* a_pContext)
		{
			if (a_pContext && a_pContext->pDevice)
			{
				m_pContext = a_pContext;
				return;
			}

			// 呼び出し元が持っていないので、その場で開く
			m_upOwned = std::make_unique<ScopedResourceBuild>();
			m_pContext = &m_upOwned->GetContext();
		}

		const ResourceBuildContext& GetContext() const { return *m_pContext; }

	private:

		std::unique_ptr<ScopedResourceBuild>	m_upOwned = nullptr;
		const ResourceBuildContext*				m_pContext = nullptr;
	};
}
