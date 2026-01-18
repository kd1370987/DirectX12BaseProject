#pragma once

class BaseScene
{
public:

	BaseScene() = default;
	virtual ~BaseScene() = default;
	
	/// <summary>
	/// 初期化
	/// </summary>
	void Enter();

	/// <summary>
	/// 解放
	/// </summary>
	void Exit();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float a_dt);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// シーンのタイプを取得
	/// </summary>
	/// <returns>シーンのタイプ</returns>
	const SceneType& GetType() {return m_sceneType;}

protected:

	virtual void SetSceneType() = 0;

	virtual void Init() {};
	virtual void Release() {};

	virtual void RegistryComponent() {};
	virtual void RegistrySystem() {};
	virtual void RegistryEntity() {};

protected:

	SceneType m_sceneType = SceneType::None;

};