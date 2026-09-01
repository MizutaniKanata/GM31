#pragma once

#include "gameObject.h"
#include <vector>

class Meteor : public GameObject
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
		int LifeMax;
		Vector3 Position;
		Vector3 Velocity;
	};

	static const int PARTICLE_MAX = 150;
	PARTICLE m_Particle[ PARTICLE_MAX ]; // [0]=隕石本体、[1]以降=爆発の破片

	enum class STATE { FALL, EXPLODE, DEAD };
	STATE m_State = STATE::FALL;

	Vector3 m_StartPosition{};
	std::vector<XMFLOAT4> m_ColorStops;
	float m_FallSpeed = 15.0f;

public:
	void Init() override;
	void Update() override;
	void Draw() override;
	void Uninit() override;

	// Game/Playerから引数で渡して生成する
	void Setup( const Vector3& startPosition,
				const std::vector<XMFLOAT4>& colorStops,
				float fallSpeed = 15.0f );

private:
	XMFLOAT4 GetColorByLifeRatio( float t ) const;
};