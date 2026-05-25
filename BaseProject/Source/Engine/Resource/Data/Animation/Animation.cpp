#include "Animation.h"

void Engine::Resource::AnimationKeyQuaternion::Archive(Persistence::Archive& a_ar, int a_idx)
{
	a_ar.Field("AnimationKeyQuaternion_Time" + a_idx,time);
	a_ar.Field("AnimationKeyQuaternion_Quat" + a_idx,quat);
}

void Engine::Resource::AnimationKeyXMFLOAT3::Archive(Persistence::Archive& a_ar, int a_idx)
{
	a_ar.Field("AnimationKeyXMFLOAT3_Time" + a_idx,time);
	a_ar.Field("AnimationKeyXMFLOAT3_Vec" + a_idx,vec);
}

void Engine::Resource::AnimationNode::Archive(Persistence::Archive& a_ar, int a_idx)
{
	a_ar.Field("AnimationNode_NodeOffset" + a_idx,nodeOffset);
	int _i = 0;
	for (auto& _data : translations)
	{
		_data.Archive(a_ar,_i);
		_i++;
	}
	_i = 0;
	for (auto& _data : rotations)
	{
		_data.Archive(a_ar, _i);
		_i++;
	}
	_i = 0;
	for (auto& _data : scales)
	{
		_data.Archive(a_ar, _i);
		_i++;
	}
}

void Engine::Resource::AnimationData::Save(const std::string& a_fileDir,const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save,a_fileDir, a_name, "anim");
	_ar.StringField("AnimationData_Name",name);
	_ar.Field("AnimationData_MaxLength",maxLength);
	int _i = 0;
	for (auto& _node : nodes)
	{
		_node.Archive(_ar,_i);
		_i++;
	}
}

void Engine::Resource::AnimationData::Load(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Load,a_fileDir, a_name, "anim");
	_ar.StringField("AnimationData_Name", name);
	_ar.Field("AnimationData_MaxLength", maxLength);
	int _i = 0;
	for (auto& _node : nodes)
	{
		_node.Archive(_ar, _i);
		_i++;
	}
}
