#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Explosion.h"
#include "Box.h"
#include "Score.h"

void Bullet::Init()
{
	m_Layer = 1;

	ModelRenderer* modelRenderer = AddComponent<ModelRenderer>( this );
	modelRenderer->Load( "asset\\model\\bullet.obj" );
	// シェーダー読込
	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso" );

	Renderer::CreatePixelShader( &m_PixelShader, "shader\\unlitTexturePS.cso" );
}

void Bullet::Update()
{
	float dt = 1.0f / 60.0f;

	m_Position += m_Velocity * dt;

	auto enemies = Manager::GetGameObjects<Enemy>();
	for ( auto enemy : enemies )
	{
		Vector3 direction = enemy->GetPosition() - m_Position;
		float lenght = direction.length();

		if ( lenght < 1.0f )
		{
			enemy->SetDestroy();
			SetDestroy();
			Manager::AddGameObject<Explosion>()->SetPosition( enemy->GetPosition() );
			Manager::GetGameObject<Score>()->Add( 1 );
			break;
		}
	}
	auto boxes = Manager::GetGameObjects<Box>();
	for ( auto box : boxes )
	{
		Vector3 boxPosition = box->GetPosition();
		Vector3 boxScale = box->GetScale();

		if ( boxPosition.x - boxScale.x < m_Position.x &&
			m_Position.x < boxPosition.x + boxScale.x &&
			boxPosition.z - boxScale.z < m_Position.z &&
			m_Position.z < boxPosition.z + boxScale.z )
		{
			SetDestroy();
			break;
		}
	}



	m_LifeTime -= dt;
	if ( m_LifeTime <= 0.0f )
	{
		// 自分自身削除
		SetDestroy();
	}

	GameObject::Update();
}

void Bullet::Draw()
{
	// 入力レイアウト設定
	Renderer::GetDeviceContext()->IASetInputLayout( m_VertexLayout );

	// シェーダ設定
	Renderer::GetDeviceContext()->VSSetShader( m_VertexShader, NULL, 0 );
	Renderer::GetDeviceContext()->PSSetShader( m_PixelShader, NULL, 0 );

	// マトリクス設定
	XMMATRIX world, scale, rot, trans;
	scale = XMMatrixScaling( m_Scale.x, m_Scale.y, m_Scale.z );
	rot = XMMatrixRotationRollPitchYaw( m_Rotation.x, m_Rotation.y + XM_PI, m_Rotation.z );
	trans = XMMatrixTranslation( m_Position.x, m_Position.y, m_Position.z );
	world = scale * rot * trans;

	Renderer::SetWorldMatrix( world );

	GameObject::Draw(); // 継承元のDraw()を呼び出す
}

void Bullet::Uninit()
{
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();

	GameObject::Uninit();
}
