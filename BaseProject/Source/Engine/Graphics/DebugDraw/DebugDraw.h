#pragma once

namespace Engine::Graphics
{
	/// <summary>
	/// デバッグ用のワイヤー表示を積む場所
	/// </summary>
	/// <remarks>
	/// 形状を1つ積むと DebugLineData が1つ増え、DebugLinePass がまとめて描く。
	///
	/// ここはエンジン層に置いてある。以前は MainEditor が持っていたが、
	/// それだとエンジンとアプリからエディターを名指しすることになり、
	/// 依存が逆流していた。積む先をエンジン側に移したことで、
	/// エディターは「表示するかどうか」を決めるだけになっている。
	///
	/// 積む側の経路 :
	///   ECSのシステム    : SystemContext の pServices->pDebugDraw
	///   GameObject       : ObjectContext の pServices->pDebugDraw
	///   エンジン内部     : GraphicsEngine::RefDebugDraw()
	///
	/// 中身は描き終わりに捨てられる(GraphicsEngine::EndFrame)。
	/// 積んだフレームのうちに描かれるので、消す処理を呼ぶ側が持つ必要はない。
	///
	/// オプション(DebugDrawOption::drawWire)が off のときは1本も積まない。
	/// 空のままなら描画側も早期リターンするので、描画コマンドごと止まる。
	/// </remarks>
	class DebugDraw
	{
	public:

		//===================================================================
		// 積む
		//===================================================================

		void DrawLine(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_endPos,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		void DrawBox(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingBox& a_aabb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingOrientedBox& a_obb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);

		void DrawCapsule(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);

		void DrawSphere(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawSphere(const DirectX::BoundingSphere& a_sphere, const DirectX::SimpleMath::Color& a_color = Color::WHITE);

		/// <summary>
		/// レイを線で出し、当たっていれば終点に球を置く
		/// </summary>
		void DrawRay(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_dir,
			float a_length,
			bool a_isHit,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		//===================================================================
		// 描く側が使う
		//===================================================================

		// 積んだものを捨てる。フレームの頭で1回だけ呼ぶ
		void Clear();

		const std::vector<DebugLineData>& GetLineDataVec() const { return m_lineDataVec; }

	private:

		// 積んでよいか : オプションと上限を見る
		bool CanPush();

	private:

		std::vector<DebugLineData> m_lineDataVec = {};
		UINT m_capacity = 10000;
	};
}
