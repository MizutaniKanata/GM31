#pragma once

#include "gameObject.h"

class Particle : public GameObject
{
private:

	ID3D11Buffer* m_VertexBuffer;

	ID3D11InputLayout* m_VertexLayout;
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;

	ID3D11ShaderResourceView* m_Texture;

	struct PARTICLE
	{
		bool Enable;
		int Life;
		Vector3 Position;
		Vector3 Velocity;
	};

	static const int PARTICLE_MAX = 1000;
	PARTICLE m_Particle[ PARTICLE_MAX ];

public:
	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;
};