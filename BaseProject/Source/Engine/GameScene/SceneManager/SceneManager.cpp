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

using namespace DirectX;

ConstantBuffer* g_matBff[FRAME_BUFFER_COUNT];

const wchar_t* MODEL_FILE_PATH = L"Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX";
std::vector<AssimpMesh> g_meshes;

#include "Engine/GPUResource/Model/Model.h"

bool SceneManager::Init()
{

	ImportSettings _importSettings = 
	{
		MODEL_FILE_PATH,
		g_meshes,
		false,
		true
	};

	m_spModel = std::make_shared<ModelResource>();
	if (!m_spModel->Load("Asset/Model/tank/tank.gltf"))
	{
		printf("モデルの読み込みに失敗\n");
		return false;
	}

	AssimpLoader _loader;
	if (!_loader.Load(_importSettings))
	{
		printf("モデルの読み込みに失敗\n");
		return false;
	}

	auto _eyePos = XMVectorSet(0.0f, 120.0f, 100.0f, 0.0f);								// 視点の位置
	auto _targetPos = XMVectorSet(0.0f,120.0f,0.0f,0.0f);								// 視点を向ける座標
	auto _upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);									// 上方向を表すベクトル
	auto _fov = XMConvertToRadians(60);															// 視野角
	auto _aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);		// アスペクト比

	DirectX::XMStoreFloat4x4(
		&m_cameraMat,
		DirectX::XMMatrixTranslationFromVector(_eyePos)
	);

	for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		g_matBff[_i] = new ConstantBuffer(sizeof(trans));
		if (!g_matBff[_i]->IsValid())
		{
			printf("マテリアル用定数バッファの生成に失敗\n");
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
	}

	printf("シーンの初期化に成功\n");
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
	// メッシュの数だけインデックス分の描画を行う処理を回す
	for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	{
		auto _vbView = g_meshes[_i].vertexBuffer->View();
		auto _ibView = g_meshes[_i].indexBuffer->View();		// そのメッシュに対応するインデックスバッファビューを取得

	
		_commandList->IASetVertexBuffers(0, 1, &_vbView);				// 頂点バッファをセット
		_commandList->IASetIndexBuffer(&_ibView);						// インデックスバッファをセット

		_commandList->SetGraphicsRootConstantBufferView(1, g_matBff[_currentIdx]->GetAddres());

		_commandList->SetGraphicsRootDescriptorTable(2, g_meshes[_i].materialHandle->handleGPU);	// マテリアルテクスチャをセット
		
		_commandList->DrawIndexedInstanced(static_cast<UINT>(g_meshes[_i].indices.size()), 1, 0, 0, 0);	// インデックスの数分描画
	}

	//RenderContext::Instance().DrawModel(m_spModel);
}
