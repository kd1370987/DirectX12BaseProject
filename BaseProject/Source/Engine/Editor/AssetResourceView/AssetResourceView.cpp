#include "AssetResourceView.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "ModelView/ModelView.h"

AssetResourceView::AssetResourceView()
{
}

AssetResourceView::~AssetResourceView()
{
}

void AssetResourceView::Init()
{
	m_upModelView = std::make_unique<ModelView>();
	m_upModelView->Init();
}

void AssetResourceView::Draw()
{
	m_upModelView->Draw();
}
