#include "main.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "Enemy.h"

void Enemy::Init()
{
	m_Layer = 1;

	ModelRenderer* modelRenderer = AddComponent<ModelRenderer>( this );
	modelRenderer->Load( "asset\\model\\player.obj" );
	// シェーダー読込
	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso" );

	Renderer::CreatePixelShader( &m_PixelShader, "shader\\unlitTexturePS.cso" );
}

void Enemy::Update()
{
	GameObject::Update();
}

void Enemy::Draw()
{
	// 入力レイアウト設定
	Renderer::GetDeviceContext()->IASetInputLayout( m_VertexLayout );

	// シェーダ設定
	Renderer::GetDeviceContext()->VSSetShader( m_VertexShader, NULL, 0 );
	Renderer::GetDeviceContext()->PSSetShader( m_PixelShader, NULL, 0 );

	// マトリクス設定
	XMMATRIX world, scale, rot, trans;
	scale = XMMatrixScaling( m_Scale.x, m_Scale.y, m_Scale.z );
	rot = XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y, m_Rotation.z );
	trans = XMMatrixTranslation( m_Position.x, m_Position.y, m_Position.z );
	world = scale * rot * trans;

	Renderer::SetWorldMatrix( world );

	GameObject::Draw(); // 継承元のDraw()を呼び出す
}

void Enemy::Uninit()
{
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();

	GameObject::Uninit();
}
