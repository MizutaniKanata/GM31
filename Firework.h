#pragma once

#include "gameObject.h"
#include <vector>

class Firework : public GameObject
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

	static const int PARTICLE_MAX = 200;
	PARTICLE m_Particle[ PARTICLE_MAX ];

	enum class STATE { LAUNCH, EXPLODE, DEAD };
	STATE m_State = STATE::LAUNCH;

	Vector3 m_StartPosition{};
	Vector3 m_TargetHeight{};

	// 色の遷移リスト（爆発直後→中間→終盤の順で並べる）
	std::vector<XMFLOAT4> m_ColorStops;
	float m_LaunchSpeed = 8.0f;

public:
	void Init() override;
	void Update() override;
	void Draw() override;
	void Uninit() override;

	void Setup( const Vector3& startPosition, float explodeHeight,
				const std::vector<XMFLOAT4>& colorStops,
				float launchSpeed = 8.0f );

private:
	XMFLOAT4 GetColorByLifeRatio( float t ) const; // t: 1.0(発生直後)→0.0(消滅直前)
};
