#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "Meteor.h"
#include "camera.h"

void Meteor::Init()
{
	m_Layer = 2;

	VERTEX_3D vertex[ 4 ];
	vertex[ 0 ].Position = XMFLOAT3( -0.5f, 0.5f, 0.0f );
	vertex[ 0 ].Normal = XMFLOAT3( 0.0f, 0.0f, -1.0f );
	vertex[ 0 ].Diffuse = XMFLOAT4( 1, 1, 1, 1 );
	vertex[ 0 ].TexCoord = XMFLOAT2( 0.0f, 0.0f );

	vertex[ 1 ].Position = XMFLOAT3( 0.5f, 0.5f, 0.0f );
	vertex[ 1 ].Normal = XMFLOAT3( 0.0f, 0.0f, -1.0f );
	vertex[ 1 ].Diffuse = XMFLOAT4( 1, 1, 1, 1 );
	vertex[ 1 ].TexCoord = XMFLOAT2( 1.0f, 0.0f );

	vertex[ 2 ].Position = XMFLOAT3( -0.5f, -0.5f, 0.0f );
	vertex[ 2 ].Normal = XMFLOAT3( 0.0f, 0.0f, -1.0f );
	vertex[ 2 ].Diffuse = XMFLOAT4( 1, 1, 1, 1 );
	vertex[ 2 ].TexCoord = XMFLOAT2( 0.0f, 1.0f );

	vertex[ 3 ].Position = XMFLOAT3( 0.5f, -0.5f, 0.0f );
	vertex[ 3 ].Normal = XMFLOAT3( 0.0f, 0.0f, -1.0f );
	vertex[ 3 ].Diffuse = XMFLOAT4( 1, 1, 1, 1 );
	vertex[ 3 ].TexCoord = XMFLOAT2( 1.0f, 1.0f );

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( VERTEX_3D ) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer( &bd, &sd, &m_VertexBuffer );

	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile( L"asset\\texture\\particle.png", WIC_FLAGS_NONE, &metadata, image );
	CreateShaderResourceView( Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &m_Texture );

	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\fireworkVS.cso" );
	Renderer::CreatePixelShader( &m_PixelShader, "shader\\fireworkPS.cso" );

	for ( int i = 0; i < PARTICLE_MAX; i++ )
		m_Particle[ i ].Enable = false;
}

void Meteor::Setup( const Vector3& startPosition, const std::vector<XMFLOAT4>& colorStops, float fallSpeed )
{
	m_StartPosition = startPosition;
	m_FallSpeed = fallSpeed;
	m_ColorStops = colorStops.empty()
		? std::vector<XMFLOAT4>{
			{ 1.0f, 0.6f, 0.2f, 1.0f },  // 落下中：燃える岩の橙色
			{ 1.0f, 0.9f, 0.6f, 1.0f },  // 衝突時：高温の白光
			{ 0.4f, 0.3f, 0.3f, 0.6f }   // 終盤：焦げた岩の灰色
	}
	: colorStops;
	SetPosition( startPosition );

	m_Particle[ 0 ].Enable = true;
	m_Particle[ 0 ].Life = 9999;
	m_Particle[ 0 ].LifeMax = 9999;
	m_Particle[ 0 ].Position = m_StartPosition;
	m_Particle[ 0 ].Velocity = { 0.0f, -m_FallSpeed, 0.0f };
}

XMFLOAT4 Meteor::GetColorByLifeRatio( float t ) const
{
	if ( m_ColorStops.size() == 1 ) return m_ColorStops[ 0 ];
	float segment = 1.0f / (float)( m_ColorStops.size() - 1 );
	float pos = ( 1.0f - t ) / segment;
	int idx = (int)pos;
	if ( idx >= (int)m_ColorStops.size() - 1 ) return m_ColorStops.back();
	float localT = pos - idx;
	const XMFLOAT4& a = m_ColorStops[ idx ];
	const XMFLOAT4& b = m_ColorStops[ idx + 1 ];
	return XMFLOAT4(
		a.x + ( b.x - a.x ) * localT,
		a.y + ( b.y - a.y ) * localT,
		a.z + ( b.z - a.z ) * localT,
		a.w + ( b.w - a.w ) * localT
	);
}

void Meteor::Update()
{
	const float dt = 1.0f / 60.0f;
	const Vector3 gravity{ 0.0f, -9.8f, 0.0f };

	if ( m_State == STATE::FALL )
	{
		// 落下
		m_Particle[ 0 ].Position += m_Particle[ 0 ].Velocity * dt;

		if ( m_Particle[ 0 ].Position.y <= 0.0f )
		{
			// 地面衝突
			m_Particle[ 0 ].Position.y = 0.0f;
			m_Particle[ 0 ].Enable = false;

			for ( int i = 1; i < PARTICLE_MAX; i++ )
			{
				float theta = ( (float)rand() / RAND_MAX ) * XM_2PI;
				float elevation = ( (float)rand() / RAND_MAX ) * ( XM_PIDIV2 * 0.8f ); // 低めに広がる
				float speed = 3.0f + ( (float)rand() / RAND_MAX ) * 7.0f;

				m_Particle[ i ].Enable = true;
				m_Particle[ i ].Life = 30 + rand() % 25;
				m_Particle[ i ].LifeMax = m_Particle[ i ].Life;
				m_Particle[ i ].Position = m_Particle[ 0 ].Position;
				m_Particle[ i ].Velocity.x = cosf( theta ) * cosf( elevation ) * speed;
				m_Particle[ i ].Velocity.y = sinf( elevation ) * speed;
				m_Particle[ i ].Velocity.z = sinf( theta ) * cosf( elevation ) * speed;
			}
			m_State = STATE::EXPLODE;
		}
	}
	else if ( m_State == STATE::EXPLODE )
	{
		bool anyAlive = false;
		for ( int i = 1; i < PARTICLE_MAX; i++ )
		{
			if ( m_Particle[ i ].Enable )
			{
				m_Particle[ i ].Velocity += gravity * dt;
				m_Particle[ i ].Position += m_Particle[ i ].Velocity * dt;
				if ( m_Particle[ i ].Position.y < 0.0f )
					m_Particle[ i ].Position.y = 0.0f;

				m_Particle[ i ].Life--;
				if ( m_Particle[ i ].Life < 0 )
					m_Particle[ i ].Enable = false;
				else
					anyAlive = true;
			}
		}

		if ( !anyAlive )
		{
			m_State = STATE::DEAD;
			SetDestroy(); // 全て消えたら自動削除
		}
	}

	GameObject::Update();
}

void Meteor::Draw()
{
	Renderer::GetDeviceContext()->IASetInputLayout( m_VertexLayout );
	Renderer::GetDeviceContext()->VSSetShader( m_VertexShader, NULL, 0 );
	Renderer::GetDeviceContext()->PSSetShader( m_PixelShader, NULL, 0 );

	Camera* camera = Manager::GetGameObject<Camera>();
	XMMATRIX view = camera->GetViewMatrix();
	XMMATRIX invView = XMMatrixInverse( NULL, view );
	invView.r[ 3 ].m128_f32[ 0 ] = 0.0f;
	invView.r[ 3 ].m128_f32[ 1 ] = 0.0f;
	invView.r[ 3 ].m128_f32[ 2 ] = 0.0f;

	MATERIAL material{};
	material.TextureEnable = true;

	UINT stride = sizeof( VERTEX_3D );
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );
	Renderer::GetDeviceContext()->PSSetShaderResources( 0, 1, &m_Texture );
	Renderer::GetDeviceContext()->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	Renderer::SetDepthEnable( false );

	const float SIZE_MULTIPLIER = 1.5f; // エフェクト全体の拡大率

	for ( int i = 0; i < PARTICLE_MAX; i++ )
	{
		if ( m_Particle[ i ].Enable )
		{
			float t = ( m_Particle[ i ].LifeMax > 0 )
				? (float)m_Particle[ i ].Life / (float)m_Particle[ i ].LifeMax
				: 1.0f;

			float scale = ( ( i == 0 ) ? 1.2f : ( 0.35f * t + 0.05f ) ) * SIZE_MULTIPLIER;

			MATERIAL m = material;
			m.Diffuse = ( i == 0 )
				? XMFLOAT4{ 1.0f, 0.5f, 0.1f, 1.0f }
			: GetColorByLifeRatio( t );
			Renderer::SetMaterial( m );

			XMMATRIX world, scaleMat, trans;
			scaleMat = XMMatrixScaling( scale, scale, scale );
			trans = XMMatrixTranslation( m_Particle[ i ].Position.x,
										 m_Particle[ i ].Position.y,
										 m_Particle[ i ].Position.z );
			world = scaleMat * invView * trans;

			Renderer::SetWorldMatrix( world );
			Renderer::GetDeviceContext()->Draw( 4, 0 );
		}
	}

	Renderer::SetDepthEnable( true );
}

void Meteor::Uninit()
{
	m_Texture->Release();
	m_VertexBuffer->Release();
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();
}