#include "Animation.h"

void Engine::Resource::AnimationKeyQuaternion::Archive(Persistence::Archive& a_ar, const std::string& a_prefix)
{
	a_ar.Field(a_prefix + "_Time", time);
	a_ar.Field(a_prefix + "_Quat", quat);
}

void Engine::Resource::AnimationKeyXMFLOAT3::Archive(Persistence::Archive& a_ar, const std::string& a_prefix)
{
	a_ar.Field(a_prefix + "_Time", time);
	a_ar.Field(a_prefix + "_Vec", vec);
}

void Engine::Resource::AnimationNode::Archive(Persistence::Archive& a_ar, const std::string& a_prefix)
{
	a_ar.Field(a_prefix + "_NodeOffset", nodeOffset);


	// ---- Translations ----
	size_t _transSize = translations.size();
	a_ar.Field(a_prefix + "_TransCount", _transSize);
	translations.resize(_transSize);

	for (int _i = 0; _i < _transSize; ++_i)
	{
		auto& _data = translations[_i];
		_data.Archive(a_ar, a_prefix + "_Trans" + std::to_string(_i));
	}

	// ---- Rotations ----
	size_t _rotSize = rotations.size();
	a_ar.Field(a_prefix + "_RotCount", _rotSize);
	rotations.resize(_rotSize);

	for (int _i = 0; _i < _rotSize; ++_i)
	{
		auto& _data = rotations[_i];
		_data.Archive(a_ar, a_prefix + "_Rot" + std::to_string(_i));
	}

	// ---- Scales ----
	size_t _scaleSize = scales.size();
	a_ar.Field(a_prefix + "_ScaleCount", _scaleSize);
	scales.resize(_scaleSize);

	for (int _i = 0; _i < _scaleSize; ++_i)
	{
		auto& _data = scales[_i];
		_data.Archive(a_ar, a_prefix + "_Scale" + std::to_string(_i));
	}
}

void Engine::Resource::AnimationData::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save, a_fileDir, a_name, "anim");
	_ar.StringField("AnimationData_Name", name);
	_ar.Field("AnimationData_MaxLength", maxLength);

	size_t _nodeSize = nodes.size();
	_ar.Field("AnimationData_NodeCount", _nodeSize);

	for (int _i = 0; _i < _nodeSize; ++_i)
	{
		auto _node = nodes[_i];
		_node.Archive(_ar, "Node" + std::to_string(_i));
	}
}

void Engine::Resource::AnimationData::Load(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Load, a_fileDir, a_name, "anim");
	_ar.StringField("AnimationData_Name", name);
	_ar.Field("AnimationData_MaxLength", maxLength);

	size_t _nodeSize = 0;
	_ar.Field("AnimationData_NodeCount", _nodeSize);
	nodes.resize(_nodeSize);

	for (int _i = 0; _i < _nodeSize; ++_i)
	{
		auto& _node = nodes[_i];
		_node.Archive(_ar, "Node" + std::to_string(_i));
	}
}