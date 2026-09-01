#pragma once

#include "GameObject.h"

class Bullet : public GameObject
{
private:
	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	Vector3 m_Velocity{};
	float m_LifeTime{ 2.0f };

public:
	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	void SetVelocity( const Vector3& velocity ) { m_Velocity = velocity; }
};
