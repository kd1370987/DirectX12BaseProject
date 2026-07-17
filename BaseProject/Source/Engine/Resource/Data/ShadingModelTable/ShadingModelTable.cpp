#include "ShadingModelTable.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	std::span<const ResourceRef<Shader>> Engine::Resource::ShadingModelTable::GetShaderHandles(UINT a_passHash) const
	{
		auto _it = m_shaderHandleMap.find(a_passHash);
		if (_it != m_shaderHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	void ShadingModelTable::Archive(Persistence::Archive& a_ar)
	{
		// 基本的なメンバ変数の保存・読み込み
		a_ar.Field("m_typeName", m_typeName);

		// マップの保存・読み込み
		if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
		{
			// ==========================================
			// 保存処理
			// ==========================================
			size_t _mapSize = m_shaderGUIDMap.size();
			if (a_ar.BeginArray("ShaderGUIDMap", _mapSize))
			{
				size_t _idx = 0;
				for (auto& [_passName, _guidVec] : m_shaderGUIDMap)
				{
					if (a_ar.BeginObject(_idx))
					{
						// パス名を保存
						std::string _key = _passName;
						a_ar.Field("PassName", _key);

						// GUID配列を保存
						size_t _guidSize = _guidVec.size();
						if (a_ar.BeginArray("GUIDs", _guidSize))
						{
							for (size_t _i = 0; _i < _guidSize; ++_i)
							{
								if (a_ar.BeginObject(_i))
								{
									a_ar.Field("GUID", _guidVec[_i]);
									a_ar.EndObject();
								}
							}
							a_ar.EndArray();
						}
						a_ar.EndObject();
					}
					_idx++;
				}
				a_ar.EndArray();
			}
		}
		else
		{
			// ==========================================
			// 復元 (Load) 処理
			// ==========================================
			size_t _mapSize = 0;
			if (a_ar.BeginArray("ShaderGUIDMap", _mapSize)) // Load時はここにファイル内の要素数が入る
			{
				m_shaderGUIDMap.clear(); // 既存のデータをクリア

				for (size_t _idx = 0; _idx < _mapSize; ++_idx)
				{
					if (a_ar.BeginObject(_idx))
					{
						// パス名 (Key) を復元
						std::string _passName;
						a_ar.Field("PassName", _passName);

						// GUID配列 (Value) を復元
						std::vector<Engine::GUID> _guidVec;
						size_t _guidSize = 0;
						if (a_ar.BeginArray("GUIDs", _guidSize))
						{
							_guidVec.resize(_guidSize);
							for (size_t _i = 0; _i < _guidSize; ++_i)
							{
								if (a_ar.BeginObject(_i))
								{
									a_ar.Field("GUID", _guidVec[_i]);
									a_ar.EndObject();
								}
							}
							a_ar.EndArray();
						}

						// マップに再登録
						m_shaderGUIDMap.emplace(_passName, _guidVec);
						a_ar.EndObject();
					}
				}
				a_ar.EndArray();
			}

			// シェーダーロード
			m_shaderHandleMap.clear();  // ランタイムデータの初期化
			m_activePassHashes.clear(); // パスハッシュリストの初期化

			for (auto& [_pathName, _shaderGUIDVec] : m_shaderGUIDMap)
			{
				UINT _hash = StringUtility::ToHash(_pathName);

				// 有効なパスとしてハッシュを登録
				m_activePassHashes.push_back(_hash);

				// PSが0個でもパス自体は「有効」として扱うため、必ず空配列で初期化する
				m_shaderHandleMap[_hash] = std::vector<ResourceRef<Shader>>();

				for (auto& _shaderGUID : _shaderGUIDVec)
				{
					m_shaderHandleMap[_hash].push_back(ResourceManager::Instance().Load<Shader>(_shaderGUID));
				}
			}
		}
	}
	const std::vector<Engine::GUID>& ShadingModelTable::GetShaderGUIDs(const std::string& a_passName) const
	{
		static const std::vector<Engine::GUID> _empty = {};

		auto _it = m_shaderGUIDMap.find(a_passName);
		if (_it == m_shaderGUIDMap.end()) { return _empty; }
		return _it->second;
	}
	void ShadingModelTable::EnablePass(const std::string& a_passName)
	{
		if (m_shaderGUIDMap.contains(a_passName)) { return; }

		UINT _hash = StringUtility::ToHash(a_passName);

		// シリアライズ用データの更新
		m_shaderGUIDMap[a_passName] = {};
		m_activePasses.push_back(a_passName);

		// ランタイム用データの更新
		m_shaderHandleMap[_hash] = {};
		m_activePassHashes.push_back(_hash);
	}
	void ShadingModelTable::DisablePass(const std::string& a_passName)
	{
		UINT _hash = StringUtility::ToHash(a_passName);

		// シリアライズ用データの更新
		m_shaderGUIDMap.erase(a_passName);
		auto _itStr = std::find(m_activePasses.begin(), m_activePasses.end(), a_passName);
		if (_itStr != m_activePasses.end())
		{
			m_activePasses.erase(_itStr);
		}

		// ランタイム用データからの即時削除
		m_shaderHandleMap.erase(_hash);
		auto _itHash = std::find(m_activePassHashes.begin(), m_activePassHashes.end(), _hash);
		if (_itHash != m_activePassHashes.end())
		{
			m_activePassHashes.erase(_itHash);
		}
	}
	void ShadingModelTable::AddShader(const std::string& a_passName, const Engine::GUID& a_shaderGUID)
	{
		// 無効なパスには追加させない
		auto _it = m_shaderGUIDMap.find(a_passName);
		if (_it == m_shaderGUIDMap.end()) { return; }

		// シリアライズ用データに追加
		_it->second.push_back(a_shaderGUID);

		// ランタイム用データに追加
		UINT _hash = StringUtility::ToHash(a_passName);
		m_shaderHandleMap[_hash].push_back(ResourceManager::Instance().Load<Shader>(a_shaderGUID));
	}
	void ShadingModelTable::RemoveShader(const std::string& a_passName, size_t a_index)
	{
		auto _it = m_shaderGUIDMap.find(a_passName);
		if (_it == m_shaderGUIDMap.end()) { return; }
		if (a_index >= _it->second.size()) { return; }

		// シリアライズ用データ（GUID）から削除
		_it->second.erase(_it->second.begin() + a_index);

		// ランタイム用データ（ハンドル）からも削除
		UINT _hash = StringUtility::ToHash(a_passName);
		auto& _handleVec = m_shaderHandleMap[_hash];
		if (a_index < _handleVec.size())
		{
			_handleVec.erase(_handleVec.begin() + a_index);
		}
	}
	std::vector<UINT> ShadingModelTable::GetPassHashes() const
	{
		//std::vector<UINT> hashes;
		//hashes.reserve(m_shaderHandleMap.size());
		//for (const auto& [passHash, shaders] : m_shaderHandleMap)
		//{
		//	hashes.push_back(passHash);
		//}
		//return hashes;
		
		return m_activePassHashes; 
		
	}
}