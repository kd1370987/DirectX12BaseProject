#include "AssetResourceView.h"

#include "ModelView/ModelView.h"
#include "TextureView/TextureView.h"
namespace Engine::Editor
{
	AssetResourceView::AssetResourceView()
	{}

	AssetResourceView::~AssetResourceView()
	{}

	void AssetResourceView::Init()
	{
		m_upModelView = std::make_unique<ModelView>();
		m_upModelView->Init();

		m_upTextureView = std::make_unique<TextureView>();
		m_upTextureView->Init();
	}

	void AssetResourceView::Draw()
	{
		m_upModelView->Draw();

		m_upTextureView->Draw();
	}
}