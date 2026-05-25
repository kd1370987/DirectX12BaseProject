#include "Animation.h"

void Engine::Resource::AnimationKeyQuaternion::Archive(Persistence::Archive& a_ar, int a_idx)
{
	std::string _idxStr = std::to_string(a_idx);
	a_ar.Field("AnimationKeyQuaternion_Time" + _idxStr, time);
	a_ar.Field("AnimationKeyQuaternion_Quat" + _idxStr, quat);
}

void Engine::Resource::AnimationKeyXMFLOAT3::Archive(Persistence::Archive& a_ar, int a_idx)
{
	std::string _idxStr = std::to_string(a_idx);
	a_ar.Field("AnimationKeyXMFLOAT3_Time" + _idxStr, time);
	a_ar.Field("AnimationKeyXMFLOAT3_Vec" + _idxStr, vec);
}

void Engine::Resource::AnimationNode::Archive(Persistence::Archive& a_ar, int a_idx)
{
	std::string _idxStr = std::to_string(a_idx);
	a_ar.Field("AnimationNode_NodeOffset" + _idxStr, nodeOffset);

	size_t _transSize = translations.size();
	a_ar.Field("AnimationNode_TransCount" + _idxStr, _transSize);
	translations.resize(_transSize);

	int _i = 0;
	for (auto& _data : translations)
	{
		_data.Archive(a_ar, _i);
		_i++;
	}

	size_t _rotSize = rotations.size();
	a_ar.Field("AnimationNode_RotCount" + _idxStr, _rotSize);
	rotations.resize(_rotSize);

	_i = 0;
	for (auto& _data : rotations)
	{
		_data.Archive(a_ar, _i);
		_i++;
	}

	size_t _scaleSize = scales.size();
	a_ar.Field("AnimationNode_ScaleCount" + _idxStr, _scaleSize);
	scales.resize(_scaleSize);

	_i = 0;
	for (auto& _data : scales)
	{
		_data.Archive(a_ar, _i);
		_i++;
	}
}

void Engine::Resource::AnimationData::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save, a_fileDir, a_name, "anim");
	_ar.StringField("AnimationData_Name", name);
	_ar.Field("AnimationData_MaxLength", maxLength);

	size_t _nodeSize = nodes.size();
	_ar.Field("AnimationData_NodeCount", _nodeSize);

	int _i = 0;
	for (auto& _node : nodes)
	{
		_node.Archive(_ar, _i);
		_i++;
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

	int _i = 0;
	for (auto& _node : nodes)
	{
		_node.Archive(_ar, _i);
		_i++;
	}
}