#include "AdditivePoseLinkSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/StateMachineComponent.h"
#include "Application/Components/Character/Robot/AdditivePoseComponent.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

void AdditivePoseLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<const ModelComponent, const StateMachineComponent, AdditivePoseComponent>(
		Engine::ECS::ESystemType::Start,
		"AdditivePoseLinkSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag* a_startTag,
			const ModelComponent* a_modelArray,
			const StateMachineComponent* a_stateMachineArray,
			AdditivePoseComponent* a_additiveArray
		)
		{
			auto& _entryPool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				const StateMachineComponent& _stateComp = a_stateMachineArray[_i];
				AdditivePoseComponent& _additiveComp = a_additiveArray[_i];

				// モデル取得
				const auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_pModel) continue;

				// 設計図(アニメーター)取得
				const auto* _pAnimator = a_ctx.pServices->pResourceManager->Get(_stateComp.stateMachineHandle);
				if (!_pAnimator) continue;

				const auto& _defVec = _pAnimator->GetAdditiveBones();

				// 再実行(モデル差し替え等)に備えて、確保済みなら先に返却する
				if (_additiveComp.handle.IsValid())
				{
					_entryPool.FreeRange(_additiveComp.handle);
					_additiveComp.handle = {};
				}

				if (_defVec.empty()) continue;

				// 領域確保
				_additiveComp.handle = _entryPool.AllocateRange(static_cast<uint32_t>(_defVec.size()));
				auto _entryVec = _entryPool.RefRange(_additiveComp.handle);
				if (_entryVec.empty())
				{
					ENGINE_LOG("加算ポーズ用の領域確保に失敗しました");
					continue;
				}

				// ノード名ハッシュ → ノードインデックスを解決
				const auto& _nodeVec = _pModel->GetOriginalNodeVec();
				for (size_t _d = 0; _d < _defVec.size(); ++_d)
				{
					const auto& _def = _defVec[_d];

					AdditiveBoneEntry _entry = {};
					_entry.share = _def.share;
					_entry.axisScale = _def.axisScale;
					_entry.channel = _def.channel;
					_entry.nodeIdx = -1;

					for (size_t _n = 0; _n < _nodeVec.size(); ++_n)
					{
						if (_nodeVec[_n].nodeNameHash != _def.nodeNameHash) continue;
						_entry.nodeIdx = static_cast<int>(_n);
						break;
					}

					// モデルに存在しないノードを指していれば、そのボーンは無効のまま残す
					if (_entry.nodeIdx < 0)
					{
						ENGINE_LOG("加算ポーズの対象ノードが見つかりません : %s", _def.nodeName.c_str());
					}

					_entryVec[_d] = _entry;
				}

				// 実行時状態を初期化
				_additiveComp.currentAimQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
				_additiveComp.lagAngle = {};
				_additiveComp.lagVelocity = {};
				_additiveComp.prevVelocity = {};
				_additiveComp.isPrevVelocityValid = false;
			}
		}
	);
}
