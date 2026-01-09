#pragma once

#include "../BaseScene/BaseScene.h"

class ModelResource;

class GameScene : public BaseScene
{
public:

	GameScene() = default;
	~GameScene() = default;

	void Enter() override;

	void Exit() override;

	void Update() override;

	void Draw() override;

private:

	void SetSceneType() override
	{
		m_sceneType = SceneType::Game;
	}

private:

	ECS::Entity m_entity;

	float m_rotateY;

	float m_moveX = 0;

	std::weak_ptr<ModelResource> m_wpModel[5];
	std::weak_ptr<ModelResource> m_wpModel2;

	DirectX::XMFLOAT4X4 m_cameraMat;

	DirectX::XMFLOAT4X4 m_charaMat;
	DirectX::XMFLOAT4X4 m_charaMat2;
};