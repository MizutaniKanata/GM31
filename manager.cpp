#include "main.h"
#include "manager.h"
#include "input.h"
#include "audio.h"
#include "renderer.h"
#include "gameObject.h"
#include "camera.h"
#include "Game.h"
#include "Title.h"

std::list<GameObject*> Manager::m_GameObjects; // staricメンバー変数はCPPファイルで定義する必要がある
Scene* Manager::m_Scene = nullptr;
Scene* Manager::m_NextScene = nullptr;
float Manager::m_DelayTime = 0.0f;

void Manager::Init()
{
	Input::Init();
	Renderer::Init();
	Audio::InitMaster();

	SetScene<Game>();
}
void Manager::Update()
{
	float dt = 1.0f / 60.0f;
	Input::Update();

	if ( m_Scene != nullptr )
		m_Scene->Update(); // ステートマシン、ステートパターン

	for ( GameObject* gameObject : m_GameObjects ) // 範囲for文
	{
		gameObject->Update(); // ポリモーフィズム
	}

	// ゲームオブジェクト削除、ラムダ式
	m_GameObjects.remove_if( []( GameObject* object )
	{
		if ( object != nullptr && object->Destroy() )
		{
			object->Uninit();
			SAFE_DELETE( object );
			return true;
		}
	} );

	//　シーン切り替え
	if ( m_NextScene != nullptr )
	{
		m_DelayTime -= dt;

		if ( m_DelayTime < 0.0f )
		{
			if ( m_Scene != nullptr )
			{
				SAFE_DELETE( m_Scene );
			}

			for ( GameObject* gameObject : m_GameObjects )
			{
				gameObject->Uninit();
				SAFE_DELETE( gameObject );
			}

			m_GameObjects.clear();

			m_Scene = m_NextScene;
			m_Scene->Init();

			m_NextScene = nullptr;
		}
	}
}

void Manager::Draw()
{
	Renderer::Begin();

	Camera* camera = GetGameObject<Camera>();

	if ( camera )
	{
		// Z値計算
		Vector3 forward = camera->GetForward();
		Vector3 position = camera->GetPosition();
		for ( GameObject* gameObject : m_GameObjects )
		{
			gameObject->CalcCameraZ( position, forward );
		}
		// Zソート
		m_GameObjects.sort( []( GameObject* a, GameObject* b )
		{
			return a->GetCameraZ() > b->GetCameraZ();

		} );
	}

	// 描画
	for ( int layer = 0; layer < 4; layer++ )
	{
		for ( GameObject* gameObject : m_GameObjects )
		{
			if ( gameObject->GetLayer() == layer )
				gameObject->Draw();
		}
	}
	Renderer::End();
}
void Manager::Uninit()
{
	if ( m_Scene != nullptr )
	{
		SAFE_DELETE( m_Scene );
	}

	for ( GameObject* gameObject : m_GameObjects )
	{
		SAFE_DELETE( gameObject );
	}
	m_GameObjects.clear();
	Audio::UninitMaster();
	Input::Uninit();
	Renderer::Uninit();
}