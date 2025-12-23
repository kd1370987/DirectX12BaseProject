#include "SceneManager.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/PSOManager/PSOManager.h"

#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/GPUResource/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/GPUResource/Buffer/VertexBuffer/VertexBuffer.h"

#include "Engine/GPUResource/Model/ModelLoader/Assimp/AssimpLoader.h"
#include "Engine/GPUResource/Texture/Texture2D/Texture2D.h"
#include "Engine/GPUResource/Texture/Texture.h"

#include "Engine/GPUResource/Model/Model.h"
#include "Engine/GPUResource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GPUResource/Model/ModelResource/Node/Node.h"
#include "Engine/GPUResource/Model/ModelResource/Material/Material.h"

struct alignas(256) trans
{
	DirectX::XMFLOAT4X4 mat;
};

// 定数バッファ(オブジェクト単位での更新)
struct alignas(256) CBObject
{
	// UV操作
	DirectX::XMFLOAT2 uvOffset = { 0.0f,0.0f };
	DirectX::XMFLOAT2 uvTiling = { 1.0f,1.0f };
};

// マテリアル単位更新用定数バッファ
struct alignas(256) CBMaterial
{
	DirectX::XMFLOAT4 baseColorXYZW = { 1.0f,1.0f,1.0f,1.0f };

	DirectX::XMFLOAT4 emissiveXYZ = { 1.0f,1.0f,1.0f,0.0f };

	DirectX::XMFLOAT4 metalicRoughnessXY = { 0.0f,1.0f,0.0f,0.0f };
};

using namespace DirectX;

ConstantBuffer* g_objBff[FRAME_BUFFER_COUNT];
ConstantBuffer* g_matBff[FRAME_BUFFER_COUNT];
ConstantBuffer* g_materialBff[FRAME_BUFFER_COUNT];

//const wchar_t* MODEL_FILE_PATH = L"Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX";
std::string MODEL_FILE_PATH = "Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX";
//std::vector<AssimpMesh> g_meshes;
AssimpModel g_assimpModel;

#include "Engine/GPUResource/Model/Model.h"

bool SceneManager::Init()
{
	m_spModel = std::make_shared<ModelResource>();
	if (!m_spModel->Load("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX"))
	{
		assert(0 && "FBXモデル読み込みに失敗\n");
		return false;
	}

	AssimpLoader _loader;
	//if(!_loader.Load(
	//	MODEL_FILE_PATH,
	//	g_meshes,
	//	false,
	//	true
	//))
	//{
	//	assert(0 && "モデル読み込みに失敗\n");
	//	return false;
	//}
	if(!_loader.Load(
		MODEL_FILE_PATH,
		g_assimpModel,
		false,
		true
	))
	{
		assert(0 && "モデル読み込みに失敗\n");
		return false;
	}

	// カメラ座標設定
	auto _eyePos = XMVectorSet(0.0f, 120.0f, 100.0f, 0.0f);
	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	/*for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		g_matBff[_i] = new ConstantBuffer(RenderingEngine::Instance().GetDevice());
		if (!g_matBff[_i]->Create(sizeof(trans)))
		{
			assert(0 && "マテリアル用定数バッファの作成に失敗\n");
			return false;
		}
		
		auto _ptrMat = g_matBff[_i]->GetPtr <trans>();
		_ptrMat->mat = DirectX::XMFLOAT4X4
		{
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
		};
		
		g_objBff[_i] = new ConstantBuffer(RenderingEngine::Instance().GetDevice());
		if (!g_objBff[_i]->Create(sizeof(CBObject)))
		{
			assert(0 && "オブジェクト用定数バッファの作成に失敗\n");
			return false;
		}
	
		g_objBff[_i]->GetPtr<CBObject>()->uvOffset = { 1.0f,1.0f };
		g_objBff[_i]->GetPtr<CBObject>()->uvTiling = { 1.0f,1.0f };

		g_materialBff[_i] = new ConstantBuffer(RenderingEngine::Instance().GetDevice());
		if (!g_materialBff[_i]->Create(sizeof(CBMaterial)))
		{
			assert(0 && "マテリアル用定数バッファの生成に失敗\n");
			return false;
		}
		g_materialBff[_i]->GetPtr<CBMaterial>()->baseColorXYZW = { 0.0f,1.0f,0.0f,1.0f };
		g_materialBff[_i]->GetPtr<CBMaterial>()->emissiveXYZ = { 1.0f,1.0f,1.0f ,0.0f};
		g_materialBff[_i]->GetPtr<CBMaterial>()->metalicRoughnessXY = {0.0f,1.0f,0.0f,0.0f};
	}*/
	return true;
}

bool SceneManager::Release()
{
	return true;
}

void SceneManager::Update()
{
	//m_rotateY += 0.05f;
	//// 現在のフレーム番号
	//auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();
	//// 現在のフレーム番号に対応する定数バッファを取得
	//auto _currentMat = g_matBff[_currentIdx]->GetPtr <trans>();

	//DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(m_rotateY);
	//DirectX::XMStoreFloat4x4(&_currentMat->mat, rotY);
}

void SceneManager::Draw()
{
	
	RenderContext::Instance().BeginSimpleRender();

	auto _commandList = RenderingEngine::Instance().GetCommandList();				// コマンドリスト
	
	RenderContext::Instance().SetToShader(m_cameraMat);

	RenderContext::Instance().DrawModel(
		m_spModel,
		DirectX::XMMatrixIdentity(),
		DirectX::XMFLOAT4{1.0f,1.0f,1.0f,1.0f},
		DirectX::XMFLOAT3{1.0f,1.0f,1.0f}
	);

	//auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();			// 現在のフレーム番号
	//
	//// マテリアル用ディスクリプタヒープ
	//_commandList->SetGraphicsRootDescriptorTable(1, g_objBff[_currentIdx]->GetHandle());

	//// メッシュの数だけインデックス分の描画を行う処理を回す
	////for (size_t _i = 0; _i < g_assimpModel.nodes.size(); ++_i)
	//for (size_t _i = 0; _i < m_spModel->GetOriginalNodes().size(); ++_i)
	//{
	//	// 頂点バッファ・インデックスバッファをセット
	//	//auto _vbView = g_assimpModel.nodes[_i].spMesh->vertexBuffer->View();
	//	auto _vbView = m_spModel->GetOriginalNodes()[_i].spMesh->GetVertexBuffer().View();
	//	//auto _ibView = g_assimpModel.nodes[_i].spMesh->indexBuffer->View();
	//	auto _ibView = m_spModel->GetOriginalNodes()[_i].spMesh->GetIndexBuffer().View();
	//	_commandList->IASetVertexBuffers(0, 1, &_vbView);
	//	_commandList->IASetIndexBuffer(&_ibView);

	//	_commandList->SetGraphicsRootDescriptorTable(2, g_matBff[_currentIdx]->GetHandle());
	//	_commandList->SetGraphicsRootDescriptorTable(3, g_materialBff[_currentIdx]->GetHandle());

	//	// マテリアルテクスチャをセット
	//	/*_commandList->SetGraphicsRootDescriptorTable(
	//		4, 
	//		g_assimpModel.nodes[_i].spMesh->materialHandle->handleGPU
	//	);*/
	//	m_spModel->GetOriginalNodes()[_i].spMesh->GetSubsets();
	//	const Material& _material = m_spModel->GetMaterials()[m_spModel->GetOriginalNodes()[_i].spMesh->GetSubsets()[0].materialNumber];
	//	_commandList->SetGraphicsRootDescriptorTable(
	//		4, 
	//		ResourceManager::Instance().GetTexture(_material.baseColorTexKey).lock()->GetGpuSrvHandle()
	//	);

	//	// インデックスの数分描画
	//	/*_commandList->DrawIndexedInstanced(static_cast<UINT>(g_assimpModel.nodes[_i].spMesh->indices.size()), 1, 0, 0, 0);*/

	//	_commandList->DrawIndexedInstanced(
	//		static_cast<UINT>(m_spModel->GetOriginalNodes()[_i].spMesh->GetSubsets()[0].faceCount * 3)
	//		, 1, 0, 0, 0
	//	);
	//}
}
