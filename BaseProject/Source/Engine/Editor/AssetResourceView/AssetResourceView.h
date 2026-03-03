#pragma once

class ModelView;

class AssetResourceView
{
public:

	AssetResourceView();
	~AssetResourceView();

	void Init();

	void Draw();

private:

	std::unique_ptr<ModelView> m_upModelView = nullptr;

};