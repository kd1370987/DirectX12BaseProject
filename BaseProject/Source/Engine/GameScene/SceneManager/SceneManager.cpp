#include "SceneManager.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/PSOManager/PSOManager.h"

#include "Engine/GPUResource/Model/Model.h"

bool SceneManager::Init()
{
	m_spModel = std::make_shared<ModelResource>();
	if (!m_spModel->Load("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX"))
	//if (!m_spModel->Load("Asset/Model/tank/tank.gltf"))
	{
		assert(0 && "FBXモデル読み込みに失敗\n");
		return false;
	}

	// カメラ座標設定
	auto _eyePos = DirectX::XMVectorSet(0.0f, 120.0f, 100.0f, 0.0f);
	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	m_charaMat = DirectX::XMFLOAT4X4
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	return true;
}

bool SceneManager::Release()
{
	return true;
}

void SceneManager::Update()
{
	m_rotateY += 0.05f;

	DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(m_rotateY);
	DirectX::XMStoreFloat4x4(&m_charaMat, rotY);
}

void SceneManager::Draw()
{
	
	RenderContext::Instance().BeginSimpleRender();

	RenderContext::Instance().SetToShader(m_cameraMat);

	RenderContext::Instance().DrawModel(
		m_spModel,
		m_charaMat,
		DirectX::XMFLOAT4{1.0f,0.0f,1.0f,1.0f},
		DirectX::XMFLOAT3{1.0f,1.0f,1.0f}
	);
}
