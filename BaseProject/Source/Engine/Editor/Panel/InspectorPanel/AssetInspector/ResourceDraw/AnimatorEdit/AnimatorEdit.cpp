#include "AnimatorEdit.h"

#include "../../../../../Helper/EditorField.inl"
#include "../../../../../Widget/StateGraphEditor/StateGraphEditor.h"

#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Editor::Inspector
{
	namespace
	{
		using Resource::AnimatorAsset;
		using Resource::AnimatorNode;

		//==================================================================================
		// アセットごとのノードエディタ
		//
		// ImNodes のコンテキスト(ノードの座標・選択状態)はグラフごとに持つ必要がある。
		// アセットは ImNodes を知らないので、エディター側でハンドルごとに持っておく。
		//==================================================================================
		struct AnimatorEditorState
		{
			StateGraphEditor<AnimatorNode> editor;

			// 最後に座標を反映したときのアセットの読み込み回数
			// 読み直されていたら、保存されていた座標をノードエディタへ流し直す
			uint32_t appliedLoadCount = (std::numeric_limits<uint32_t>::max)();
		};

		std::unordered_map<Resource::ID, AnimatorEditorState>& RefEditorStates()
		{
			static std::unordered_map<Resource::ID, AnimatorEditorState> s_states;
			return s_states;
		}

		AnimatorEditorState& RefEditorState(const Handle<AnimatorAsset>& a_handle, const AnimatorAsset& a_animator)
		{
			AnimatorEditorState& _state = RefEditorStates()[a_handle.id];

			if (_state.appliedLoadCount != a_animator.GetLoadCount())
			{
				_state.editor.RequestApplyLoadedPositions();
				_state.appliedLoadCount = a_animator.GetLoadCount();
			}
			return _state;
		}

		//----------------------------------------------------------------------------------
		// 保存 : ファイルパスはハンドル→GUID→パスで解決する
		//----------------------------------------------------------------------------------
		void DrawSave(
			const ECS::EngineServices& a_services,
			AnimatorAsset& a_animator,
			const Handle<AnimatorAsset>& a_handle,
			AnimatorEditorState& a_state)
		{
			if (!Engine::Editor::Button("Save") || !a_services.pAssetDatabase) return;

			// ノードエディタ上の今の座標をノードへ書き戻してから保存する
			a_state.editor.SyncPositions(a_animator.RefGraph());

			auto _guid = a_services.pResourceManager->GetCache<AnimatorAsset>(a_handle);
			auto _path = a_services.pAssetDatabase->GetFilePathFromGUID(_guid);
			a_animator.Save(_path, *a_services.pResourceManager);
			ENGINE_LOG("%s : Save AnimatorAsset", _path.c_str());
		}

		//----------------------------------------------------------------------------------
		// 加算ポーズの対象ボーン定義
		//----------------------------------------------------------------------------------
		void DrawAdditiveBones(const Resource::ResourceManager& a_resourceManager, AnimatorAsset& a_animator)
		{
			if (!Engine::Editor::CollapsingHeader("Additive Bones")) return;

			const auto* _pModel = a_resourceManager.Get(a_animator.RefModelHandle());
			if (!_pModel)
			{
				Engine::Editor::HelpText("Select a model first");
				return;
			}

			auto& _bones = a_animator.RefAdditiveBones();

			// チャンネルごとの配分合計。1.0から大きく外れていると見た目が破綻するので目安として出す。
			float _shareSum[3] = { 0.0f, 0.0f, 0.0f };
			for (const auto& _def : _bones)
			{
				size_t _chIdx = static_cast<size_t>(_def.channel);
				if (_chIdx < 3) _shareSum[_chIdx] += _def.share;
			}
			Engine::Editor::Value("Share sum", "Aim %.2f / LagArm %.2f / LagLeg %.2f", _shareSum[0], _shareSum[1], _shareSum[2]);

			int _removeIdx = -1;
			for (size_t _i = 0; _i < _bones.size(); ++_i)
			{
				Resource::AdditiveBoneDef& _def = _bones[_i];
				Engine::Editor::IDScope _id(static_cast<int>(_i));

				// 対象ノード選択
				Engine::Editor::ModelNodeField("Node", _pModel, _def.nodeName, _def.nodeNameHash);

				// チャンネル選択
				if (Engine::Editor::ComboScope _combo{ "Channel", Resource::ToString(_def.channel) })
				{
					const Resource::EAdditiveChannel _channelVec[] =
					{
						Resource::EAdditiveChannel::Aim,
						Resource::EAdditiveChannel::LagArm,
						Resource::EAdditiveChannel::LagLeg
					};
					for (auto _ch : _channelVec)
					{
						bool _selected = (_def.channel == _ch);
						if (Engine::Editor::Selectable(Resource::ToString(_ch), _selected))
						{
							_def.channel = _ch;
						}
						if (_selected) Engine::Editor::SetItemDefaultFocus();
					}
				}

				Engine::Editor::Field("Share", _def.share, 0.01f, 0.0f, 1.0f);

				// Lag系のみ軸ごとの効きを使う(符号を反転させると左右対称にできる)
				if (_def.channel != Resource::EAdditiveChannel::Aim)
				{
					Engine::Editor::Field("AxisScale", _def.axisScale, 0.01f);
				}

				if (Engine::Editor::DeleteButton("Remove")) _removeIdx = static_cast<int>(_i);

				Engine::Editor::Line();
			}

			if (_removeIdx >= 0)
			{
				_bones.erase(_bones.begin() + _removeIdx);
			}

			if (Engine::Editor::CreateButton("Add Bone"))
			{
				_bones.emplace_back();
			}
		}
	}

	//-----------------------------------------------------------------------------------------
	// アニメーター(アニメ用ステートマシン)の編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void AnimatorEdit(
		EditorContext& a_editContext,
		Resource::AnimatorAsset* a_pAnimator,
		const Handle<Resource::AnimatorAsset>& a_handle
	)
	{
		if (!a_pAnimator) { return; }

		// ---- 概要 ----
		Engine::Editor::Value("Name", "%s", a_pAnimator->GetName().c_str());

		// 開始ステート名 : ハッシュから引けなければハッシュのまま表示
		UINT _defaultStartHash = a_pAnimator->GetDefaultStartHash();
		auto _startName = a_pAnimator->GetNodeName(_defaultStartHash);
		if (_startName.empty())
		{
			Engine::Editor::Value("DefaultStart", "(unknown) %u", _defaultStartHash);
		}
		else
		{
			Engine::Editor::Value("DefaultStart", "%s", std::string(_startName).c_str());
		}

		Engine::Editor::Line();

		if (!a_editContext.pServices) return;
		const ECS::EngineServices& _services = *a_editContext.pServices;

		AnimatorEditorState& _state = RefEditorState(a_handle, *a_pAnimator);

		DrawSave(_services, *a_pAnimator, a_handle, _state);
		Engine::Editor::Line();

		// アニメを付随させるための参照モデル
		Engine::Editor::AssetField<Resource::Model>(
			_services,
			"Model",
			"Model",
			a_pAnimator->RefModelGUID(),
			a_pAnimator->RefModelHandle()
		);
		Engine::Editor::Line();

		// 加算ポーズの対象ボーン定義
		DrawAdditiveBones(*_services.pResourceManager, *a_pAnimator);
		Engine::Editor::Line();

		// ---- ノードエディタ ----
		// ノード本体(アニメ選択UI)だけを注入して汎用ノードエディタを描画
		_state.editor.Draw(a_pAnimator->RefGraph(),
			[a_pAnimator, &_services](AnimatorNode& a_node)
			{
				const Handle<Resource::Model>& _modelHandle = a_pAnimator->RefModelHandle();
				if (_modelHandle == Handle<Resource::Model>()) return;

				auto* _pModel = _services.pResourceManager->Get(_modelHandle);
				if (!_pModel) return;

				Engine::Editor::ItemWidthScope _itemWidth(130.0f);

				Engine::Editor::ModelAnimationField(_services, "Animation##ChangeAnimation", _pModel, a_node.playAnimData);
				Engine::Editor::Field("Speed##AnimationSpeed", a_node.speed, 0.01f, 0.0f);
				Engine::Editor::Field("Loop", a_node.isLoop);

				// 加算ポーズの効き(ステートごと)
				Engine::Editor::Field("Additive##AdditiveWeight", a_node.additiveWeight, 0.01f, 0.0f, 1.0f);
			});
	}
}
