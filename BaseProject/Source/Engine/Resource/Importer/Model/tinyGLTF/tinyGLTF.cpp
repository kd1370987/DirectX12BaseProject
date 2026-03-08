#include "tinyGLTF.h"

// TinyGLTF
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "tinygltf/tiny_gltf.h"

//===================================================
// バッファから型を指定して取得する関数
//===================================================
class GLTFBufferGetter
{
public:

	// アクセス番号を指定して初期化・紐づけ
	GLTFBufferGetter(const tinygltf::Model* a_model, int a_accessor)
	{
		// 紐づけ
		m_pModel = a_model;
		m_pAccessor = &a_model->accessors[a_accessor];
		m_pBufferView = &a_model->bufferViews[m_pAccessor->bufferView];
		m_pBuffer = &a_model->buffers[m_pBufferView->buffer];

		// データの先頭アドレス計算
		m_pAddress = &m_pBuffer->data[m_pBufferView->byteOffset + m_pAccessor->byteOffset];
	}

	// float取得
	float GetValue_Float(int a_idx)
	{
		if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_BYTE)                 return Get<char>(a_idx) / (float)(SCHAR_MAX);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)	return Get<BYTE>(a_idx) / (float)UCHAR_MAX;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_SHORT)			return Get<short>(a_idx) / (float)SHRT_MAX;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)	return Get<unsigned short>(a_idx) / (float)USHRT_MAX;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_INT)				return Get<int>(a_idx) / (float)INT_MAX;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)	return Get<unsigned int>(a_idx) / (float)UINT_MAX;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_FLOAT)			return Get<float>(a_idx);

		assert(0 && "対応していない型");
		return 0;
	}

	// int取得
	int GetValue_Int(int a_idx)
	{
		if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_BYTE)					return (int)Get<char>(a_idx);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)	return (int)Get<BYTE>(a_idx);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_SHORT)			return (int)Get<short>(a_idx);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)	return (int)Get<unsigned short>(a_idx);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_INT)				return (int)Get<int>(a_idx);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)		return (int)Get<unsigned int>(a_idx);

		assert(0 && "対応していない型");
		return 0;
	}

	// 値を正規化して取得
	float GetValue_UNORM(int a_idx)
	{
		if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_BYTE)                 return std::max(Get<char>(a_idx) / 127.0f, -1.0f);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)   return Get<BYTE>(a_idx) / 255.0f;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_SHORT)           return std::max(Get<short>(a_idx) / 32767.0f, -1.0f);
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)  return Get<unsigned short>(a_idx) / 65535.0f;
		else if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_FLOAT)           return GetValue_Float(a_idx);

		assert(0 && "対応していない型");
		return 0;
	}

	// ゲッター
	const tinygltf::Accessor* GetAccsessor()  const { return m_pAccessor; }
	const tinygltf::BufferView* GetBufferView() const { return m_pBufferView; }
	const tinygltf::Buffer* GetBuffer()     const { return m_pBuffer; }

private:
	// 型指定型でIndex番目のデータを取得
	template<class Type>
	const Type& Get(int a_index)
	{
		return *(const Type*)&m_pAddress[a_index * sizeof(Type)];
	}

	const BYTE* m_pAddress = nullptr;        // データの先頭アドレス

	const tinygltf::Model* m_pModel = nullptr;         // モデル
	const tinygltf::Accessor* m_pAccessor = nullptr;      // BufferViewのデータ型や要素数、正規化情報を持つ
	const tinygltf::BufferView* m_pBufferView = nullptr;    // Bufferの一部範囲を示す
	const tinygltf::Buffer* m_pBuffer = nullptr;        // 実際のバイナリデータ（頂点座標とか法線、UVなど）
};

//===================================================
// 行列のZ軸ミラーリング
//===================================================
static void XMFLOAT4X4MirrorZ(DirectX::XMFLOAT4X4& a_mat)
{
	// 回転のZミラーリング
	a_mat._13 *= -1;
	a_mat._23 *= -1;
	a_mat._31 *= -1;
	a_mat._32 *= -1;

	// 座標のZミラーリング
	a_mat._43 *= -1;
}


Engine::Resource::Model Engine::Resource::GLTF::Import(const std::string& a_filePath)
{
	// GLTFを読み込み
	auto _spGLTFModel = Load(a_filePath);

	// モデルデータにシリアライズして返す
	return Engine::Resource::GLTF::Serialize(a_filePath,_spGLTFModel);
}

std::shared_ptr<Engine::Resource::GLTF::ModelData> Engine::Resource::GLTF::Load(std::string_view a_filePath)
{
	//===============================================
	//
	// モデルデータの読み込み
	//
	//===============================================
	tinygltf::Model _tinyModel;                     // モデルデータの入れもの
	{
		tinygltf::TinyGLTF _gltf_ctx;               // 読込用オブジェクト
		std::string _err;
		std::string _warn;
		std::string _input_filename(a_filePath);
		std::string _ext = FileUtility::GetFilePathExtension(_input_filename);

		// GLTF読込
		bool _ret = false;
		if (_ext.compare("glb") == 0)
		{
			// GLB読込
			_ret = _gltf_ctx.LoadBinaryFromFile(&_tinyModel, &_err, &_warn, _input_filename.c_str());
		}
		else
		{
			// GLTF読込
			_ret = _gltf_ctx.LoadASCIIFromFile(&_tinyModel, &_err, &_warn, _input_filename.c_str());
		}

		if (!_warn.empty())
		{
			ImGuiContex::Instance().AddLog("Warn : %s\n", _warn.c_str());
		}

		if (!_err.empty())
		{
			ImGuiContex::Instance().AddLog("Err : %s\n", _err.c_str());
		}

		if (!_ret)
		{
			ImGuiContex::Instance().AddLog("Failed to parse gltf: %s", a_filePath.data());
			return nullptr;
		}
	}

	// 戻り値用データを準備
	std::shared_ptr<Engine::Resource::GLTF::ModelData> _destModel = {};
	_destModel = std::make_shared<Engine::Resource::GLTF::ModelData>();

	//----------------------------------------------------
	// マテリアル
	//----------------------------------------------------
	{
		// 指定Indexのテクスチャ名取得
		auto GetTextureFilename = [&_tinyModel](int a_texIndex) -> std::string
			{
				if (a_texIndex < 0) return "";
				int _imgIndex = _tinyModel.textures[a_texIndex].source;
				if (_imgIndex < 0) return "";
				return _tinyModel.images[_imgIndex].uri;
			};

		// マテリアル数だけ、配列を確保
		_destModel->materials.resize(_tinyModel.materials.size());

		// 全マテリアルデータをコピーする
		for (UINT _materialIdx = 0; _materialIdx < _destModel->materials.size(); ++_materialIdx)
		{
			const auto& _srcMaterial = _tinyModel.materials[_materialIdx];        // コピー元確保
			auto& _destMaterial = _destModel->materials[_materialIdx];            // コピー先確保

			// マテリアル名
			_destMaterial.name = _srcMaterial.name;

			// 透明モード設定
			_destMaterial.alphaMode = _srcMaterial.alphaMode;			   // モード
			_destMaterial.AlphaCutoff = (float)(_srcMaterial.alphaCutoff); // 閾値
			_destMaterial.doubleSided = _srcMaterial.doubleSided;          // 設定面

			// 基本色
			_destMaterial.baseColorTexName = GetTextureFilename(_srcMaterial.pbrMetallicRoughness.baseColorTexture.index); // 名前
			if (_srcMaterial.pbrMetallicRoughness.baseColorFactor.size() == 4)
			{
				// 乗算用値
				_destMaterial.baseColorFactor = {
					static_cast<float>(_srcMaterial.pbrMetallicRoughness.baseColorFactor[0]),
					static_cast<float>(_srcMaterial.pbrMetallicRoughness.baseColorFactor[1]),
					static_cast<float>(_srcMaterial.pbrMetallicRoughness.baseColorFactor[2]),
					static_cast<float>(_srcMaterial.pbrMetallicRoughness.baseColorFactor[3])
				};
			}


			// 金属製・粗さ
			_destMaterial.metallicRoughnessTexName = GetTextureFilename(_srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
			_destMaterial.metallicFactor = static_cast<float>(_srcMaterial.pbrMetallicRoughness.metallicFactor);
			_destMaterial.roughnessFactor = static_cast<float>(_srcMaterial.pbrMetallicRoughness.roughnessFactor);

			// エミッシブ
			_destMaterial.emissiveTexName = GetTextureFilename(_srcMaterial.emissiveTexture.index);
			if (_srcMaterial.emissiveFactor.size() == 3)
			{
				_destMaterial.emissiveFactor = {
					static_cast<float>(_srcMaterial.emissiveFactor[0]),
					static_cast<float>(_srcMaterial.emissiveFactor[1]),
					static_cast<float>(_srcMaterial.emissiveFactor[2])
				};
			}

			// 法線マップ
			_destMaterial.normalTexName = GetTextureFilename(_srcMaterial.normalTexture.index);

			// オクルージョンマップ
			_destMaterial.occlusionTexName = GetTextureFilename(_srcMaterial.occlusionTexture.index);
		}

		// マテリアルがゼロの場合は、１つだけ作成しておく
		if (_destModel->materials.size() == 0)
		{
			_destModel->materials.resize(1);
		}
	}

	//----------------------------------------------------
	// ノード
	//----------------------------------------------------

	_destModel->nodes.resize(_tinyModel.nodes.size());      // 全ノード分メモリを確保
	//-------------------------
	// 全ノード　基本情報設定
	//-------------------------
	for (UINT _nodeIdx = 0; _nodeIdx < _destModel->nodes.size(); ++_nodeIdx)
	{
		auto* _destNode = &_destModel->nodes[_nodeIdx];     // コピー先
		_destNode->localTransform = DXSM::Matrix::Identity;
		_destNode->worldTransform = DXSM::Matrix::Identity;
		_destNode->inverseBindMatrix = DXSM::Matrix::Identity;

		//-----------------------
		// 情報
		//-----------------------
		_destNode->name = _tinyModel.nodes[_nodeIdx].name;              // 名前
		_destNode->children = _tinyModel.nodes[_nodeIdx].children;      // 子インデックス配列

		// 全ての子に、親を設定
		for (auto&& _idx : _destNode->children)
		{
			_destModel->nodes[_idx].parent = _nodeIdx;
		}

		//-----------------------
		// 変換行列取得
		//-----------------------
		DXSM::Matrix _sMat = DXSM::Matrix::Identity;
		DXSM::Matrix _rMat = DXSM::Matrix::Identity;
		DXSM::Matrix _tMat = DXSM::Matrix::Identity;
		// 拡縮
		if (_tinyModel.nodes[_nodeIdx].scale.size() != 0)
		{
			_sMat = DXSM::Matrix::CreateScale(
				static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[0]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[1]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[2])
			);
		}
		// 回転
		if (_tinyModel.nodes[_nodeIdx].rotation.size() != 0)
		{
			DXSM::Quaternion _quat(
				static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[0]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[1]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[2]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[3])
			);
			_rMat = DXSM::Matrix::CreateFromQuaternion(_quat);
		}
		// 移動
		if (_tinyModel.nodes[_nodeIdx].translation.size() != 0)
		{
			_tMat = DXSM::Matrix::CreateTranslation(
				static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[0]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[1]),
				static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[2])
			);
		}
		// 行列
		if (_tinyModel.nodes[_nodeIdx].matrix.size() != 0)
		{
			for (int _n = 0; _n < 16; ++_n)
			{
				*(&_sMat._11 + _n) = (float)_tinyModel.nodes[_nodeIdx].matrix[_n];
			}
		}

		// 変換行列を格納
		_destNode->localTransform = _sMat * _rMat * _tMat;

		// Z軸ミラーリング
		XMFLOAT4X4MirrorZ(_destNode->localTransform);

		// メッシュあり
		if (_tinyModel.nodes[_nodeIdx].mesh >= 0)
		{
			// メッシュフラグON
			_destNode->isMesh = true;
		}
	}
	//----------------------------------------------------
	// ノードノードのみの参照リスト
	//----------------------------------------------------
	for (auto&& _idx : _tinyModel.scenes[0].nodes)
	{
		_destModel->rootNodeIndices.push_back(_idx);
	}

	//----------------------------------------------------
	// 各ノードのTransformからWorldTransformを算出
	//----------------------------------------------------
	{
		// 行列計算用再起関数
		std::function<void(Engine::Resource::GLTF::Node*, const DirectX::XMFLOAT4X4*)> _rec =
			[&_rec, &_destModel](Engine::Resource::GLTF::Node* a_node, const DirectX::XMFLOAT4X4* a_parentMat)
			{
				// ワールド行列をもとめる
				if (a_parentMat)
				{
					DXSM::Matrix _localMat(a_node->localTransform);
					DXSM::Matrix _parentMat(*a_parentMat);
					a_node->worldTransform = _localMat * _parentMat;
				}
				else
				{
					a_node->worldTransform = a_node->localTransform;
				}


				// 子の再帰
				for (auto&& _child : a_node->children)
				{
					_rec(&_destModel->nodes[_child], &a_node->worldTransform);
				}
			};

		// 親子関係から行列を作成
		for (int _nodeIdx : _destModel->rootNodeIndices)
		{
			_rec(&_destModel->nodes[_nodeIdx], nullptr);
		}
	}

	//----------------------------------------------------
	// ボーン
	//----------------------------------------------------
	if (_tinyModel.skins.size() > 0)
	{
		// 配列確保
		_destModel->boneNodeIndices = _tinyModel.skins[0].joints;

		// InverseBindMarices(オフセット行列)取得用
		GLTFBufferGetter _ibmGetter(&_tinyModel, _tinyModel.skins[0].inverseBindMatrices);

		// ボーンだけのノード参照配列
		for (UINT _jointIdx = 0; _jointIdx < _tinyModel.skins[0].joints.size(); ++_jointIdx)
		{
			// _jointIdx番目のボーンの、ノード内でのIndex
			int _originNodeIdx = _tinyModel.skins[0].joints[_jointIdx];

			Engine::Resource::GLTF::Node* _boneNode = &_destModel->nodes[_originNodeIdx];
			_boneNode->boneNodeIndex = _jointIdx;

			// オフセット行列取得
			DXSM::Matrix _invBindMat = DXSM::Matrix::Identity;
			for (int _matIdx = 0; _matIdx < 16; ++_matIdx)
			{
				(&_invBindMat._11)[_matIdx] = _ibmGetter.GetValue_Float(_jointIdx * 16 + _matIdx);
			}

			// Z軸ミラーリングして格納
			XMFLOAT4X4MirrorZ(_invBindMat);
			_boneNode->inverseBindMatrix = _invBindMat;

			// 変換行列へ変換
			_boneNode->worldTransform = _invBindMat.Invert();
		}

		// ボーンLocalMat算出
		for (int _nodeIdx : _destModel->boneNodeIndices)
		{
			Engine::Resource::GLTF::Node* _boneNode = &_destModel->nodes[_nodeIdx];

			if (_boneNode->parent >= 0)
			{
				DXSM::Matrix _boneWorldMat(_boneNode->worldTransform);
				DXSM::Matrix _invBindMat(_destModel->nodes[_boneNode->parent].inverseBindMatrix);
				_boneNode->localTransform = _boneWorldMat * _invBindMat;
			}
			else
			{
				_boneNode->localTransform = _boneNode->worldTransform;
			}
		}
	}

	//----------------------------------------------------
	// メッシュ
	//----------------------------------------------------
	for (UINT _nodeIdx = 0; _nodeIdx < _destModel->nodes.size(); ++_nodeIdx)
	{
		auto* _destNode = &_destModel->nodes[_nodeIdx]; // コピー先確保

		//-----------------------
		// メッシュの場合
		//-----------------------
		int _meshIdx = _tinyModel.nodes[_nodeIdx].mesh;     // メッシュIndex取得
		if (_meshIdx < 0)continue;                          // メッシュなし

		// メッシュフラグON
		_destNode->isMesh = true;

		// 作業データ
		struct GLTFPrimitive
		{
			std::vector<Engine::Resource::MeshVertex8bit> vertices = {};
			std::vector<Engine::Resource::MeshFace>       faces = {};

			UINT                        materialNumber = 0;

			std::map<std::string, int>  attributes = {};
		};
		std::vector<std::shared_ptr<GLTFPrimitive>> _tmpPrimitives(_tinyModel.meshes[_meshIdx].primitives.size());

		//-----------------------
		// 全プリミティブ
		//-----------------------
		for (size_t _primitiveIdx = 0; _primitiveIdx < _tinyModel.meshes[_meshIdx].primitives.size(); ++_primitiveIdx)
		{
			// コピー元準備
			auto& _srcPrimitive = _tinyModel.meshes[_meshIdx].primitives[_primitiveIdx];

			// TRIANGLES以外は無視（メッシュの基本図形はこれだけで十分だから）
			if (_srcPrimitive.mode != TINYGLTF_MODE_TRIANGLES)continue;

			// 作成
			std::shared_ptr<GLTFPrimitive> _destPrimitive = std::make_shared<GLTFPrimitive>();
			_tmpPrimitives[_primitiveIdx] = _destPrimitive;
			_destPrimitive->attributes = _srcPrimitive.attributes;

			// マテリアルナンバー
			_destPrimitive->materialNumber = std::max(0, _srcPrimitive.material);


			//-----------------------
			// 頂点バッファ
			//-----------------------

			// 座標
			{
				// 座標ゲッター生成
				GLTFBufferGetter _posGetter(&_tinyModel, _srcPrimitive.attributes["POSITION"]);

				_destPrimitive->vertices.resize(_posGetter.GetAccsessor()->count);      // 配列確保
				for (UINT _vertexIdx = 0; _vertexIdx < _posGetter.GetAccsessor()->count; ++_vertexIdx)
				{
					// コピー先
					auto& _ver = _destPrimitive->vertices[_vertexIdx];

					// ガード
					if (_posGetter.GetAccsessor()->type != TINYGLTF_TYPE_VEC3)
					{
						assert(0 && "この頂点形式には対応していません");
					}

					// コピー
					_ver.pos.x = _posGetter.GetValue_Float(_vertexIdx * 3 + 0);
					_ver.pos.y = _posGetter.GetValue_Float(_vertexIdx * 3 + 1);
					_ver.pos.z = _posGetter.GetValue_Float(_vertexIdx * 3 + 2) * -1;    // Z軸ミラー
				}
			}

			// 法線
			if (_srcPrimitive.attributes.count("NORMAL") > 0)
			{
				// 法線ゲッター生成
				GLTFBufferGetter _normalGetter(&_tinyModel, _srcPrimitive.attributes["NORMAL"]);
				for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
				{
					// コピー先設定
					auto& _nor = _destPrimitive->vertices[_vertexIdx].normal;
					// コピー
					_nor.x = _normalGetter.GetValue_Float(_vertexIdx * 3 + 0);
					_nor.y = _normalGetter.GetValue_Float(_vertexIdx * 3 + 1);
					_nor.z = _normalGetter.GetValue_Float(_vertexIdx * 3 + 2) * -1;    // Z軸ミラー
				}
			}

			// UV
			if (_srcPrimitive.attributes.count("TEXCOORD_0") > 0)
			{
				// UVゲッター生成
				GLTFBufferGetter _uvGetter(&_tinyModel, _srcPrimitive.attributes["TEXCOORD_0"]);
				for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
				{
					// コピー先設定
					auto& _uv = _destPrimitive->vertices[_vertexIdx].uv;
					// コピー
					_uv.x = _uvGetter.GetValue_UNORM(_vertexIdx * 2 + 0);
					_uv.y = _uvGetter.GetValue_UNORM(_vertexIdx * 2 + 1);
				}
			}

			// 頂点カラー
			if (_srcPrimitive.attributes.count("COLOR_0") > 0)
			{
				// 色ゲッター生成
				GLTFBufferGetter _colorGetter(&_tinyModel, _srcPrimitive.attributes["COLOR_0"]);

				for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
				{
					DirectX::XMFLOAT4 _color(1, 1, 1, 1);

					// RGB
					if (_colorGetter.GetAccsessor()->type == TINYGLTF_TYPE_VEC3)
					{
						_color.x = _colorGetter.GetValue_Float(_vertexIdx * 3 + 0);
						_color.y = _colorGetter.GetValue_Float(_vertexIdx * 3 + 1);
						_color.z = _colorGetter.GetValue_Float(_vertexIdx * 3 + 2);
					}
					// RGBA
					else if (_colorGetter.GetAccsessor()->type == TINYGLTF_TYPE_VEC4)
					{
						_color.x = _colorGetter.GetValue_Float(_vertexIdx * 4 + 0);
						_color.y = _colorGetter.GetValue_Float(_vertexIdx * 4 + 1);
						_color.z = _colorGetter.GetValue_Float(_vertexIdx * 4 + 2);
						_color.w = _colorGetter.GetValue_Float(_vertexIdx * 4 + 3);
					}

					// float(0.0～1.0) -> byte(0～255)に変換
					unsigned char _r = static_cast<unsigned char>(std::clamp(_color.x, 0.0f, 1.0f) * 255.0f);
					unsigned char _g = static_cast<unsigned char>(std::clamp(_color.y, 0.0f, 1.0f) * 255.0f);
					unsigned char _b = static_cast<unsigned char>(std::clamp(_color.z, 0.0f, 1.0f) * 255.0f);
					unsigned char _a = static_cast<unsigned char>(std::clamp(_color.w, 0.0f, 1.0f) * 255.0f);

					// RGBA -> 32Bit UINTにパック
					unsigned int _packedColor = (_r) | (_g << 8) | (_b << 16) | (_a << 24);
					_destPrimitive->vertices[_vertexIdx].color = _packedColor;
				}

			}

			// スキンメッシュ情報が無ければ無視
			if (_tinyModel.skins.size() > 0)
			{
				// スキンIndex
				if (_srcPrimitive.attributes.count("JOINTS_0") > 0)
				{
					_destNode->nodeMesh.isSkinMesh = true;      // スキンメッシュ持ち
					GLTFBufferGetter _jointGetter(&_tinyModel, _srcPrimitive.attributes["JOINTS_0"]);
					for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
					{
						auto& _skinIdx = _destPrimitive->vertices[_vertexIdx].skinIndexList;

						_skinIdx[0] = static_cast<short>(_jointGetter.GetValue_Int(_vertexIdx * 4 + 0));
						_skinIdx[1] = static_cast<short>(_jointGetter.GetValue_Int(_vertexIdx * 4 + 1));
						_skinIdx[2] = static_cast<short>(_jointGetter.GetValue_Int(_vertexIdx * 4 + 2));
						_skinIdx[3] = static_cast<short>(_jointGetter.GetValue_Int(_vertexIdx * 4 + 3));
					}
				}

				// スキンウェイト
				if (_srcPrimitive.attributes.count("WEIGHTS_0") > 0)
				{
					_destNode->nodeMesh.isSkinMesh = true;      // スキンメッシュ持ち
					GLTFBufferGetter _weightGetter(&_tinyModel, _srcPrimitive.attributes["WEIGHTS_0"]);
					for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
					{
						auto& _skinWeights = _destPrimitive->vertices[_vertexIdx].skinWeightList;

						_skinWeights[0] = _weightGetter.GetValue_UNORM(_vertexIdx * 4 + 0);
						_skinWeights[1] = _weightGetter.GetValue_UNORM(_vertexIdx * 4 + 1);
						_skinWeights[2] = _weightGetter.GetValue_UNORM(_vertexIdx * 4 + 2);
						_skinWeights[3] = _weightGetter.GetValue_UNORM(_vertexIdx * 4 + 3);

						if (_skinWeights[0] == 0) _skinWeights[0] = 1.0f;

						// ウェイト補正
						int _cnt = 0;
						for (UINT _x = 0; _x < 4; ++_x)
						{
							if (_skinWeights[_x] == 0.0f)break;
							_cnt++;
						}
						float _totalWeight = 0;
						for (int _x = 0; _x < _cnt - 1; ++_x)
						{
							_totalWeight += _skinWeights[_x];
						}
						_skinWeights[_cnt - 1] = 1.0f - _totalWeight;
					}

				}
			}

			//-----------------------
			// インデックスバッファ
			//-----------------------
			GLTFBufferGetter _indexGetter(&_tinyModel, _srcPrimitive.indices);      // ゲッター生成
			_destPrimitive->faces.resize(_indexGetter.GetAccsessor()->count / 3);   // 面の数分配列を確保
			for (UINT _faceIdx = 0; _faceIdx < _destPrimitive->faces.size(); ++_faceIdx)
			{
				// データ型のバイト数をもとめる（Z軸ミラーのため、1 と 2 を入れ替える）
				_destPrimitive->faces[_faceIdx].idx[0] =
					static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 0));
				_destPrimitive->faces[_faceIdx].idx[2] =
					static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 1));
				_destPrimitive->faces[_faceIdx].idx[1] =
					static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 2));
			}
		}

		//-----------------------
		// マテリアル
		//-----------------------
		// ソート
		std::sort(
			_tmpPrimitives.begin(),
			_tmpPrimitives.end(),
			[](std::shared_ptr<GLTFPrimitive> a_v1, std::shared_ptr<GLTFPrimitive>a_v2)
			{
				return a_v1->materialNumber < a_v2->materialNumber;
			}
		);

		// マテリアルの最大数分サブセット作成
		_destNode->nodeMesh.subsets.resize(_tmpPrimitives.size());
		for (UINT _priIdx = 0; _priIdx < _tmpPrimitives.size(); ++_priIdx)
		{
			// マテリアル番号
			_destNode->nodeMesh.subsets[_priIdx].materialNumber = _tmpPrimitives[_priIdx]->materialNumber;
		}

		// 全プリミティブを合成し、１つのメッシュにする
		UINT _currentVertexIdx = 0;
		UINT _currentFaceIdx = 0;
		for (UINT _priIdx = 0; _priIdx < _tmpPrimitives.size(); ++_priIdx)
		{
			// 参照先確保
			const auto& _primitive = _tmpPrimitives[_priIdx];

			// 頂点バッファの合成
			if (_primitive->vertices.size() >= 1)
			{
				UINT _st = static_cast<UINT>(_destNode->nodeMesh.vertices.size());
				_destNode->nodeMesh.vertices.resize(_destNode->nodeMesh.vertices.size() + _primitive->vertices.size());
				memcpy(&_destNode->nodeMesh.vertices[_st],
					&_primitive->vertices[0], _primitive->vertices.size() * sizeof(Engine::Resource::MeshVertex8bit)
				);
			}

			// インデックス合成
			if (_primitive->faces.size() >= 1)
			{
				UINT _st = static_cast<UINT>(_destNode->nodeMesh.faces.size());
				_destNode->nodeMesh.faces.resize(_destNode->nodeMesh.faces.size() + _primitive->faces.size());
				// 反転するため 0, 2, 1の順番にする（通常は 0, 1, 2の順番）
				for (UINT _faceIdx = 0; _faceIdx < _primitive->faces.size(); ++_faceIdx)
				{
					_destNode->nodeMesh.faces[_st + _faceIdx].idx[0] = _primitive->faces[_faceIdx].idx[0] + _currentVertexIdx;
					_destNode->nodeMesh.faces[_st + _faceIdx].idx[1] = _primitive->faces[_faceIdx].idx[1] + _currentVertexIdx;
					_destNode->nodeMesh.faces[_st + _faceIdx].idx[2] = _primitive->faces[_faceIdx].idx[2] + _currentVertexIdx;
				}
			}

			// サブセット
			_destNode->nodeMesh.subsets[_priIdx].faceCount += static_cast<UINT>(_primitive->faces.size());		// 面数を加算

			// 頂点数・面数を次の開始位置に設定
			_currentVertexIdx += static_cast<UINT>(_primitive->vertices.size());
			_currentFaceIdx += static_cast<UINT>(_primitive->faces.size());
		}

		// このプリミティブの作業を終了
		_tmpPrimitives.clear();

		// サブセットのオフセットをもとめる
		UINT _offset = 0;
		for (UINT _priIdx = 0; _priIdx < _destNode->nodeMesh.subsets.size(); ++_priIdx)
		{
			_destNode->nodeMesh.subsets[_priIdx].faceStart = _offset;		// 面描画の開始Index
			_offset += _destNode->nodeMesh.subsets[_priIdx].faceCount;		// 次のプリミティブの開始位置設定
		}

		// メッシュの全長点の接線を計算する（任意接線生成）
		for (auto&& _vertex : _destNode->nodeMesh.vertices)
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

	//----------------------------------------------------
	// アニメーション
	//----------------------------------------------------
	for (UINT _animaIdx = 0; _animaIdx < _tinyModel.animations.size(); ++_animaIdx)
	{
		// 参照元用意
		const auto& _srcAnima = _tinyModel.animations[_animaIdx];

		// アニメーションデータに追加
		std::shared_ptr<Engine::Resource::GLTF::AnimationData> _spAnimation = {};
		_spAnimation = std::make_shared<Engine::Resource::GLTF::AnimationData>();

		_destModel->animations.push_back(_spAnimation);

		// 名前
		_spAnimation->name = _srcAnima.name;

		// アニメーションノード
		std::vector<std::shared_ptr<Engine::Resource::AnimationNode>> _tmpNodes;		// 一時的な作業データ準備
		_tmpNodes.resize(_destModel->nodes.size());					// 配列確保

		// 全チャンネル
		for (const auto& _channel : _srcAnima.channels)
		{
			// 参照元用意
			const auto& _sampler = _srcAnima.samplers[_channel.sampler];

			// 対象ノードのIndex
			auto& _destAnimaNode = _tmpNodes[_channel.target_node];

			// 初回のみ
			if (_destAnimaNode == nullptr)
			{
				_destAnimaNode = std::make_shared<Engine::Resource::AnimationNode>();
				_destAnimaNode->nodeOffset = _channel.target_node;
			}

			GLTFBufferGetter _timeGetter(&_tinyModel, _sampler.input);		// 時間アクセサ
			GLTFBufferGetter _valueGetter(&_tinyModel, _sampler.output);	// データアクセサ

			// 座標のアニメーションノード設定
			if (_channel.target_path == "translation")
			{
				for (UINT _kIdx = 0; _kIdx < _timeGetter.GetAccsessor()->count; ++_kIdx)
				{
					Engine::Resource::AnimationKeyXMFLOAT3 _vec;

					// 時間
					_vec.time = _timeGetter.GetValue_Float(_kIdx) * 60.0f;		// 元が60fpsとして変換
					if (_vec.time > _spAnimation->maxLength)
					{
						_spAnimation->maxLength = _vec.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2) * -1;		// Z軸ミラー
						_destAnimaNode->translations.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2) * -1;		// Z軸ミラー
						_destAnimaNode->translations.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 9 + 3);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 9 + 4);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 9 + 5) * -1;		// Z軸ミラー
						_destAnimaNode->translations.push_back(_vec);						// 保存
					}
				}
			}
			else if (_channel.target_path == "scale")
			{
				for (UINT _kIdx = 0; _kIdx < _timeGetter.GetAccsessor()->count; ++_kIdx)
				{
					Engine::Resource::AnimationKeyXMFLOAT3 _vec;

					// 時間
					_vec.time = _timeGetter.GetValue_Float(_kIdx) * 60.0f;		// 元が60fpsとして変換
					if (_vec.time > _spAnimation->maxLength)
					{
						_spAnimation->maxLength = _vec.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode->scales.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode->scales.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 9 + 3);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 9 + 4);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 9 + 5);
						_destAnimaNode->scales.push_back(_vec);						// 保存
					}
				}
			}
			else if (_channel.target_path == "rotation")
			{
				for (UINT _kIdx = 0; _kIdx < _timeGetter.GetAccsessor()->count; ++_kIdx)
				{
					Engine::Resource::AnimationKeyQuaternion _quat;

					// 時間
					_quat.time = _timeGetter.GetValue_Float(_kIdx) * 60.0f;		// 元が60fpsとして変換
					if (_quat.time > _spAnimation->maxLength)
					{
						_spAnimation->maxLength = _quat.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 4 + 0) * -1;	// Z軸ミラー
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 4 + 1) * -1;
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 4 + 2);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 4 + 3);
						_destAnimaNode->rotations.push_back(_quat);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 4 + 0) * -1;	// Z軸ミラー
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 4 + 1) * -1;
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 4 + 2);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 4 + 3);
						_destAnimaNode->rotations.push_back(_quat);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 12 + 4) * -1;// Z軸ミラー
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 12 + 5) * -1;
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 12 + 6);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 12 + 7);
						_destAnimaNode->rotations.push_back(_quat);						// 保存
					}
				}
			}
		}

		// アニメーションで使用していない不必要なノードを除外したリストを作成
		_spAnimation->spAnimationNodes.clear();
		for (auto&& _n : _tmpNodes)
		{
			if (_n == nullptr)continue;
			_spAnimation->spAnimationNodes.push_back(_n);
		}
	}

	// シリアライズしたモデルを返す
	return _destModel;
}
void CreateNodes(
	Engine::Resource::Model& a_dst,
	const std::shared_ptr<Engine::Resource::GLTF::ModelData>& a_src
)
{
	//=================================================
	// ノード作成
	//=================================================
	a_dst.originalNodes.resize(a_src->nodes.size());		// ノード配列確保
	for (UINT _i = 0; _i < static_cast<UINT>(a_src->nodes.size()); ++_i)
	{
		// ノード情報
		const Engine::Resource::GLTF::Node& _srcNode = a_src->nodes[_i];	// 入力元
		Engine::Resource::Node& _dstNode = a_dst.originalNodes[_i];		// 出力先

		// 基本情報コピー
		if (_srcNode.isMesh)
		{
			// メッシュ作成
			//_dstNode.spMesh = std::make_shared<Mesh>();
			auto _spMesh = std::make_shared<Engine::Resource::Mesh>();

			// メッシュデータコピー
			//if (_dstNode.spMesh)
			if (_spMesh)
			{
				// 頂点配列作成
				std::vector<Engine::Resource::MeshVertexFloat> _vertices = {};
				_vertices.resize(_srcNode.nodeMesh.vertices.size());
				for (size_t _j = 0; _j < _srcNode.nodeMesh.vertices.size(); ++_j)
				{
					Engine::Resource::MeshVertexFloat _dstVertex = {};

					unsigned int _srcColor = _srcNode.nodeMesh.vertices[_j].color;
					float r = ((float)((_srcColor >> 24) & 0xFF)) / 255.0f;
					float g = ((float)((_srcColor >> 16) & 0xFF)) / 255.0f;
					float b = ((float)((_srcColor >> 8) & 0xFF)) / 255.0f;
					float a = ((float)((_srcColor >> 0) & 0xFF)) / 255.0f;
					_dstVertex.color = DirectX::XMFLOAT4(r, g, b, a);

					_dstVertex.normal = _srcNode.nodeMesh.vertices[_j].normal;
					_dstVertex.pos = _srcNode.nodeMesh.vertices[_j].pos;
					_dstVertex.tangent = _srcNode.nodeMesh.vertices[_j].tangent;
					_dstVertex.uv = _srcNode.nodeMesh.vertices[_j].uv;

					_dstVertex.skinIndexList = _srcNode.nodeMesh.vertices[_j].skinIndexList;
					_dstVertex.skinWeightList = _srcNode.nodeMesh.vertices[_j].skinWeightList;

					_vertices[_j] = _dstVertex;
				}

				// メッシュ作成
				//_dstNode.spMesh->CreateFloat(
				_spMesh->CreateFloat(
					_vertices,
					_srcNode.nodeMesh.faces,
					_srcNode.nodeMesh.subsets,
					_srcNode.nodeMesh.isSkinMesh
				);
			}

			// メッシュノードリストにインデックス登録
			a_dst.meshNodeIndices.push_back(_i);

			_dstNode.meshIndices.push_back(static_cast<int>(a_dst.spMeshVec.size()));
			a_dst.spMeshVec.push_back(_spMesh);
		}

		// ノード情報セット
		_dstNode.name = _srcNode.name;										// ノード名
		_dstNode.localTransform = _srcNode.localTransform;					// ローカル行列
		_dstNode.worldTransform = _srcNode.worldTransform;					// ワールド行列
		_dstNode.boneInverseWorldMatrix = _srcNode.inverseBindMatrix;		// ボーンのオフセット行列

		_dstNode.parent = _srcNode.parent;									// 親インデックス
		_dstNode.children = _srcNode.children;								// 子供リスト

		_dstNode.boneIndex = _srcNode.boneNodeIndex;						// ボーンインデックス
		_dstNode.isSkinMesh = _srcNode.nodeMesh.isSkinMesh;					// スキンメッシュ持ちかどうか

		// 当たり判定用ノードの検索
		if (_dstNode.name.find("COL") != std::string::npos)
		{
			// 判定用ノードに割り当て
			a_dst.collisionMeshNodeIndices.push_back(_i);
		}
		else
		{
			// 描画用ノードに割り当て
			a_dst.drawMeshNodeIndices.push_back(_i);
		}
	}

	//=================================================
	// ルートノード・ボーンノードリスト作成
	//=================================================
	for (UINT _nodeIdx = 0; _nodeIdx < a_src->nodes.size(); ++_nodeIdx)
	{
		// ルートノードの登録
		if (a_src->nodes[_nodeIdx].parent == -1)
		{
			a_dst.rootNodeIndices.push_back(_nodeIdx);
		}
		// ボーンノードの登録
		int _boneIdx = a_src->nodes[_nodeIdx].boneNodeIndex;		// ボーンインデックス取得
		if (_boneIdx >= 0)
		{
			// ボーンノードリストのサイズ確保
			if (_boneIdx >= (int)a_dst.boneNodeIndices.size())
			{
				a_dst.boneNodeIndices.resize(_boneIdx + 1);
			}

			// 登録
			a_dst.boneNodeIndices[_boneIdx] = _nodeIdx;
		}
	}

	// 当たり判定用ノードがなければ、a_dst.drawMeshNodeと同じ割り当てを行う
	if (a_dst.collisionMeshNodeIndices.size() == 0)
	{
		a_dst.collisionMeshNodeIndices = a_dst.drawMeshNodeIndices;

		for (auto& _idx : a_dst.collisionMeshNodeIndices)
		{
			for (auto& _meshIdx : a_dst.originalNodes[_idx].meshIndices)
			{
				a_dst.spMeshVec[_meshIdx]->CreateCollision();
			}
		}
	}
}
void CreateMaterials(
	Engine::Resource::Model& a_dst, 
	const std::shared_ptr<Engine::Resource::GLTF::ModelData>& a_src,
	const std::string& a_fileDir
)
{
	//=================================================
	// マテリアル作成
	//=================================================

	// マテリアル配列を受け取れるサイズのメモリを確保
	a_dst.materials.resize(a_src->materials.size());

	for (UINT _i = 0; _i < a_dst.materials.size(); ++_i)
	{
		// マテリアル情報
		const Engine::Resource::GLTF::Material& _srcMaterial = a_src->materials[_i];	// 入力元
		Engine::Resource::Material& _dstMaterial = a_dst.materials[_i];					// 出力先

		// マテリアル情報コピー
		_dstMaterial.name = _srcMaterial.name;									// マテリアル名

		// アルファ
		if (_srcMaterial.alphaMode == "OPAQUE")
		{
			_dstMaterial.alphaMode = Engine::Resource::Alpha::Opaque;
		}
		else if (_srcMaterial.alphaMode == "MASK")
		{
			_dstMaterial.alphaMode = Engine::Resource::Alpha::Mask;
		}
		else if (_srcMaterial.alphaMode == "BLEND")
		{
			_dstMaterial.alphaMode = Engine::Resource::Alpha::Blend;
		}

		// 材質データ
		_dstMaterial.baseColor = _srcMaterial.baseColorFactor;				// 基本色
		_dstMaterial.metallic = _srcMaterial.metallicFactor;				// 金属製
		_dstMaterial.roughness = _srcMaterial.roughnessFactor;				// 粗さ
		_dstMaterial.emissive = _srcMaterial.emissiveFactor;				// エミッシブ

		// テクスチャセット
		_dstMaterial.SetTexture2D(
			a_fileDir,
			_srcMaterial.baseColorTexName,
			_srcMaterial.metallicRoughnessTexName,
			_srcMaterial.emissiveTexName,
			_srcMaterial.normalTexName
		);
	}
}
void CreateAnimations(
	Engine::Resource::Model& a_dst, 
	const std::shared_ptr<Engine::Resource::GLTF::ModelData>& a_src
)
{
	//=================================================
	// アニメーション作成
	//=================================================
	a_dst.spAnimations.resize(a_src->animations.size());		// アニメーション配列確保

	for (UINT _i = 0; _i < a_dst.spAnimations.size(); ++_i)
	{
		auto _srcAnimation = a_src->animations[_i];	// 元データ

		a_dst.spAnimations[_i] = std::make_shared<Engine::Resource::AnimationData>();
		Engine::Resource::AnimationData& _dstAnimation = *a_dst.spAnimations[_i];							// 出力先

		// アニメーション情報コピー
		_dstAnimation.name = _srcAnimation->name;									// 名前
		_dstAnimation.maxLength = _srcAnimation->maxLength;							// アニメーションの長さ

		_dstAnimation.nodes.resize(_srcAnimation->spAnimationNodes.size());			// ノード配列確保
		for (UINT _nIdx = 0; _nIdx < _dstAnimation.nodes.size(); ++_nIdx)
		{
			auto _srcNode = _srcAnimation->spAnimationNodes[_nIdx];	// 元データ
			Engine::Resource::AnimationNode& _dstAnimaNode = _dstAnimation.nodes[_nIdx];						// 出力先
			// ノード情報コピー
			_dstAnimaNode.nodeOffset = _srcNode->nodeOffset;				// 対象ノードのオフセット
			_dstAnimaNode.translations = _srcNode->translations;			// 座標キーリスト
			_dstAnimaNode.rotations = _srcNode->rotations;					// 回転キーリスト
			_dstAnimaNode.scales = _srcNode->scales;						// 拡縮キーリスト
		}

	}
}

Engine::Resource::Model Engine::Resource::GLTF::Serialize(
	const std::string& a_filePath,
	std::shared_ptr<Engine::Resource::GLTF::ModelData> a_spGLTFModel
)
{
	// ファイルのディレクトリパスを取得
	std::string _fileDir = FileUtility::GetDirFromPath(a_filePath);

	// モデル構造体を準備
	Engine::Resource::Model _model = {};
	_model.materials.clear();
	_model.originalNodes.clear();
	_model.rootNodeIndices.clear();
	_model.boneNodeIndices.clear();
	_model.meshNodeIndices.clear();
	_model.drawMeshNodeIndices.clear();
	_model.collisionMeshNodeIndices.clear();
	_model.spAnimations.clear();

	// ノード作成
	CreateNodes(_model, a_spGLTFModel);

	// マテリアル作成
	CreateMaterials(_model, a_spGLTFModel, _fileDir);

	// アニメーション
	CreateAnimations(_model, a_spGLTFModel);

	return _model;
}
