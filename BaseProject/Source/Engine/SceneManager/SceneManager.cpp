#include "SceneManager.h"

#include "../Core/RenderingEngine.h"
#include "../Core/App.h"

#include "Framework/Shader/ShaderCommon/SharedStruct.h"
#include "Framework/Shader/RootSignature/RootSignature.h"
#include "Framework/Shader/PipelineState/PipelineState.h"

#include "Framework/Direct3D/Buffer/VertexBuffer.h"
#include "Framework/Direct3D/Buffer/ConstantBuffer.h"
#include "Framework/Direct3D/Buffer/IndexBuffer.h"

using namespace DirectX;

RootSignature* g_rootSignature;
PipelineState* g_pipelineState;

VertexBuffer* g_vertexBuffer;
IndexBuffer* g_indexBuffer;
ConstantBuffer* g_constantBuffer[RenderingEngine::FRAME_BUFFER_COUNT];

bool SceneManager::Init()
{
	// 三角形描画準備
	Vertex _vertices[4] = {};
	_vertices[0].position = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	_vertices[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	_vertices[1].position = XMFLOAT3(1.0f, 1.0f, 0.0f);
	_vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	_vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);
	_vertices[2].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	_vertices[3].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	_vertices[3].color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

	// 頂点バッファの生成
	auto _vertexSize = sizeof(Vertex) * std::size(_vertices);
	auto _vertexStride = sizeof(Vertex);
	g_vertexBuffer = new VertexBuffer(_vertexSize, _vertexStride, _vertices);
	if (!g_vertexBuffer->IsValid())
	{
		printf("頂点バッファの生成に失敗\n");
		return false;
	}

	uint32_t _indices[] = { 0,1,2,0,2,3 };		// これに書かれている順序で描画する

	// インデックスバッファの生成
	auto _size = sizeof(uint32_t) * std::size(_indices);
	g_indexBuffer = new IndexBuffer(_size, _indices);
	if (!g_indexBuffer->IsValid())
	{
		printf("インデックスバッファの生成に失敗\n");
		return false;
	}

	auto _eyePos = XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f);											// 視点の位置
	auto _targetPos = XMVectorZero();															// 視点を向ける座標
	auto _upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);											// 上方向を表すベクトル
	auto _fov = XMConvertToRadians(37.5);														// 視野角
	auto _aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);		// アスペクト比

	for (size_t _i = 0; _i < RenderingEngine::FRAME_BUFFER_COUNT; ++_i)
	{
		g_constantBuffer[_i] = new ConstantBuffer(sizeof(Transform));
		if (!g_constantBuffer[_i]->IsValid())
		{
			printf("変換行列用定数バッファの生成に失敗\n");
			return false;
		}

		// 変換行列の登録
		auto _ptr = g_constantBuffer[_i]->GetPtr<Transform>();
		_ptr->world = XMMatrixIdentity();
		_ptr->view = XMMatrixLookAtRH(_eyePos, _targetPos, _upward);
		_ptr->proj = XMMatrixPerspectiveFovRH(_fov, _aspect, 0.3f, 1000.0f);
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
#ifdef DEBUG
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
	m_rotateY += 0.2f;
	// 現在のフレーム番号
	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();
	// 現在のフレーム番号に対応する定数バッファを取得
	auto _currentTransform = g_constantBuffer[_currentIdx]->GetPtr<Transform>();
	_currentTransform->world = DirectX::XMMatrixRotationY(m_rotateY);
}

void SceneManager::Draw()
{
	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();	// 現在のフレーム番号
	auto _commandList = RenderingEngine::Instance().CommandList();				// コマンドリスト
	auto _vbView = g_vertexBuffer->View();		// 頂点バッファビュー
	auto _ibView = g_indexBuffer->View();		// インデックスバッファビュー


	// ルートシグネチャをセット
	_commandList->SetGraphicsRootSignature(g_rootSignature->Get());
	// パイプラインステートをセット
	_commandList->SetPipelineState(g_pipelineState->Get());
	// 定数バッファをセット
	_commandList->SetGraphicsRootConstantBufferView(0,g_constantBuffer[_currentIdx]->GetAddres());


	// 三角形を描画する設定にする
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファをスロット0版を使って1個だけ設定する
	_commandList->IASetVertexBuffers(0,1,&_vbView);

	// インデックスバッファをセット
	_commandList->IASetIndexBuffer(&_ibView);


	// 6個の頂点を描画する
	_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
