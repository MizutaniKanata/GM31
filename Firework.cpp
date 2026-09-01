#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "Firework.h"
#include "camera.h"

void Firework::Init()
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
	assert( m_Texture );

	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\fireworkVS.cso" );
	Renderer::CreatePixelShader( &m_PixelShader, "shader\\fireworkPS.cso" );

	for ( int i = 0; i < PARTICLE_MAX; i++ )
		m_Particle[ i ].Enable = false;

	// 発射用の先頭パーティクル（1発目・上昇する火の玉）
	m_Particle[ 0 ].Enable = true;
	m_Particle[ 0 ].Life = 9999;
	m_Particle[ 0 ].LifeMax = 9999;
	m_Particle[ 0 ].Position = m_StartPosition;
	m_Particle[ 0 ].Velocity = { 0.0f, m_LaunchSpeed, 0.0f };
}

void Firework::Setup( const Vector3& startPosition, float explodeHeight,
					  const std::vector<XMFLOAT4>& colorStops, float launchSpeed )
{
	m_StartPosition = startPosition;
	m_TargetHeight = { startPosition.x, startPosition.y + explodeHeight, startPosition.z };
	m_ColorStops = colorStops.empty()
		? std::vector<XMFLOAT4>{
			{ 1.0f, 1.0f, 0.9f, 1.0f },
			{ 1.0f, 0.15f, 0.05f, 1.0f },
			{ 0.6f, 0.02f, 0.02f, 0.6f }
	}
	: colorStops;
	m_LaunchSpeed = launchSpeed;
	SetPosition( startPosition );

	// Init()実行後にSetupが呼ばれるため、ここで速度を確定させる
	m_Particle[ 0 ].Enable = true;
	m_Particle[ 0 ].Life = 9999;
	m_Particle[ 0 ].LifeMax = 9999;
	m_Particle[ 0 ].Position = m_StartPosition;
	m_Particle[ 0 ].Velocity = { 0.0f, m_LaunchSpeed, 0.0f };
}

// t=1.0(発生直後の白熱状態) → t=0.0(消滅直前)を色リストで線形補間
XMFLOAT4 Firework::GetColorByLifeRatio( float t ) const
{
	if ( m_ColorStops.size() == 1 ) return m_ColorStops[ 0 ];

	// 区間を等分し、tの位置に対応する2色を線形補間
	float segment = 1.0f / (float)( m_ColorStops.size() - 1 );
	float pos = ( 1.0f - t ) / segment; // 0.0(発生直後)→N-1(終盤)
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

void Firework::Update()
{
	const float dt = 1.0f / 60.0f;
	const Vector3 gravity{ 0.0f, -9.8f, 0.0f };

	if ( m_State == STATE::LAUNCH )
	{
		// 先頭(1発目)を上昇させる
		m_Particle[ 0 ].Position.y += m_Particle[ 0 ].Velocity.y * dt;

		if ( m_Particle[ 0 ].Position.y >= m_TargetHeight.y )
		{
			// 爆発発生：中心から放射状にパーティクルをばらまく
			m_Particle[ 0 ].Enable = false;

			for ( int i = 1; i < PARTICLE_MAX; i++ )
			{
				float theta = ( (float)rand() / RAND_MAX ) * XM_2PI;
				float phi = ( (float)rand() / RAND_MAX ) * XM_PI;
				float speed = 4.0f + ( (float)rand() / RAND_MAX ) * 6.0f;

				m_Particle[ i ].Enable = true;
				m_Particle[ i ].Life = 45 + rand() % 20;
				m_Particle[ i ].LifeMax = m_Particle[ i ].Life;
				m_Particle[ i ].Position = m_Particle[ 0 ].Position;
				m_Particle[ i ].Velocity.x = sinf( phi ) * cosf( theta ) * speed;
				m_Particle[ i ].Velocity.y = cosf( phi ) * speed;
				m_Particle[ i ].Velocity.z = sinf( phi ) * sinf( theta ) * speed;
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
				m_Particle[ i ].Velocity += gravity * dt * 0.3f; // ふわっと落ちる
				m_Particle[ i ].Position += m_Particle[ i ].Velocity * dt;
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
			SetDestroy(); // GameObjectの仕組みで自動削除
		}
	}

	GameObject::Update();
}

void Firework::Draw()
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
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.TextureEnable = true;
	Renderer::SetMaterial( material );

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

			// 既存のscale計算に1.5倍を掛ける
			float scale = ( ( i == 0 ) ? 1.0f : ( 0.3f * t + 0.05f ) ) * SIZE_MULTIPLIER;

			MATERIAL m = material;
			m.Diffuse = ( i == 0 )
				? XMFLOAT4{ 1.0f, 0.9f, 0.6f, 1.0f }
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

void Firework::Uninit()
{
	m_Texture->Release();
	m_VertexBuffer->Release();
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();
}