#include "UIBase.h"

namespace App::Object
{
	//======================================================================================
	// UIの全体管理
	//======================================================================================
	class UIManager : public Engine::GameObject::BaseObject
	{
	public:



	private:

		std::vector<UIBase*> m_pUIs = {};

	};
}