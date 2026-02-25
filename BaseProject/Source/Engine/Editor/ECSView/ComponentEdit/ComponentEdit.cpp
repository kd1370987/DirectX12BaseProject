#include "ComponentEdit.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/Collider.h"

void ComponentEdit::Init()
{
}

void ComponentEdit::Register(ECS::ComponentTypeID a_typeID, const std::vector<FielMeta>& a_data)
{
	EditFunc _func = [this,a_typeID,a_data](const ECS::Entity& a_entity) 
		{
			auto& _metaData = World::Instance().GetComponentMetaData(a_typeID);

			ImGui::Text(_metaData.name.c_str());

			ImGui::Separator();

			uint8_t* _refData = World::Instance().NRefData(a_entity, a_typeID);
			for (auto& _field : a_data)
			{
				void* _data = _refData + _field.offset;

				switch (_field.type)
				{
				case FielMeta::Type::Bool: 
				{
					bool _value = (*(ECS::Flg*)_data) != 0;
					if (ImGui::Checkbox(_field.name, &_value))*(ECS::Flg*)_data = _value ? 1u : 0u;
					break;
				}
				case FielMeta::Type::Float:
				{
					ImGui::DragFloat(_field.name,(float*)_data);
					break;
				}
				case FielMeta::Type::Float2:
				{
					ImGui::DragFloat2(_field.name,(float*)_data);
					break;
				}
				case FielMeta::Type::Float3:
				{
					ImGui::DragFloat3(_field.name, (float*)_data);
					break;
				}
				case FielMeta::Type::Float4:
				{
					ImGui::DragFloat4(_field.name, (float*)_data);
					break;
				}
				case FielMeta::Type::Matrix:
				{
					float* m = (float*)_data;
					for (int i = 0; i < 4; ++i)
					{
						ImGui::DragFloat4("##row", &m[i * 4]);
					}
					break;
				}
				case FielMeta::Type::Text:
				{
					ImGui::Text(_field.name);
					ImGui::Text("%s",(char*)_data);
					break;
				}
				case FielMeta::Type::U32:
				{
					ImGui::InputScalar(_field.name, ImGuiDataType_U32, _data);
					break;
				}
				case FielMeta::Type::U64:
				{
					ImGui::InputScalar(_field.name, ImGuiDataType_U64, _data);
					break;
				}
				case FielMeta::Type::Enum:
				{
					DrawEnumCombo<Layer>(_field.name, *reinterpret_cast<Layer*>(_data));
					break;
				}
				case FielMeta::Type::EnumFlag:
				{
					DrawEnumFlagsCombo<Layer>(_field.name, *reinterpret_cast<Layer*>(_data));
					break;
				}
				default:
					break;
				}

				ImGui::Separator();

			}
		};


	m_editFuncMap[a_typeID] = _func;
}

EditFunc ComponentEdit::GetCompEditFunc(ECS::ComponentTypeID a_typeID)
{
	auto _it = m_editFuncMap.find(a_typeID);
	if (_it != m_editFuncMap.end())
	{
		return _it->second;
	}

	EditFunc _func = [a_typeID](const ECS::Entity& a_entity)
		{
			auto& _metaData = World::Instance().GetComponentMetaData(a_typeID);

			ImGui::Text(_metaData.name.c_str());

		};
	return _func;
}

