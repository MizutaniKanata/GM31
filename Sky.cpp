#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "Sky.h"
#include "camera.h"

void Sky::Init()
{
	m_Layer = 1;

	ModelRenderer* modelRenderer = AddComponent<ModelRenderer>( this );
	modelRenderer->Load( "asset\\model\\sky.obj" );
	// シェーダー読込
	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso" );

	Renderer::CreatePixelShader( &m_PixelShader, "shader\\unlitTexturePS.cso" );

	m_Scale = { 100.0f,100.0f,100.0f };
}

void Sky::Update()
{
	Camera* camera = Manager::GetGameObject<Camera>();
	m_Position = camera->GetPosition();

	GameObject::Update();
}

void Sky::Draw()
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

void Sky::Uninit()
{
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();

	GameObject::Uninit();
}