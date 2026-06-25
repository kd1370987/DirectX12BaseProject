#include "Animation.h"

void Engine::Resource::AnimationKeyQuaternion::Archive(Persistence::Archive& a_ar)
{
	a_ar.Field("Time", time);
	a_ar.Field("Quat", quat);
}

void Engine::Resource::AnimationKeyXMFLOAT3::Archive(Persistence::Archive& a_ar)
{
	a_ar.Field("Time", time);
	a_ar.Field("Vec", vec);
}

void Engine::Resource::AnimationNode::Archive(Persistence::Archive& a_ar)
{
	a_ar.Field("_NodeOffset", nodeOffset);

	// ---- Translations ----
	size_t _transSize = translations.size();
	if (a_ar.BeginArray("Translations", _transSize))
	{
		translations.resize(_transSize); // ロード時のためにリサイズ
		for (size_t _i = 0; _i < _transSize; ++_i)
		{
			if (a_ar.BeginObject(_i))
			{
				translations[_i].Archive(a_ar);
				a_ar.EndObject();
			}
		}
		a_ar.EndArray();
	}

	// ---- Rotations ----
	size_t _rotSize = rotations.size();
	if (a_ar.BeginArray("Rotations", _rotSize))
	{
		rotations.resize(_rotSize);
		for (size_t _i = 0; _i < _rotSize; ++_i)
		{
			if (a_ar.BeginObject(_i))
			{
				rotations[_i].Archive(a_ar);
				a_ar.EndObject();
			}
		}
		a_ar.EndArray();
	}

	// ---- Scales ----
	size_t _scaleSize = scales.size();
	if (a_ar.BeginArray("Scales", _scaleSize))
	{
		scales.resize(_scaleSize);
		for (size_t _i = 0; _i < _scaleSize; ++_i)
		{
			if (a_ar.BeginObject(_i))
			{
				scales[_i].Archive(a_ar);
				a_ar.EndObject();
			}
		}
		a_ar.EndArray();
	}
}

void Engine::Resource::AnimationData::Release()
{
	nodes.clear();
}

void Engine::Resource::AnimationData::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save, a_fileDir, a_name, "anim");
	Archive(_ar);
}

void Engine::Resource::AnimationData::Load(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Load, a_fileDir, a_name, "anim");
	Archive(_ar);
}

void Engine::Resource::AnimationData::Load(const std::string& a_filePath)
{
	auto _fileDir = FileUtility::GetDirFromPath(a_filePath);
	auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
	Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "anim");
	Archive(_ar);
}

void Engine::Resource::AnimationData::Archive(Persistence::Archive& a_ar)
{
	a_ar.StringField("AnimationData_Name", name);
	a_ar.Field("AnimationData_MaxLength", maxLength);

	size_t _nodeSize = nodes.size(); // Save時は現在のサイズ、Load時は0が渡されて内部で上書きされる
	if (a_ar.BeginArray("Nodes", _nodeSize))
	{
		nodes.resize(_nodeSize); // Load時は読み込んだサイズでリサイズされる（Save時はそのまま）
		for (size_t _i = 0; _i < _nodeSize; ++_i)
		{
			if (a_ar.BeginObject(_i))
			{
				nodes[_i].Archive(a_ar);
				a_ar.EndObject();
			}
		}
		a_ar.EndArray();
	}
}
