#include "UIBase.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

namespace App::Object
{
	void UIBase::Release(Engine::GameObject::ObjectContext& a_context)
	{}

	void UIBase::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		//auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		//if (!_pGE) return;

		//const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
		//const float _w = static_cast<float>(_winOp.windowWidth);
		//const float _h = static_cast<float>(_winOp.windowHeight);
		//if (_w <= 0.0f || _h <= 0.0f) return;

		//for (auto& _elemet : m_elemets)
		//{
		//	// 中心座標 : ピクセル(左上原点/Y下向き) → NDC(中心原点/Y上向き)
		//	DXSM::Vector2 _ndcPos;
		//	_ndcPos.x = _elemet.screenPos.x / _w * 2.0f - 1.0f;
		//	_ndcPos.y = 1.0f - _elemet.screenPos.y / _h * 2.0f;

		//	// サイズ : フルサイズ(px) → NDC半径。
		//	// ベースクアッドが-1〜1(全幅2)で size 分だけ拡縮するため、半径 = px / 解像度 となる。
		//	DXSM::Vector2 _ndcSize;
		//	_ndcSize.x = _elemet.size.x / _w;
		//	_ndcSize.y = _elemet.size.y / _h;

		//	_pGE->SubmitUI(
		//		m_reticleTexRef,	// ResourceRef -> Handle への暗黙変換
		//		_ndcPos,
		//		_ndcSize,
		//		Engine::Color::WHITE
		//	);
		//}
	}
}