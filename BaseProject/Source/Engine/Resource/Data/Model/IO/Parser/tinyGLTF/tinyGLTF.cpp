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

tinygltf::Model tinyLoadFromFile(std::string_view a_filePath)
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
		std::string _ext = Engine::File::GetFilePathExtension(_input_filename);

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
			ENGINE_WARNING("tinyGLTF : ParseWarn : %s", _warn.c_str());
		}
		if (!_err.empty())
		{
			ENGINE_WARNING("tinyGLTF : ParseErr : %s",_err.c_str());
		}
		if (!_ret)
		{
			ENGINE_WARNING("tinyGLTF : パースに失敗 : %s",a_filePath.data());
		}
	}
	return _tinyModel;
}

// マテリアルパース
std::vector<Engine::Resource::Parse::RawMaterial> ParseMaterial(const tinygltf::Model& a_tinyModel)
{
	std::vector<Engine::Resource::Parse::RawMaterial> _result = {};

	// 指定Indexのテクスチャ名取得
	auto GetTextureFilename = [&a_tinyModel](int a_texIndex) -> std::string
		{
			if (a_texIndex < 0) return "";
			int _imgIndex = a_tinyModel.textures[a_texIndex].source;
			if (_imgIndex < 0) return "";
			return a_tinyModel.images[_imgIndex].uri;
		};

	// マテリアル数だけ、配列を確保
	_result.resize(a_tinyModel.materials.size());

	// 全マテリアルデータをコピーする
	for (UINT _materialIdx = 0; _materialIdx < _result.size(); ++_materialIdx)
	{
		const auto& _srcMaterial = a_tinyModel.materials[_materialIdx];        // コピー元確保
		auto& _destMaterial = _result[_materialIdx];            // コピー先確保

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
	if (_result.size() == 0)
	{
		_result.resize(1);
	}

	return _result;
}

std::vector<Engine::Resource::Parse::RawNode> ParseNode(const tinygltf::Model& a_tinyModel)
{
	std::vector<Engine::Resource::Parse::RawNode> _result = {};
	_result.resize(a_tinyModel.nodes.size());      // 全ノード分配列を確保
	//-------------------------
	// 全ノード　基本情報設定
	//-------------------------
	for (UINT _nodeIdx = 0; _nodeIdx < _result.size(); ++_nodeIdx)
	{
		auto* _destNode = &_result[_nodeIdx];     // コピー先
		_destNode->localTransform = DXSM::Matrix::Identity;
		_destNode->worldTransform = DXSM::Matrix::Identity;
		_destNode->inverseBindMatrix = DXSM::Matrix::Identity;

		//-----------------------
		// 情報
		//-----------------------
		_destNode->name = a_tinyModel.nodes[_nodeIdx].name;              // 名前
		_destNode->children = a_tinyModel.nodes[_nodeIdx].children;      // 子インデックス配列

		// 全ての子に、親を設定
		for (auto&& _idx : _destNode->children)
		{
			_result[_idx].parent = _nodeIdx;
		}

		//-----------------------
		// 変換行列取得
		//-----------------------
		DXSM::Matrix _sMat = DXSM::Matrix::Identity;
		DXSM::Matrix _rMat = DXSM::Matrix::Identity;
		DXSM::Matrix _tMat = DXSM::Matrix::Identity;
		// 拡縮
		if (a_tinyModel.nodes[_nodeIdx].scale.size() != 0)
		{
			_sMat = DXSM::Matrix::CreateScale(
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].scale[0]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].scale[1]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].scale[2])
			);
		}
		// 回転
		if (a_tinyModel.nodes[_nodeIdx].rotation.size() != 0)
		{
			DXSM::Quaternion _quat(
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].rotation[0]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].rotation[1]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].rotation[2]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].rotation[3])
			);
			_rMat = DXSM::Matrix::CreateFromQuaternion(_quat);
		}
		// 移動
		if (a_tinyModel.nodes[_nodeIdx].translation.size() != 0)
		{
			_tMat = DXSM::Matrix::CreateTranslation(
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].translation[0]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].translation[1]),
				static_cast<float>(a_tinyModel.nodes[_nodeIdx].translation[2])
			);
		}
		// 行列
		if (a_tinyModel.nodes[_nodeIdx].matrix.size() != 0)
		{
			for (int _n = 0; _n < 16; ++_n)
			{
				*(&_sMat._11 + _n) = (float)a_tinyModel.nodes[_nodeIdx].matrix[_n];
			}
		}

		// 変換行列を格納 : 座標系の変換は ModelProcessor が行う
		_destNode->localTransform = _sMat * _rMat * _tMat;
	}

	return _result;
}

std::vector<int> ParseRootNodes(const tinygltf::Model& a_tinyModel)
{
	std::vector<int> _result = {};
	//----------------------------------------------------
	// ノードノードのみの参照リスト
	//----------------------------------------------------
	for (auto&& _idx : a_tinyModel.scenes[0].nodes)
	{
		_result.push_back(_idx);
	}
	return _result;
}

std::vector<int> ParseBoneIndices(const tinygltf::Model& a_tinyModel, std::vector<Engine::Resource::Parse::RawNode>& a_nodes)
{
	std::vector<int> _result = {};

	//----------------------------------------------------
	// ボーン
	//----------------------------------------------------
	if (a_tinyModel.skins.size() > 0)
	{
		// 配列確保
		_result = a_tinyModel.skins[0].joints;

		// InverseBindMarices(オフセット行列)取得用
		GLTFBufferGetter _ibmGetter(&a_tinyModel, a_tinyModel.skins[0].inverseBindMatrices);

		// ボーンだけのノード参照配列
		for (UINT _jointIdx = 0; _jointIdx < a_tinyModel.skins[0].joints.size(); ++_jointIdx)
		{
			// _jointIdx番目のボーンの、ノード内でのIndex
			int _originNodeIdx = a_tinyModel.skins[0].joints[_jointIdx];

			Engine::Resource::Parse::RawNode* _boneNode = &a_nodes[_originNodeIdx];
			_boneNode->boneNodeIndex = _jointIdx;

			// オフセット行列取得 : 座標系の変換は ModelProcessor が行う
			DXSM::Matrix _invBindMat = DXSM::Matrix::Identity;
			for (int _matIdx = 0; _matIdx < 16; ++_matIdx)
			{
				(&_invBindMat._11)[_matIdx] = _ibmGetter.GetValue_Float(_jointIdx * 16 + _matIdx);
			}

			_boneNode->inverseBindMatrix = _invBindMat;

			// 変換行列へ変換
			_boneNode->worldTransform = _invBindMat.Invert();
		}

		// ボーンLocalMat算出
		for (int _nodeIdx : _result)
		{
			Engine::Resource::Parse::RawNode* _boneNode = &a_nodes[_nodeIdx];

			if (_boneNode->parent >= 0)
			{
				DXSM::Matrix _boneWorldMat(_boneNode->worldTransform);
				DXSM::Matrix _invBindMat(a_nodes[_boneNode->parent].inverseBindMatrix);
				_boneNode->localTransform = _boneWorldMat * _invBindMat;
			}
			else
			{
				_boneNode->localTransform = _boneNode->worldTransform;
			}
		}
	}

	return _result;
}

void CalcWorldTransform(Engine::Resource::Parse::RawModel& a_model)
{
	//----------------------------------------------------
	// 各ノードのTransformからWorldTransformを算出
	//----------------------------------------------------

	// 行列計算用再起関数
	std::function<void(Engine::Resource::Parse::RawNode*, const DirectX::XMFLOAT4X4*)> _rec =
		[&_rec, &a_model](Engine::Resource::Parse::RawNode* a_node, const DirectX::XMFLOAT4X4* a_parentMat)
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
				_rec(&a_model.nodes[_child], &a_node->worldTransform);
			}
		};

	// 親子関係から行列を作成
	for (int _nodeIdx : a_model.rootNodeIndices)
	{
		_rec(&a_model.nodes[_nodeIdx], nullptr);
	}
}

void ParseMesh(tinygltf::Model& a_model, Engine::Resource::Parse::RawModel& a_dest)
{
	//----------------------------------------------------
	// メッシュ
	//----------------------------------------------------
	for (UINT _nodeIdx = 0; _nodeIdx < a_dest.nodes.size(); ++_nodeIdx)
	{
		auto* _destNode = &a_dest.nodes[_nodeIdx]; // コピー先確保

		//-----------------------
		// メッシュの場合
		//-----------------------
		int _meshIdx = a_model.nodes[_nodeIdx].mesh;     // メッシュIndex取得
		if (_meshIdx < 0)continue;                          // メッシュなし

		// このノードのメッシュ
		Engine::Resource::Parse::RawMesh _destMesh = {};

		// 作業データ
		struct GLTFPrimitive
		{
			std::vector<Engine::Resource::MeshVertex8bit> vertices = {};
			std::vector<Engine::Resource::MeshFace>       faces = {};

			UINT                        materialNumber = 0;

			std::map<std::string, int>  attributes = {};
		};
		std::vector<std::shared_ptr<GLTFPrimitive>> _tmpPrimitives;
		_tmpPrimitives.reserve(a_model.meshes[_meshIdx].primitives.size());

		//-----------------------
		// 全プリミティブ
		//-----------------------
		for (size_t _primitiveIdx = 0; _primitiveIdx < a_model.meshes[_meshIdx].primitives.size(); ++_primitiveIdx)
		{
			// コピー元準備
			auto& _srcPrimitive = a_model.meshes[_meshIdx].primitives[_primitiveIdx];

			// TRIANGLES以外は無視（メッシュの基本図形はこれだけで十分だから）
			if (_srcPrimitive.mode != TINYGLTF_MODE_TRIANGLES)continue;

			// 作成
			std::shared_ptr<GLTFPrimitive> _destPrimitive = std::make_shared<GLTFPrimitive>();
			_tmpPrimitives.push_back(_destPrimitive);
			_destPrimitive->attributes = _srcPrimitive.attributes;

			// マテリアルナンバー
			_destPrimitive->materialNumber = std::max(0, _srcPrimitive.material);


			//-----------------------
			// 頂点バッファ
			//-----------------------

			// 座標
			{
				// 座標ゲッター生成
				GLTFBufferGetter _posGetter(&a_model, _srcPrimitive.attributes["POSITION"]);

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
					_ver.pos.z = _posGetter.GetValue_Float(_vertexIdx * 3 + 2);
				}
			}

			// 法線
			if (_srcPrimitive.attributes.count("NORMAL") > 0)
			{
				// 法線ゲッター生成
				GLTFBufferGetter _normalGetter(&a_model, _srcPrimitive.attributes["NORMAL"]);
				for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
				{
					// コピー先設定
					auto& _nor = _destPrimitive->vertices[_vertexIdx].normal;
					// コピー
					_nor.x = _normalGetter.GetValue_Float(_vertexIdx * 3 + 0);
					_nor.y = _normalGetter.GetValue_Float(_vertexIdx * 3 + 1);
					_nor.z = _normalGetter.GetValue_Float(_vertexIdx * 3 + 2);
				}
			}

			// UV
			if (_srcPrimitive.attributes.count("TEXCOORD_0") > 0)
			{
				// UVゲッター生成
				GLTFBufferGetter _uvGetter(&a_model, _srcPrimitive.attributes["TEXCOORD_0"]);
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
				GLTFBufferGetter _colorGetter(&a_model, _srcPrimitive.attributes["COLOR_0"]);

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
			if (a_model.skins.size() > 0)
			{
				// スキンIndex
				if (_srcPrimitive.attributes.count("JOINTS_0") > 0)
				{
					_destMesh.isSkinMesh = true;      // スキンメッシュ持ち
					GLTFBufferGetter _jointGetter(&a_model, _srcPrimitive.attributes["JOINTS_0"]);
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
					_destMesh.isSkinMesh = true;      // スキンメッシュ持ち
					GLTFBufferGetter _weightGetter(&a_model, _srcPrimitive.attributes["WEIGHTS_0"]);
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
			GLTFBufferGetter _indexGetter(&a_model, _srcPrimitive.indices);      // ゲッター生成
			_destPrimitive->faces.resize(_indexGetter.GetAccsessor()->count / 3);   // 面の数分配列を確保
			for (UINT _faceIdx = 0; _faceIdx < _destPrimitive->faces.size(); ++_faceIdx)
			{
				// 巻き順は読み込んだまま。反転は ModelProcessor が行う
				_destPrimitive->faces[_faceIdx].idx[0] =
					static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 0));
				_destPrimitive->faces[_faceIdx].idx[1] =
					static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 1));
				_destPrimitive->faces[_faceIdx].idx[2] =
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
			[](const std::shared_ptr<GLTFPrimitive>& a_v1, const std::shared_ptr<GLTFPrimitive>& a_v2)
			{
				return a_v1->materialNumber < a_v2->materialNumber;
			}
		);

		// マテリアルの最大数分サブセット作成
		_destMesh.subsets.resize(_tmpPrimitives.size());
		for (UINT _priIdx = 0; _priIdx < _tmpPrimitives.size(); ++_priIdx)
		{
			// マテリアル番号
			_destMesh.subsets[_priIdx].materialNumber = _tmpPrimitives[_priIdx]->materialNumber;
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
				UINT _st = static_cast<UINT>(_destMesh.vertices.size());
				_destMesh.vertices.resize(_destMesh.vertices.size() + _primitive->vertices.size());
				memcpy(&_destMesh.vertices[_st],
					&_primitive->vertices[0], _primitive->vertices.size() * sizeof(Engine::Resource::MeshVertex8bit)
				);
			}

			// インデックス合成
			if (_primitive->faces.size() >= 1)
			{
				UINT _st = static_cast<UINT>(_destMesh.faces.size());
				_destMesh.faces.resize(_destMesh.faces.size() + _primitive->faces.size());
				for (UINT _faceIdx = 0; _faceIdx < _primitive->faces.size(); ++_faceIdx)
				{
					_destMesh.faces[_st + _faceIdx].idx[0] = _primitive->faces[_faceIdx].idx[0] + _currentVertexIdx;
					_destMesh.faces[_st + _faceIdx].idx[1] = _primitive->faces[_faceIdx].idx[1] + _currentVertexIdx;
					_destMesh.faces[_st + _faceIdx].idx[2] = _primitive->faces[_faceIdx].idx[2] + _currentVertexIdx;
				}
			}

			// サブセット
			_destMesh.subsets[_priIdx].faceCount += static_cast<UINT>(_primitive->faces.size());		// 面数を加算

			// 頂点数・面数を次の開始位置に設定
			_currentVertexIdx += static_cast<UINT>(_primitive->vertices.size());
			_currentFaceIdx += static_cast<UINT>(_primitive->faces.size());
		}

		// このプリミティブの作業を終了
		_tmpPrimitives.clear();

		// サブセットのオフセットをもとめる
		UINT _offset = 0;
		for (UINT _priIdx = 0; _priIdx < _destMesh.subsets.size(); ++_priIdx)
		{
			_destMesh.subsets[_priIdx].faceStart = _offset;		// 面描画の開始Index
			_offset += _destMesh.subsets[_priIdx].faceCount;		// 次のプリミティブの開始位置設定
		}

		// メッシュ配列へ登録し、ノードから参照させる
		_destNode->meshIndex = static_cast<int>(a_dest.meshes.size());
		a_dest.meshes.push_back(std::move(_destMesh));
	}
}

std::vector<Engine::Resource::Parse::RawAnimation> ParseAnimation(tinygltf::Model& a_model, size_t a_nodeCount)
{
	std::vector<Engine::Resource::Parse::RawAnimation> _result = {};

	//----------------------------------------------------
	// アニメーション
	//----------------------------------------------------
	for (UINT _animaIdx = 0; _animaIdx < a_model.animations.size(); ++_animaIdx)
	{
		// 参照元用意
		const auto& _srcAnima = a_model.animations[_animaIdx];

		// アニメーションデータに追加
		Engine::Resource::Parse::RawAnimation _dstAnimation = {};

		// 名前
		_dstAnimation.name = _srcAnima.name;

		// アニメーションノード : 一時的な作業データ準備(nodeOffsetが-1のものは未使用)
		std::vector<Engine::Resource::Parse::RawAnimationNode> _tmpNodes;
		_tmpNodes.resize(a_nodeCount);

		// 全チャンネル
		for (const auto& _channel : _srcAnima.channels)
		{
			// 参照元用意
			const auto& _sampler = _srcAnima.samplers[_channel.sampler];

			// 対象ノードのIndex
			auto& _destAnimaNode = _tmpNodes[_channel.target_node];
			_destAnimaNode.nodeOffset = _channel.target_node;

			GLTFBufferGetter _timeGetter(&a_model, _sampler.input);		// 時間アクセサ
			GLTFBufferGetter _valueGetter(&a_model, _sampler.output);	// データアクセサ

			// 座標のアニメーションノード設定
			if (_channel.target_path == "translation")
			{
				for (UINT _kIdx = 0; _kIdx < _timeGetter.GetAccsessor()->count; ++_kIdx)
				{
					Engine::Resource::AnimationKeyXMFLOAT3 _vec;

					// 時間
					_vec.time = _timeGetter.GetValue_Float(_kIdx) * 60.0f;		// 元が60fpsとして変換
					if (_vec.time > _dstAnimation.maxLength)
					{
						_dstAnimation.maxLength = _vec.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode.translations.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode.translations.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 9 + 3);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 9 + 4);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 9 + 5);
						_destAnimaNode.translations.push_back(_vec);						// 保存
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
					if (_vec.time > _dstAnimation.maxLength)
					{
						_dstAnimation.maxLength = _vec.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode.scales.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 3 + 0);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 3 + 1);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 3 + 2);
						_destAnimaNode.scales.push_back(_vec);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_vec.vec.x = _valueGetter.GetValue_Float(_kIdx * 9 + 3);
						_vec.vec.y = _valueGetter.GetValue_Float(_kIdx * 9 + 4);
						_vec.vec.z = _valueGetter.GetValue_Float(_kIdx * 9 + 5);
						_destAnimaNode.scales.push_back(_vec);						// 保存
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
					if (_quat.time > _dstAnimation.maxLength)
					{
						_dstAnimation.maxLength = _quat.time;
					}

					// 値
					if (_sampler.interpolation == "STEP")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 4 + 0);
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 4 + 1);
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 4 + 2);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 4 + 3);
						_destAnimaNode.rotations.push_back(_quat);						// 保存
					}
					else if (_sampler.interpolation == "LINEAR")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 4 + 0);
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 4 + 1);
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 4 + 2);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 4 + 3);
						_destAnimaNode.rotations.push_back(_quat);						// 保存
					}
					else if (_sampler.interpolation == "CUBICSPLINE")
					{
						_quat.quat.x = _valueGetter.GetValue_Float(_kIdx * 12 + 4);
						_quat.quat.y = _valueGetter.GetValue_Float(_kIdx * 12 + 5);
						_quat.quat.z = _valueGetter.GetValue_Float(_kIdx * 12 + 6);
						_quat.quat.w = _valueGetter.GetValue_Float(_kIdx * 12 + 7);
						_destAnimaNode.rotations.push_back(_quat);						// 保存
					}
				}
			}
		}

		// アニメーションで使用していない不必要なノードを除外したリストを作成
		for (auto&& _n : _tmpNodes)
		{
			if (_n.nodeOffset < 0)continue;
			_dstAnimation.animationNodes.push_back(std::move(_n));
		}

		_result.push_back(std::move(_dstAnimation));
	}

	return _result;
}

Engine::Resource::Parse::RawModel Engine::Resource::GLTF::Load(std::string_view a_filePath)
{
	//===============================================
	// モデルデータの読み込み
	//===============================================
	tinygltf::Model _tinyModel = tinyLoadFromFile(a_filePath);

	// 戻り値用データを準備
	Engine::Resource::Parse::RawModel _destModel = {};

	_destModel.materials = ParseMaterial(_tinyModel);								// マテリアルパース
	_destModel.nodes = ParseNode(_tinyModel);										// ノードパース
	_destModel.rootNodeIndices = ParseRootNodes(_tinyModel);						// ルートノード
	CalcWorldTransform(_destModel);													// 各ノードのTransformからWorldTransformを算出
	_destModel.boneNodeIndices = ParseBoneIndices(_tinyModel, _destModel.nodes);		// ボーン
	ParseMesh(_tinyModel, _destModel);												// メッシュ、マテリアル、プリミティブ
	_destModel.animations = ParseAnimation(_tinyModel, _destModel.nodes.size());		// アニメーション

	// 読み込んだままの中間素材を返す
	return _destModel;
}
