#pragma once

class GameObject;
class Scene;

class Manager
{
private:
	static std::list<GameObject*> m_GameObjects;

	static Scene* m_Scene;
	static Scene* m_NextScene;
	static float m_DelayTime;

public:
	static void Init();
	static void Update();
	static void Draw();
	static void Uninit();

	template <typename T>
	static void SetScene( float time = 0.0f )
	{
		if ( m_NextScene == nullptr )
		{
			m_DelayTime = time;
			m_NextScene = new T();
		}
	}

	template <typename T>
	static T* AddGameObject()
	{
		T* gameObject = new T();
		gameObject->Init();
		m_GameObjects.push_back( gameObject );
		return gameObject;
	}

	template <typename T>
	static T* GetGameObject()
	{
		for ( GameObject* gameObject : m_GameObjects )
		{
			T* find = dynamic_cast<T*>( gameObject );
			if ( find != nullptr )
				return find;
		}
		return nullptr;
	}

	template <typename T>
	static std::vector<T*> GetGameObjects()
	{
		std::vector<T*> gameObjects;
		for ( GameObject* gameObject : m_GameObjects )
		{
			T* find = dynamic_cast<T*>( gameObject );
			if ( find != nullptr )
				gameObjects.push_back( find );
		}
		return gameObjects;
	}
};