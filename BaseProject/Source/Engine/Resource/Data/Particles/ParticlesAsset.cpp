#include "ParticlesAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Data/Texture/IO/TextureIO.h"

namespace Engine::Resource
{
	void Engine::Resource::ParticlesAsset::Create(const std::string& a_name, const Engine::GUID& a_guid)
	{
		m_name = a_name;
		m_guid = a_guid;
	}
	void ParticlesAsset::Release()
	{
		m_name = "";
		m_guid = {};
		m_texGUID = {};
	}
	void ParticlesAsset::Save(const std::string & a_filePath)
	{
		// アーカイブ作成
		auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		Persistence::Archive _archi(Persistence::Archive::Mode::Save,_fileDir,_fileName,"ptic");

		// 保存
		_archi.Field("m_name",m_name);
		_archi.Field("m_guid",m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin",m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax",m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);
	}
	void ParticlesAsset::Load(const std::string & a_fileDir, const std::string & a_fileName)
	{
		// アーカイブ作成
		Persistence::Archive _archi(Persistence::Archive::Mode::Load, a_fileDir, a_fileName, "ptic");

		// 読み込み
		_archi.Field("m_name", m_name);
		_archi.Field("m_guid", m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin", m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax", m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);

		// キャパシティが0だとリソース生成ができないので最低値を入れておく
		if (m_capacity == 0)
		{
			m_capacity = 1000;
		}

		// テクスチャのハンドル取得
		m_texHandle = ResourceManager::Instance().Load<Texture>(m_texGUID);
	}
	void ParticlesAsset::Load(const std::string& a_filePath)
	{
		// アーカイブ作成
		auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		Persistence::Archive _archi(Persistence::Archive::Mode::Load, _fileDir, _fileName, "ptic");

		// 読み込み
		_archi.Field("m_name", m_name);
		_archi.Field("m_guid", m_guid);
		_archi.Field("m_texGUID", m_texGUID);
		_archi.Field("m_initialSpeedMin", m_initialSpeedMin);
		_archi.Field("m_initialSpeedMax", m_initialSpeedMax);
		_archi.Field("m_gravityPow", m_gravityPow);
		_archi.Field("m_ligeTimeMin", m_lifeTimeMin);
		_archi.Field("m_ligeTimeMax", m_lifeTimeMax);
		_archi.Field("m_capacity", m_capacity);
		_archi.Field("m_emissionRate", m_emissionRate);

		// キャパシティが0だとリソース生成ができないので最低値を入れておく
		if (m_capacity == 0)
		{
			m_capacity = 1000;
		}

		// テクスチャのハンドル取得
		m_texHandle = ResourceManager::Instance().Load<Texture>(m_texGUID);
	}
	void ParticlesAsset::EditImGui()
	{
		if (ImGui::Button("Save"))
		{
			// ファイルパス取得
			auto _path = AssetDatabase::Instance().GetFilePathFromGUID(m_guid);
			Save(_path);
			Editor::MainEditor::Instance().AddLog("%s", _path.c_str());
			Editor::MainEditor::Instance().AddLog(" : Save Particles\n");
		}

		// パラメーター変更
		ImGui::InputText("Name", &m_name);
		ImGui::Text("%s",m_guid.String().c_str());

		ImGui::Separator();

		ImGui::PushID(1);
		ImGui::Text("InitialSpeed");
		ImGui::DragFloat("Min", &m_initialSpeedMin, 0.1f, 0.0);
		ImGui::DragFloat("Max", &m_initialSpeedMax, 0.1f, 0.0);
		ImGui::PopID();

		ImGui::Separator();

		ImGui::DragFloat("GravityPow", &m_gravityPow, 0.1f, 0.0f);

		ImGui::Separator();

		ImGui::PushID(2);
		ImGui::Text("LifeTime");
		ImGui::DragFloat("Min", &m_lifeTimeMin, 0.1f, 0.0);
		ImGui::DragFloat("Max", &m_lifeTimeMax, 0.1f, 0.0);
		ImGui::PopID();

		ImGui::Separator();

		ImGui::DragInt("Capacity", &m_capacity, 1, 0);
		ImGui::DragInt("EmissionRate", &m_emissionRate, 1, 0);

		ImGui::Separator();

		// 現在選択されているテクスチャの名前と画像を表示
		const auto* _pTex = ResourceManager::Instance().Get(m_texHandle);
		std::string _viewName = "Selecte...";
		if (!_pTex)
		{
			ImGui::Text("Not selected texture");
		}
		else
		{
			_viewName = _pTex->GetName();
			ImGui::Text("Texture : %s", _viewName.c_str());
			ImGui::Text("%s",m_texGUID.String().c_str());
		}
		ImGui::Separator();

		// テクスチャ選択コンボボックス
		if (ImGui::BeginCombo("SelectTexture",_viewName.c_str()))
		{
			for (auto& _prop : AssetDatabase::Instance().GetTypeMetaVec("Texture"))
			{
				// 現在のテクスチャと同じならフラグを立てる
				bool _selected = (m_texGUID == _prop.guid);

				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					// テクスチャのハンドル取得
					// ロードされていなかったら止まる
					m_texHandle = TextureIO::LoadTexture(_prop.guid, TexColor::WHITE);
					m_texGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}

		// テクスチャの画像を表示
		if(_pTex)
		{
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			Editor::Helper::DrawSRVView(_gpuHandle, _pTex->GetDesc().Width, _pTex->GetDesc().Height);
		}
	}
}