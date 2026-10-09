#pragma once
namespace Engine::Graphics
{
	// カメラ
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/CameraData.hlsli)と並びを合わせること。
	//    定数バッファの256バイト境界は CBAllocator が置き場所を取るときにそろえるので、構造体には付けない
	struct CameraData
	{
		// 現在フレームのデータ
		// ジッターありデータ
		Math::Matrix viewMat = {};			// ビュー行列
		Math::Matrix projMat = {};			// 射影行列
		Math::Matrix viewInvMat = {};		// ビュー行列
		Math::Matrix projInvMat = {};		// 射影逆行列
		Math::Matrix viewProjMat = {};
		Math::Matrix invViewProjMat = {};

		// モーションベクター用
		Math::Matrix nonJitteredProj;		// ジッターなし投影行列
		Math::Matrix nonJitteredViewProj;	// ジッターなしビュープロジェクション行列
		Math::Matrix nonJitteredInvViewProj;	// ジッターなしビュープロジェクション行列

		// 1フレーム前のデータ
		Math::Matrix prevView;
		Math::Matrix prevProj;
		Math::Matrix prevViewProj;


		Math::Vector4 pos = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
		Math::Vector2 jitterOffset = {};
		Math::Vector2 prevJitterOffset = {};

		Math::Vector4 frustumPlanes[6] = {};

		// カリングに使うカメラ(視点と視錐台6面)。増幅シェーダーのメッシュレットカリングが読む。
		// 普段は描くカメラと同じ値(UseSelfAsCullCamera)。
		// エディターで「カリングをゲームカメラで行う」にしたときだけ、描く視点(フリーカメラ)と
		// 別のカメラになり、間引かれたメッシュレットを外から見られる
		Math::Vector4 cullPos = { 0.0f,0.0f,0.0f,0.0f };
		Math::Vector4 cullFrustumPlanes[6] = {};

		/// <summary>
		/// フラスタムの平面を求める : すでに構造体内にデータが入っている前提での処理
		/// </summary>
		void ExtractFrustumPlanes(const Math::Matrix& viewProj) // ★引数で転置前の行列を受け取る
		{
			CalcFrustumPlanes(viewProj, frustumPlanes);
		}

		// カリング用カメラを、このカメラ自身(pos / frustumPlanes)にそろえる。
		// ExtractFrustumPlanes の後に呼ぶこと
		void UseSelfAsCullCamera()
		{
			cullPos = pos;
			for (int _i = 0; _i < 6; ++_i)
			{
				cullFrustumPlanes[_i] = frustumPlanes[_i];
			}
		}

		// カリング用カメラを、別のカメラの視点と行列から作る
		// a_viewProj : 転置前・ジッターなしのビュー×射影
		void SetCullCamera(const Math::Vector4& a_pos, const Math::Matrix& a_viewProj)
		{
			cullPos = a_pos;
			CalcFrustumPlanes(a_viewProj, cullFrustumPlanes);
		}

		// ビュー×射影(転置前)から視錐台の6面を求める
		static void CalcFrustumPlanes(const Math::Matrix& a_viewProj, Math::Vector4 (&a_outPlanes)[6])
		{
			const auto& _m = a_viewProj;
			// 各平面の抽出 : x , y , z は法線ベクトル , w は原点からの距離
			// [0] 左平面
			a_outPlanes[0] = Math::Vector4(_m._14 + _m._11, _m._24 + _m._21, _m._34 + _m._31, _m._44 + _m._41);
			// [1] 右平面
			a_outPlanes[1] = Math::Vector4(_m._14 - _m._11, _m._24 - _m._21, _m._34 - _m._31, _m._44 - _m._41);
			// [2] 下平面
			a_outPlanes[2] = Math::Vector4(_m._14 + _m._12, _m._24 + _m._22, _m._34 + _m._32, _m._44 + _m._42);
			// [3] 上平面
			a_outPlanes[3] = Math::Vector4(_m._14 - _m._12, _m._24 - _m._22, _m._34 - _m._32, _m._44 - _m._42);
			// [4] 近平面 (DirectXはZが0～1なので _13 等になる)
			a_outPlanes[4] = Math::Vector4(_m._13, _m._23, _m._33, _m._43);
			// [5] 遠平面
			a_outPlanes[5] = Math::Vector4(_m._14 - _m._13, _m._24 - _m._23, _m._34 - _m._33, _m._44 - _m._43);

			// 平面の平均化（正規化）
			// 法線(xyz)の長さで割る。距離(w)も同じ倍率で揃える必要があるので4成分まとめて掛ける
			for (auto& _plane : a_outPlanes)
			{
				const float _len = Math::Vector3(_plane.x, _plane.y, _plane.z).Length();

				// 長さ0の平面は0のままにする(0除算でNaNを撒かない)
				_plane *= (_len > 0.0f) ? (1.0f / _len) : 0.0f;
			}
		}
	};
}
