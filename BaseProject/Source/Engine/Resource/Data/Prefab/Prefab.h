#pragma once
//==========================================================================================
//
// Prefab
//
// 「エンティティのテンプレート」リソース。
// シグネチャ(どのコンポーネントを持つか)と、各コンポーネントの初期値バイト列を保持する。
// 他リソースと同じく GUID で参照し、エディタ上で ECS のエンティティインスペクタと
// 同じ感覚でコンポーネントを追加・削除・編集できる。
//
//==========================================================================================

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}

namespace Engine::Resource
{
	class Prefab
	{
	public:
		Prefab();
		~Prefab() = default;
		NON_COPYABLE_MOVABLE(Prefab);

		//----------------------------------------------------------------------------------
		// リソースローダー / 生成(他リソースと同じ入口)
		//----------------------------------------------------------------------------------
		// ファイルパスから読み込んで実体を返す(ResourceManager の Load から使用)
		static Prefab LoadFromFile(const std::string& a_path);
		// 空のプレハブアセットを新規作成(AssetDataBasePanel から使用)
		static void   Create(const std::string& a_path, const std::string& a_name);

		//----------------------------------------------------------------------------------
		// 保存 / 読み込み(コンポーネントのメタ情報が必要なので World を受け取る)
		//----------------------------------------------------------------------------------
		void Save(ECS::World* a_pWorld, const std::string& a_savePath);
		void Load(ECS::World* a_pWorld, const std::string& a_filePath);

		//----------------------------------------------------------------------------------
		// 実体化
		// このプレハブのシグネチャ・初期値からエンティティを1つ生成して返す。
		// (シーン読み込みと同じく CreateEntity → 各コンポーネントへ初期値を流し込む)
		//----------------------------------------------------------------------------------
		ECS::Entity Instantiate(ECS::World* a_pWorld);

		//----------------------------------------------------------------------------------
		// コンポーネント操作(エディタから使用)
		//----------------------------------------------------------------------------------
		// 既定値でコンポーネントを追加する
		void AddComponentDefault(ECS::World* a_pWorld, ECS::ComponentTypeID a_compTypeID);
		// 既存データ(エンティティのコンポーネント等)をコピーして追加する
		void AddComponentData(ECS::World* a_pWorld, ECS::ComponentTypeID a_compTypeID, const uint8_t* a_pSrc);
		// コンポーネントを削除する
		void RemoveComponent(ECS::ComponentTypeID a_compTypeID);

		// 所持しているか
		bool Has(ECS::ComponentTypeID a_compTypeID) const;
		// コンポーネント先頭バイトへのポインタ(エディタ編集用。無ければ nullptr)
		uint8_t* RefData(ECS::ComponentTypeID a_compTypeID);
		// シグネチャ参照
		const ECS::Signature& GetSignature() const { return m_sigunature; }

		//----------------------------------------------------------------------------------
		// シリアライズ処理
		//----------------------------------------------------------------------------------
		void Archive(Persistence::Archive& a_ar, ECS::World* a_pWorld);

	private:

		// 生成エンティティシグネチャ
		ECS::Signature m_sigunature;

		// コンポーネントごとの初期値バイト列(型ID -> バイト列)
		std::unordered_map<ECS::ComponentTypeID, std::vector<uint8_t>> m_dataMap = {};

	};
}
