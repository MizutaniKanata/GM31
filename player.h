#pragma once

#include "gameObject.h"

class Player : public GameObject
{
private:
	Vector3 m_Velocity{};

	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	bool m_Ground = true;
	float m_MoveAnimation{};

	class Audio* m_JumpSE;

	// 隕石ループ生成用
	bool m_MeteorLoop = false;
	float m_MeteorSpawnTimer{};

	// 花火ループ生成用
	bool m_FireworkLoop = false;
	float m_FireworkSpawnTimer{};

	class AnimationModel* m_AnimationModel;
	int m_AnimationFrame{};

	std::string m_AnimationName;
public:
	void Init() override;
	void Update() override;
	void Draw() override;
	void Uninit() override;
	void SetAnimation( const char* animationName );
};