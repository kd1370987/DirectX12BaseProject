#include "SceneManager.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/PSOManager/PSOManager.h"


#include "Framework/Shader/ShaderCommon/SharedStruct.h"

#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/GPUResource/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/GPUResource/Buffer/VertexBuffer/VertexBuffer.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"

#include "Engine/GPUResource/DescriptorHeap/SRVHeap/SRVHeap.h"
#include "Engine/GPUResource/DescriptorHeap/CBVHeap/CBVHeap.h"

#include "Engine/GPUResource/Model/ModelLoader/Assimp/AssimpLoader.h"
#include "Engine/GPUResource/Texture/Texture2D/Texture2D.h"

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
	DirectX::XMFLOAT4 baseColor = { 1.0f,1.0f,1.0f,1.0f };

	DirectX::XMFLOAT3 emissive = { 1.0f,1.0f,1.0f };
	float metallic = 0.0f;

	float roughness = 1.0f;
	float pad[3] = { 0.f,0.f,0.f };
};

using namespace DirectX;

ConstantBuffer* g_objBff[FRAME_BUFFER_COUNT];
ConstantBuffer* g_matBff[FRAME_BUFFER_COUNT];
ConstantBuffer* g_materialBff[FRAME_BUFFER_COUNT];

const wchar_t* MODEL_FILE_PATH = L"Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX";
std::vector<AssimpMesh> g_meshes;

//#include "Engine/GPUResource/Model/Model.h"

bool SceneManager::Init()
{

	ImportSettings _importSettings = 
	{
		MODEL_FILE_PATH,
		g_meshes,
		false,
		true
	};

	//m_spModel = std::make_shared<ModelResource>();
	/*if (!m_spModel->Load("Asset/Model/tank/tank.gltf"))
	{
		assert(0 && "GLTFモデル読み込みに失敗\n");
		return false;
	}*/

	AssimpLoader _loader;
	if (!_loader.Load(_importSettings))
	{
		assert(0 && "Assimpモデル読み込みに失敗\n");
		return false;
	}

	

	

	// カメラ座標設定
	auto _eyePos = XMVectorSet(0.0f, 120.0f, 100.0f, 0.0f);
	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		g_matBff[_i] = new ConstantBuffer(sizeof(trans));
		if (!g_matBff[_i]->IsValid())
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
		g_matBff[_i]->UnMap();
		
		g_objBff[_i] = new ConstantBuffer(sizeof(CBObject));
		if (!g_objBff[_i]->IsValid())
		{
			assert(0 && "オブジェクト用定数バッファの作成に失敗\n");
			return false;
		}
		g_objBff[_i]->GetPtr<CBObject>()->uvOffset = { 1.0f,1.0f };
		g_objBff[_i]->GetPtr<CBObject>()->uvTiling = { 1.0f,1.0f };
		g_objBff[_i]->UnMap();

		g_materialBff[_i] = new ConstantBuffer(sizeof(CBMaterial));
		if (!g_materialBff[_i]->IsValid())
		{
			assert(0 && "マテリアル用定数バッファの生成に失敗\n");
			return false;
		}
		g_materialBff[_i]->GetPtr<CBMaterial>()->baseColor = { 0.0f,1.0f,0.0f,1.0f };
		g_materialBff[_i]->GetPtr<CBMaterial>()->emissive = { 1.0f,1.0f,1.0f };
		g_materialBff[_i]->GetPtr<CBMaterial>()->metallic = 0.0f;
		g_materialBff[_i]->GetPtr<CBMaterial>()->roughness = 1.0f;
		g_materialBff[_i]->UnMap();

	}
	return true;
}

bool SceneManager::Release()
{
	return true;
}

void SceneManager::Update()
{
	m_rotateY += 0.05f;
	// 現在のフレーム番号
	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();
	// 現在のフレーム番号に対応する定数バッファを取得
	auto _currentMat = g_matBff[_currentIdx]->GetPtr <trans>();

	DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(m_rotateY);
	DirectX::XMStoreFloat4x4(&_currentMat->mat, rotY);
	g_matBff[_currentIdx]->UnMap();
}

void SceneManager::Draw()
{
	
	RenderContext::Instance().BeginSimpleRender();

	RenderContext::Instance().SetToShader(m_cameraMat);

	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();			// 現在のフレーム番号
	auto _commandList = RenderingEngine::Instance().GetCommandList();				// コマンドリスト
	auto _materialHeap = DescriptorHeapManager::Instance().GetDescriptorSRV()->GetHeap();				// マテリアル用ディスクリプタヒープ
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetDescriptorSRV()->GetHeap()
	};
	_commandList->SetDescriptorHeaps(std::size(_heaps), _heaps);								// ディスクリプタヒープをセット

	_commandList->SetGraphicsRootConstantBufferView(1, g_objBff[_currentIdx]->GetAddres());

	// メッシュの数だけインデックス分の描画を行う処理を回す
	for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	{
		auto _vbView = g_meshes[_i].vertexBuffer->View();
		auto _ibView = g_meshes[_i].indexBuffer->View();		// そのメッシュに対応するインデックスバッファビューを取得

	
		_commandList->IASetVertexBuffers(0, 1, &_vbView);				// 頂点バッファをセット
		_commandList->IASetIndexBuffer(&_ibView);						// インデックスバッファをセット

		_commandList->SetGraphicsRootConstantBufferView(2, g_matBff[_currentIdx]->GetAddres());
		_commandList->SetGraphicsRootConstantBufferView(3, g_materialBff[_currentIdx]->GetAddres());

		_commandList->SetGraphicsRootDescriptorTable(4, g_meshes[_i].materialHandle->handleGPU);	// マテリアルテクスチャをセット
		
		_commandList->DrawIndexedInstanced(static_cast<UINT>(g_meshes[_i].indices.size()), 1, 0, 0, 0);	// インデックスの数分描画
	}
}
