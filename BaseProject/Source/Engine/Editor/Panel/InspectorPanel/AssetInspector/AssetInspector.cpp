#include "AssetInspector.h"

#include "ResourceDraw/ResourceDraw.h"
#include "ResourceDraw/RenderingPipelineEdit/RenderingPipelineEdit.h"
#include "AssetLink.h"

namespace Engine::Editor::Inspector
{
	// NodeGraphEditor を前方宣言で持つので、生成と破棄は完全型が見えるここに置く
	AssetInspector::AssetInspector() = default;
	AssetInspector::~AssetInspector() = default;

	void AssetInspector::Draw(EditorContext& a_editContext)
	{
		// 参照先アセットから戻るためのバー。
		// 未選択でも出す(直前に見ていたものへ戻れるようにしておく)
		DrawAssetNavBar(a_editContext);
		ImGui::Separator();

		// 選択が変わっていればノードエディターを入れ替える。
		// 未選択になったときにも通して、抱えているものを捨てる
		SyncNodeEditor(a_editContext);

		// アセットが選択チェック
		if (!a_editContext.pAssetProp)
		{
			ImGui::Text("No selected Asset");
			return;
		}

		// アセットが選択されている場合
		// メタ情報
		ImGui::Text("Name : %s", a_editContext.pAssetProp->fileName.c_str());
		ImGui::Text("GUID : %s", a_editContext.pAssetProp->guid.String().c_str());
		ImGui::Separator();
		ImGui::Text("FilePath");
		for (auto& _ext : a_editContext.pAssetProp->extensionsVec)
		{
			auto _filePath = a_editContext.pAssetProp->filePath + a_editContext.pAssetProp->fileName + _ext;
			ImGui::Text("%s", _filePath.c_str());
		}
		ImGui::Separator();

		DrawByType(a_editContext);
	}

	//======================================================================================
	// 選択中のアセットに合わせてノードエディターを用意する
	//
	// 「今どのアセットのエディターを持っているか」を GUID で覚えておき、
	// 選択が変わったところで作り直す。
	// 作り直すと ImNodes のコンテキストごと入れ替わるので、
	// パン/ズームは初期位置に戻る(そのぶんコンテキストは溜まらない)
	//======================================================================================
	void AssetInspector::SyncNodeEditor(const EditorContext& a_editContext)
	{
		// 今の選択から、エディターを持つべきアセットのGUIDを決める
		Engine::GUID _targetGUID = {};
		if (a_editContext.pAssetProp && a_editContext.pAssetProp->type == "RenderingPipelineAsset")
		{
			_targetGUID = a_editContext.pAssetProp->guid;
		}

		// 変わっていなければそのまま使う
		if (_targetGUID == m_openedGUID) return;

		m_upNodeEditor.reset();
		m_openedGUID = _targetGUID;

		if (!_targetGUID.IsValid()) return;

		m_upNodeEditor = std::make_unique<RenderingPipelineEditor>(_targetGUID);
	}

	//======================================================================================
	// タイプごとのアセット描画
	//
	// 状態を持たないものは自由関数のまま。
	// ノードグラフを持つものだけ、上で用意した実体へ回す
	//======================================================================================
	void AssetInspector::DrawByType(EditorContext& a_editContext)
	{
		const auto& _type = a_editContext.pAssetProp->type;

		// ---- 実体を持つもの ----
		if (_type == "RenderingPipelineAsset")
		{
			if (m_upNodeEditor) m_upNodeEditor->Draw(a_editContext);
			return;
		}

		// ---- 状態を持たないもの ----
		if (_type == "Model")
		{
			ModelDraw(a_editContext);
		}
		else if (_type == "Mesh")
		{
			MeshDraw(a_editContext);
		}
		else if (_type == "Material")
		{
			MaterialDraw(a_editContext);
		}
		else if (_type == "Animation")
		{
			AnimationDraw(a_editContext);
		}
		else if (_type == "AnimatorAsset")
		{
			AnimatorDraw(a_editContext);
		}
		else if (_type == "ActionStateMachineAsset")
		{
			ActionStateMachineDraw(a_editContext);
		}
		else if (_type == "Texture")
		{
			TextureDraw(a_editContext);
		}
		else if (_type == "Shader")
		{
			ShaderDraw(a_editContext);
		}
		else if (_type == "ParticlesAsset")
		{
			ParticleDraw(a_editContext);
		}
		else if (_type == "ShadingModelTable")
		{
			ShadingModelTableDraw(a_editContext);
		}
		else if (_type == "Prefab")
		{
			PrefabDraw(a_editContext);
		}
		else if (_type == "AudioBehavior")
		{
			AudioBehaviorDraw(a_editContext);
		}
		else if (_type == "EffectAsset")
		{
			EffectAssetDraw(a_editContext);
		}
	}
}
