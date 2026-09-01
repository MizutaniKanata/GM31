#pragma once

#include "component.h"

class GameObject
{
protected:
	bool m_Destroy = false;

	int m_Layer = 1;

	float m_CameraZ;// ソート用Z値

	Vector3 m_Position{};
	Vector3 m_Rotation{};
	Vector3 m_Scale{ 1.0f, 1.0f, 1.0f };

	std::list<Component*> m_Components;

public:
	int GetLayer() { return m_Layer; }
	float GetCameraZ() const { return m_CameraZ; }
	void CalcCameraZ( Vector3 CameraPosition, Vector3 CameraForward )
	{
		Vector3 direction = m_Position - CameraPosition;
		m_CameraZ = Vector3::dot( direction, CameraForward );// 内積
	}

	void SetPosition( const Vector3& position ) { m_Position = position; }
	Vector3 GetPosition() { return m_Position; }

	void SetRotation( const Vector3& rotation ) { m_Rotation = rotation; }
	Vector3 GetRotation() { return m_Rotation; }

	void SetScale( const Vector3& scale ) { m_Scale = scale; }
	Vector3 GetScale() { return m_Scale; }

	void SetDestroy() { m_Destroy = true; }

	bool Destroy()
	{
		if ( m_Destroy )
		{
			Uninit();
			delete this;
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual void Init() {}
	virtual void Update()
	{
		for ( Component* component : m_Components )
		{
			component->Update();

		}
	}
	virtual void Draw()
	{
		for ( Component* component : m_Components )
		{
			component->Draw();

		}
	}
	virtual void Uninit()
	{
		for ( Component* component : m_Components )
		{
			component->Uninit();
			delete component;
		}
	}

	template <typename T>
	T* AddComponent( GameObject* Object )
	{
		T* component = new T( Object );
		component->Init();
		m_Components.push_back( component );
		return component;
	}
	virtual Vector3 GetForward()
	{
		XMMATRIX rot = XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );

		Vector3 forward;
		XMStoreFloat3( (XMFLOAT3*)&forward, rot.r[ 2 ] );
		return forward;
	}
	virtual Vector3 GetRight()
	{
		XMMATRIX rot = XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );

		Vector3 forward;
		XMStoreFloat3( (XMFLOAT3*)&forward, rot.r[ 0 ] );
		return forward;
	}
};
