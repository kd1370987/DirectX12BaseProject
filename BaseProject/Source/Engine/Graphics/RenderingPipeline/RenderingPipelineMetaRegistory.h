#pragma once
namespace Engine::Graphics
{

	struct PassMeta
	{
		std::function<void()> constracta = nullptr;
	};

	// レンダリングパイプラインを登録する場所
	// インスタンスは保持しない。あくまで、クラスの型情報を保持して、作成時に渡す用
	class RenderingPipelineMetaRegistory
	{
	public:

	private:


	};
}