#pragma once

#include "gameObject.h"

class Score : public GameObject
{
private:
	ID3D11Buffer* m_VertexBuffer = nullptr;

	ID3D11InputLayout* m_VertexLayout = nullptr;
	ID3D11VertexShader* m_VertexShader = nullptr;
	ID3D11PixelShader* m_PixelShader = nullptr;

	ID3D11ShaderResourceView* m_Texture = nullptr;

	int m_Value;

public:
	void Init() override;
	void Update()override;
	void Draw()override;
	void Uninit()override;

	void Add( int value ) { m_Value += value; }
};