#include "ModelProcessor.h"

namespace Engine::Resource::Processor
{
	namespace
	{
		//===================================================
		// 行列のZ軸ミラーリング
		//
		// S = diag(1,1,-1,1) としたときの S * A * S と等価。
		// 掛け算・逆行列と可換( M(A*B) == M(A)*M(B), M(A^-1) == M(A)^-1 )なので、
		// ワールド行列を組み立てた後にまとめて適用しても結果は変わらない。
		//===================================================
		void XMFLOAT4X4MirrorZ(DirectX::XMFLOAT4X4& a_mat)
		{
			// 回転のZミラーリング
			a_mat._13 *= -1;
			a_mat._23 *= -1;
			a_mat._31 *= -1;
			a_mat._32 *= -1;

			// 座標のZミラーリング
			a_mat._43 *= -1;
		}

		// 座標系が左右で異なるかどうか
		bool IsHandednessDifferent(ECoordinateSystem a_src, ECoordinateSystem a_dst)
		{
			return a_src != a_dst;
		}
	}

	void ModelProcessor::Process(Parse::RawModel& a_model, const ModelImportSettings& a_settings)
	{
		// 座標系の変換
		if (IsHandednessDifferent(a_settings.sourceCoordinate, a_settings.targetCoordinate))
		{
			MirrorZ(a_model);
		}

		// 接線の生成 : 法線を参照するので座標系変換の後
		if (a_settings.generateTangents)
		{
			GenerateTangents(a_model);
		}
	}

	void ModelProcessor::MirrorZ(Parse::RawModel& a_model)
	{
		//-------------------------------------------------
		// ノード : 各行列
		//-------------------------------------------------
		for (auto& _node : a_model.nodes)
		{
			XMFLOAT4X4MirrorZ(_node.localTransform);
			XMFLOAT4X4MirrorZ(_node.worldTransform);
			XMFLOAT4X4MirrorZ(_node.inverseBindMatrix);
		}

		//-------------------------------------------------
		// メッシュ : 頂点と巻き順
		//-------------------------------------------------
		for (auto& _mesh : a_model.meshes)
		{
			// 座標と法線のZ反転
			for (auto& _vertex : _mesh.vertices)
			{
				_vertex.pos.z *= -1;
				_vertex.normal.z *= -1;
			}

			// Z反転で面が裏返るため、巻き順を入れ替える
			for (auto& _face : _mesh.faces)
			{
				std::swap(_face.idx[1], _face.idx[2]);
			}
		}

		//-------------------------------------------------
		// アニメーション : 各キー
		//-------------------------------------------------
		for (auto& _animation : a_model.animations)
		{
			for (auto& _node : _animation.animationNodes)
			{
				// 座標キー
				for (auto& _key : _node.translations)
				{
					_key.vec.z *= -1;
				}

				// 回転キー : Z軸ミラーではXとYが反転する
				for (auto& _key : _node.rotations)
				{
					_key.quat.x *= -1;
					_key.quat.y *= -1;
				}

				// 拡縮キーは符号が変わらないため何もしない
			}
		}
	}

	void ModelProcessor::GenerateTangents(Parse::RawModel& a_model)
	{
		for (auto& _mesh : a_model.meshes)
		{
			for (auto& _vertex : _mesh.vertices)
			{
				// 接線が存在する場合はスキップ
				DXSM::Vector3 _tangent = _vertex.tangent;
				if (_tangent.LengthSquared() > 0.0f)	continue;

				DXSM::Vector3 _normal = _vertex.normal;

				// 法線と平衡になりにくい基準ベクトルを用意
				DXSM::Vector3 _ref = (fabs(_normal.y) < 0.999f) ? DXSM::Vector3::Up : DXSM::Vector3::Forward;

				// クロス結果を求めて正規化
				DXSM::Vector3 _t = _ref.Cross(_normal);
				_t.Normalize();

				// 結果を格納
				_vertex.tangent = _t;
			}
		}
	}
}
