#include "SceneManager.h"

#include "Application/App.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"


#include "Framework/Shader/ShaderCommon/SharedStruct.h"

#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/GPUResource/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/GPUResource/Buffer/VertexBuffer/VertexBuffer.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"
#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"

#include "Engine/GPUResource/Model/ModelLoader/Assimp/AssimpLoader.h"
#include "Engine/GPUResource/Texture/Texture2D/Texture2D.h"

using namespace DirectX;

RootSignature* g_rootSignature;
PipelineState* g_pipelineState;

VertexBuffer* g_vertexBuffer;
IndexBuffer* g_indexBuffer;
ConstantBuffer* g_constantBuffer[FRAME_BUFFER_COUNT];

const wchar_t* MODEL_FILE_PATH = L"Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX";
std::vector<Mesh> g_meshes;
std::vector<VertexBuffer*> g_vertexBuffers;		// 頂点バッファ配列メッシュ数分
std::vector<IndexBuffer*> g_indexBuffers;		// インデックスバッファ配列メッシュ数分

DescriptorHeap* g_descriptorHeap;
std::vector<DescriptorHandle*> g_materialHandles;		// テクスチャハンドル配列

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

	//AssimpLoader _loader;
	//if (!_loader.Load(_importSettings))
	//{
	//	printf("モデルの読み込みに失敗\n");
	//	return false;
	//}


	//// メッシュの数分頂点バッファを作成
	//g_vertexBuffers.reserve(g_meshes.size());			// 要素数は増えないがメモリは確保される(再確保を防ぐため)
	//for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	//{
	//	auto _size = g_meshes[_i].vertices.size();
	//	auto _stride = sizeof(Vertex);
	//	auto _vertices = g_meshes[_i].vertices.data();
	//	auto _pVB = new VertexBuffer();
	//	if (!_pVB->Create(_size, _stride, _vertices))
	//	{
	//		printf("頂点バッファの生成に失敗\n");
	//		return false;
	//	}
	//	g_vertexBuffers.push_back(_pVB);
	//}

	//// メッシュの数分インデックスバッファを作成
	//g_indexBuffers.reserve(g_meshes.size());
	//for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	//{
	//	auto _size = g_meshes[_i].indices.size();
	//	auto _indices = g_meshes[_i].indices.data();
	//	auto _pIB = new IndexBuffer();
	//	
	//	if (!_pIB->Create(_size, sizeof(uint32_t),_indices))
	//	{
	//		printf("インデックスバッファの生成に失敗\n");
	//		return false;
	//	}
	//	g_indexBuffers.push_back(_pIB);
	//}

	//

	//auto _eyePos = XMVectorSet(0.0f, 120.0f, 75.0f, 0.0f);											// 視点の位置
	//auto _targetPos = XMVectorSet(0.0f,120.0f,0.0f,0.0f);										// 視点を向ける座標
	//auto _upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);											// 上方向を表すベクトル
	//auto _fov = XMConvertToRadians(60);															// 視野角
	//auto _aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);		// アスペクト比

	//for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	//{
	//	g_constantBuffer[_i] = new ConstantBuffer(sizeof(Transform));
	//	if (!g_constantBuffer[_i]->IsValid())
	//	{
	//		printf("変換行列用定数バッファの生成に失敗\n");
	//		return false;
	//	}

	//	// 変換行列の登録
	//	auto _ptr = g_constantBuffer[_i]->GetPtr<Transform>();
	//	_ptr->world = XMMatrixIdentity();
	//	_ptr->view = XMMatrixLookAtRH(_eyePos, _targetPos, _upward);
	//	_ptr->proj = XMMatrixPerspectiveFovRH(_fov, _aspect, 0.3f, 1000.0f);
	//}

	// マテリアルの読込
	g_descriptorHeap = new DescriptorHeap();
	g_materialHandles.clear();
	for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	{
		auto _texPath = FileUtility::ReplaceFilePathExtension(g_meshes[_i].diffuseMap,"tga");
		printf("テクスチャパス:%ls\n", _texPath.c_str());
		printf("拡張子:%ls\n", FileUtility::GetFilePathExtension(_texPath).c_str());
		auto _mainTex = Texture2D::Get(_texPath);
		auto _handle = g_descriptorHeap->Register(_mainTex);
		g_materialHandles.push_back(_handle);
	}

	g_rootSignature = new RootSignature();
	if (!g_rootSignature->IsValid())
	{
		printf("ルートシグネチャの生成に失敗\n");
		return false;
	}

	g_pipelineState = new PipelineState();
	g_pipelineState->SetInputLayout(Vertex::InputLayout);
	g_pipelineState->SetRootSignature(g_rootSignature->Get());
#ifdef _DEBUG
	g_pipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	g_pipelineState->SetPS(L"x64/Debug/SimplePS.cso");
#else
	g_pipelineState->SetVS(L"x64/Release/SimpleVS.cso");
	g_pipelineState->SetPS(L"x64/Release/SimplePS.cso");
#endif
	
	g_pipelineState->Create();
	if (!g_pipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return false;
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
	//m_rotateY += 0.2f;
	//// 現在のフレーム番号
	//auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();
	//// 現在のフレーム番号に対応する定数バッファを取得
	//auto _currentTransform = g_constantBuffer[_currentIdx]->GetPtr<Transform>();
	//_currentTransform->world = DirectX::XMMatrixRotationY(m_rotateY);
}

void SceneManager::Draw()
{
	//auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();	// 現在のフレーム番号
	//auto _commandList = RenderingEngine::Instance().GetCommandList();				// コマンドリスト
	//auto _materialHeap = g_descriptorHeap->GetHeap();							// マテリアル用ディスクリプタヒープ

	//// メッシュの数だけインデックス分の描画を行う処理を回す
	//for (size_t _i = 0; _i < g_meshes.size(); ++_i)
	//{
	//	auto _vbView = g_vertexBuffers[_i]->View();		// そのメッシュに対応する頂点バッファビューを取得
	//	auto _ibView = g_indexBuffers[_i]->View();		// そのメッシュに対応するインデックスバッファビューを取得

	//	_commandList->SetGraphicsRootSignature(g_rootSignature->Get());								// ルートシグネチャをセット	
	//	_commandList->SetPipelineState(g_pipelineState->Get());										// パイプラインステートをセット
	//	_commandList->SetGraphicsRootConstantBufferView(0, g_constantBuffer[_currentIdx]->GetAddres());		// 定数バッファをセット

	//	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);			// プリミティブトポロジーをセット
	//	_commandList->IASetVertexBuffers(0, 1, &_vbView);									// 頂点バッファをセット
	//	_commandList->IASetIndexBuffer(&_ibView);											// インデックスバッファをセット

	//	_commandList->SetDescriptorHeaps(1, &_materialHeap);								// ディスクリプタヒープをセット
	//	_commandList->SetGraphicsRootDescriptorTable(1, g_materialHandles[_i]->HandoleGPU);	// マテリアルテクスチャをセット

	//	_commandList->DrawIndexedInstanced(static_cast<UINT>(g_meshes[_i].indices.size()), 1, 0, 0, 0);	// インデックスの数分描画
	//}
}
