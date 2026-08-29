#include "TestGBufferPass.h"

namespace Engine::Graphics::Pipeline
{
	// 入力ピンは「役割」だけ決める。中身のリソースはつないだ相手からもらう。
	// 出力ピンは自分が作るリソースなので、名前とフォーマットまでここで決める
	void TestGBufferPass::SetupSlots()
	{
		// 事前デプス : つながっていなければ自前で作る想定なので任意入力にする。
		// 必須にすると、置いただけのパスが検証で止まってしまう
		DeclareInput("PreDepth", EAccessType::Depth_Write, EPassSlotType::Texture, false);

		DeclareOutput("Albedo", "GBufferAlbedo", DXGI_FORMAT_R8G8B8A8_UNORM);
		DeclareOutput("Normal", "GBufferNormal", DXGI_FORMAT_R10G10B10A2_UNORM);
		DeclareOutput("Depth", "SceneDepth", DXGI_FORMAT_D32_FLOAT, EAccessType::Depth_Write);
	}

	// リソースが揃った後に呼ばれる。
	// ディスクリプタやPSOを引いておくならここ
	void TestGBufferPass::Compile(const PassContext& a_context)
	{
		(void)a_context;
	}

	// バリア・レンダーターゲット切り替え・クリアはグラフ側が済ませてあるので、
	// ここは自分の描画コマンドを積むだけでよい
	void TestGBufferPass::Update(const PassContext& a_context)
	{
		(void)a_context;
	}

	// 選択中に出る詳細側のUI
	// ノードの中に詰めると線が見えなくなるので、細かい設定はこちらへ置く
	bool TestGBufferPass::EditUpdate()
	{
		bool _isEdit = false;

		if (ImGui::TreeNodeEx("Output", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (Slot& _out : m_outputSlots)
			{
				ImGui::PushID(_out.pinID);
				ImGui::Text("%s : %s", _out.pinName.c_str(), _out.name.c_str());
				// どれもリソースの要件を変えるので、触られたら組み直しが要る
				_isEdit |= Editor::EditorHelper::DrawEnumCombo("LoadOp", _out.loadOp);
				_isEdit |= Editor::EditorHelper::DrawEnumCombo("Access", _out.accessType);
				_isEdit |= ImGui::DragFloat("Scale", &_out.scale, 0.01f, 0.01f, 4.0f);
				ImGui::Separator();
				ImGui::PopID();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Input", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const Slot& _in : m_inputSlots)
			{
				if (_in.IsConnected())	ImGui::Text("%s : %s", _in.pinName.c_str(), _in.name.c_str());
				else					ImGui::TextDisabled("%s : (not connected)", _in.pinName.c_str());
			}
			ImGui::TreePop();
		}

		return _isEdit;
	}

	// ノードの中に出すUI : ピンと削除ボタンは呼び出し側が描くので、ここは固有分だけ
	void TestGBufferPass::EditNode()
	{}

	// このパスは固有のパラメータをまだ持っていないので何もしない。
	// 定数バッファなどを足したらここへ書く
	void TestGBufferPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
