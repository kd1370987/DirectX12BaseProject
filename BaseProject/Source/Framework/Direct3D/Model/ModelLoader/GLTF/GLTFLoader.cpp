#include "GLTFLoader.h"

#include "../../ModelScene/Animation/Animation.h"
#include "../../ModelScene/Mesh/Mesh.h"

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
        if (m_pAccessor->componentType == TINYGLTF_PARAMETER_TYPE_BYTE)                 return Get<char>(a_idx) / static_cast<float>(SCHAR_MAX);
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
    const tinygltf::Accessor*   GetAccsessor()  const { return m_pAccessor; }
    const tinygltf::BufferView* GetBufferView() const { return m_pBufferView; }
    const tinygltf::Buffer*     GetBuffer()     const { return m_pBuffer; }

private:
    // 型指定型でIndex番目のデータを取得
    template<class Type>
    const Type& Get(int a_index)
    {
        return *(const Type*)&m_pAddress[a_index * sizeof(Type)];
    }

    const BYTE*                 m_pAddress = nullptr;        // データの先頭アドレス

    const tinygltf::Model*      m_pModel = nullptr;         // モデル
    const tinygltf::Accessor*   m_pAccessor = nullptr;      // BufferViewのデータ型や要素数、正規化情報を持つ
    const tinygltf::BufferView* m_pBufferView = nullptr;    // Bufferの一部範囲を示す
    const tinygltf::Buffer*     m_pBuffer = nullptr;        // 実際のバイナリデータ（頂点座標とか法線、UVなど）
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

std::shared_ptr<GLTFModel> GLTFLoader::LoadGLTFModel(std::string_view a_filePath)
{
    //===============================================
    //
    // モデルデータの読み込み
    //
    //===============================================
    tinygltf::Model _tinyModel;                     // モデルデータの入れもの
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
        printf("Warn : %s\n", _warn.c_str());
    }

    if (!_err.empty())
    {
        printf("Err : %s\n", _err.c_str());
    }

    if (!_ret)
    {
        printf("Failed to parse gltf\n");
        return nullptr;
    }

    // 戻り値用データを準備
    std::shared_ptr<GLTFModel> _destModel = std::make_shared<GLTFModel>();

    //----------------------------------------------------
    // マテリアル
    //----------------------------------------------------
    // 指定Indexのテクスチャ名取得
    auto _getTextureFilename = [&_tinyModel](int a_texIndex) -> std::string
    {
        if (a_texIndex < 0) return "";
        int _imgIndex = _tinyModel.textures[a_texIndex].source;
        if (_imgIndex < 0) return "";

        const auto& _img = _tinyModel.images[_imgIndex];

        if (!_img.uri.empty())
        {
            // 通常のGLTFファイル
            return _img.uri;
        }
        else if (!_img.image.empty())
        {
            // GLBの場合
            // ファイル名がないので一時的な仮名を生成
            // 実際の読み込みは子のバイナリデータを使って行う
            return "[GLB_EMBEDDED_IMAGE_" + std::to_string(_imgIndex) + "]";
        }

        return "";
    };

    // テクスチャの埋め込みチェック
    auto _checkEmbedded = [](const std::string& a_name) -> bool
    {
        return a_name.starts_with("[GLB_EMBEDDED_IMAGE_");
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
        _destMaterial.alphaMode = _srcMaterial.alphaMode;                           // モード
        _destMaterial.AlphaCutoff = static_cast<float>(_srcMaterial.alphaCutoff);   // 閾値
        _destMaterial.doubleSided = _srcMaterial.doubleSided;                       // 設定面

        // 基本色
        _destMaterial.baseColorTexName = _getTextureFilename(_srcMaterial.pbrMetallicRoughness.baseColorTexture.index); // 名前
        _destMaterial.isEmbeddedBaseColorTex = _checkEmbedded(_destMaterial.baseColorTexName);                          // 外部依存かどうか
        if(_srcMaterial.pbrMetallicRoughness.baseColorFactor.size() == 4)
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
        _destMaterial.metallicRoughnessTexName = _getTextureFilename(_srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
        _destMaterial.isEmbeddedMetallicRoughnessTex = _checkEmbedded(_destMaterial.metallicRoughnessTexName);
        _destMaterial.metallicFactor = static_cast<float>(_srcMaterial.pbrMetallicRoughness.metallicFactor);
        _destMaterial.roughnessFactor = static_cast<float>(_srcMaterial.pbrMetallicRoughness.roughnessFactor);

        // エミッシブ
        _destMaterial.emissiveTexName = _getTextureFilename(_srcMaterial.emissiveTexture.index);
        _destMaterial.isEmbeddedEmissiveTex = _checkEmbedded(_destMaterial.emissiveTexName);
        if (_srcMaterial.emissiveFactor.size() == 3)
        {
            _destMaterial.emissiveFactor = {
                static_cast<float>(_srcMaterial.emissiveFactor[0]),
                static_cast<float>(_srcMaterial.emissiveFactor[1]),
                static_cast<float>(_srcMaterial.emissiveFactor[2])
            };
        }

        // 法線マップ
        _destMaterial.normalTexName = _getTextureFilename(_srcMaterial.normalTexture.index);
        _destMaterial.isEmbeddedNormalTex = _checkEmbedded(_destMaterial.normalTexName);

        // オクルージョンマップ
        _destMaterial.occlusionTexName = _getTextureFilename(_srcMaterial.occlusionTexture.index);
        _destMaterial.isEmbeddedOcclusionTex = _checkEmbedded(_destMaterial.occlusionTexName);
    }

    // マテリアルがゼロの場合は、１つだけ作成しておく
    if (_destModel->materials.size() == 0)
    {
        _destModel->materials.resize(1);
    }

    //----------------------------------------------------
    // ノード
    //----------------------------------------------------
    
    _destModel->nodes.resize(_tinyModel.nodes.size());      // 全ノード分メモリを確保

    // 全ノード　基本情報設定
    for (UINT _nodeIdx = 0; _nodeIdx < _destModel->nodes.size(); ++_nodeIdx)
    {
        auto* _destNode = &_destModel->nodes[_nodeIdx];     // コピー先

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
        DirectX::XMMATRIX _scaleMat, _rotationMat, _transMat;
        DirectX::XMFLOAT4X4 _mat;
        // 拡縮
        if (_tinyModel.nodes[_nodeIdx].scale.size() != 0)
        {
            _scaleMat = DirectX::XMMatrixScaling(
                static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[0]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[1]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].scale[2])
            );
        }
        // 回転
        if (_tinyModel.nodes[_nodeIdx].rotation.size() != 0)
        {
            DirectX::XMFLOAT4 _quatFloat4(
                static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[0]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[1]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[2]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].rotation[3])
            );

            // 変換して計算
            DirectX::XMVECTOR _quat = DirectX::XMLoadFloat4(&_quatFloat4);
            _rotationMat = DirectX::XMMatrixRotationQuaternion(_quat);
        }
        // 移動
        if (_tinyModel.nodes[_nodeIdx].translation.size() != 0)
        {
            _transMat = DirectX::XMMatrixTranslation(
                static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[0]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[1]),
                static_cast<float>(_tinyModel.nodes[_nodeIdx].translation[2])
            );
        }
        // 行列
        if (_tinyModel.nodes[_nodeIdx].matrix.size() != 0)
        {
            for (int _i = 0; _i < 16; ++_i)
            {
                ((float*)&_mat)[_i] = static_cast<float>(_tinyModel.nodes[_nodeIdx].matrix[_i]);
            }
            _scaleMat = DirectX::XMLoadFloat4x4(&_mat);
        }

        // 行列変換
        DirectX::XMMATRIX _localTrans = _scaleMat * _rotationMat * _transMat;
        DirectX::XMStoreFloat4x4(&_destNode->localTransform, _localTrans);
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
    // 行列計算用再起関数
    std::function<void(GLTFNode*, const DirectX::XMFLOAT4X4*)> _rec =
    [&_rec, &_destModel](GLTFNode* a_node, const DirectX::XMFLOAT4X4* a_parentMat)
    {
        // ワールド行列をもとめる
        if (a_parentMat)
        {
            // 計算するために XMFLOAT4x4 から XMMATRIX に変換
            DirectX::XMMATRIX _localMat = DirectX::XMLoadFloat4x4(&a_node->localTransform);
            DirectX::XMMATRIX _parentMat = DirectX::XMLoadFloat4x4(a_parentMat);
            DirectX::XMMATRIX _worldMat = _localMat * _parentMat;
            DirectX::XMStoreFloat4x4(&a_node->worldTransform, _worldMat);
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

            GLTFNode* _boneNode = &_destModel->nodes[_originNodeIdx];
            _boneNode->boneNodeIndex = _jointIdx;

            // オフセット行列取得
            DirectX::XMFLOAT4X4 _invBindMat;
            for (int _matIdx = 0; _matIdx < 16; ++_matIdx)
            {
                (&_invBindMat._11)[_matIdx] = _ibmGetter.GetValue_Float(_jointIdx * 16 + _matIdx);
            }
            // Z軸ミラーリング
            XMFLOAT4X4MirrorZ(_invBindMat);

            // 格納
            _boneNode->inverseBindMatrix = _invBindMat;

            // 変換行列へ変換
            const DirectX::XMMATRIX _mat = DirectX::XMLoadFloat4x4(&_invBindMat);
            DirectX::XMVECTOR _det;
            DirectX::XMMATRIX _inv = DirectX::XMMatrixInverse(&_det, _mat);
            DirectX::XMStoreFloat4x4(&_boneNode->worldTransform, _inv);
        }

        // ボーンLocalMat算出
        for (int _nodeIdx : _destModel->boneNodeIndices)
        {
            GLTFNode* _boneNode = &_destModel->nodes[_nodeIdx];

            if (_boneNode->parent >= 0)
            {
                DirectX::XMMATRIX _worldMat = DirectX::XMLoadFloat4x4(&_boneNode->worldTransform);
                DirectX::XMMATRIX _invBindMat = DirectX::XMLoadFloat4x4(&_destModel->nodes[_boneNode->parent].inverseBindMatrix);
                DirectX::XMMATRIX _mat = _worldMat * _invBindMat;
                DirectX::XMStoreFloat4x4(&_boneNode->localTransform, _mat);
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
            std::vector<MeshVertex>     vertices;
            std::vector<MeshFace>       faces;

            UINT                        materialNumber = 0;

            std::map<std::string, int>  attributes;
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

                DirectX::XMFLOAT3 _pos;
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
                GLTFBufferGetter _uvGetter(&_tinyModel,_srcPrimitive.attributes["TEXCOORD_0"]);
                for (UINT _vertexIdx = 0; _vertexIdx < _destPrimitive->vertices.size(); ++_vertexIdx)
                {
                    // コピー先設定
                    auto& _uv = _destPrimitive->vertices[_vertexIdx].uv;
                    // コピー
                    _uv.x = _uvGetter.GetValue_Float(_vertexIdx * 2 + 0);
                    _uv.y = _uvGetter.GetValue_Float(_vertexIdx * 2 + 1);
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
                    GLTFBufferGetter _jointGetter(&_tinyModel,_srcPrimitive.attributes["JOINTS_0"]);
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
                _destPrimitive->faces[_faceIdx].idx[0] = static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 0));
                _destPrimitive->faces[_faceIdx].idx[1] = static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 1));
                _destPrimitive->faces[_faceIdx].idx[2] = static_cast<UINT>(_indexGetter.GetValue_Int(_faceIdx * 3 + 2));
            }
        }
    }
}
