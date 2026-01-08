#pragma once

class BaseScene
{
public:

	BaseScene() = default;
	virtual ~BaseScene() = default;
	
	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Enter();

	/// <summary>
	/// 解放
	/// </summary>
	virtual void Exit();

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// シーンのタイプを取得
	/// </summary>
	/// <returns>シーンのタイプ</returns>
	const SceneType& GetType() {return m_sceneType;}

protected:

	virtual void SetSceneType() = 0;

protected:

	SceneType m_sceneType = SceneType::None;

};