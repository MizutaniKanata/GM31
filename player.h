#pragma once

#include "gameObject.h"

class Player : public GameObject
{
private:
	// 前方宣言
	class Audio* m_JumpSE;
	class AnimationModel* m_AnimationModel;

	ID3D11InputLayout* m_VertexLayout{};
	ID3D11VertexShader* m_VertexShader{};
	ID3D11PixelShader* m_PixelShader{};

	Vector3 m_Velocity{};
	float m_MoveAnimation{};
	float m_MeteorSpawnTimer{};
	float m_FireworkSpawnTimer{};
	int m_AnimationFrame{};
	std::string m_AnimationName;
	int m_NextAnimationFrame{};
	std::string m_NextAnimationName;
	float m_Blend{};

	bool m_isGround = true;
	// 隕石ループ生成用
	bool m_isMeteorLoop = false;
	// 花火ループ生成用
	bool m_isFireworkLoop = false;

public:
	void Init() override;
	void Update() override;
	void Draw() override;
	void Uninit() override;
	void SetAnimation( const char* animationName );
};