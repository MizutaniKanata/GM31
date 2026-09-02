#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
//#include "modelRenderer.h"
#include "animationModel.h"
#include "player.h"
#include "camera.h"
#include "Bullet.h"
#include "Tree.h"
#include "Enemy.h"
#include "Box.h"
#include "audio.h"
#include "Meteor.h"
#include "Firework.h"

void Player::Init()
{
	m_Layer = 1;
	m_Position = { 0.0f, 0.0f, 0.0f };
	m_Scale = { 1.0f / 100, 1.0f / 100, 1.0f / 100 };

	m_AnimationModel = AddComponent<AnimationModel>( this );
	m_AnimationModel->Load( "asset\\model\\Akai.fbx" );
	m_AnimationModel->LoadAnimation( "asset\\model\\Akai_Idle.fbx", "Idle" );
	m_AnimationModel->LoadAnimation( "asset\\model\\Akai_Run.fbx", "Run" );

	// シェーダー読込
	Renderer::CreateVertexShader( &m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso" );

	Renderer::CreatePixelShader( &m_PixelShader, "shader\\unlitTexturePS.cso" );

	// SE読み込み
	m_JumpSE = AddComponent<Audio>( this );
	m_JumpSE->Load( "asset\\se\\wan.wav" );
}

void Player::Update()
{
	Vector3 oldPosition = m_Position;

	float dt = 1.0f / 60.0f;

	Camera* camera = Manager::GetGameObject<Camera>();
	Vector3 forward = camera->GetForward();
	Vector3 right = camera->GetRight();

	forward.y = 0.0f; // 水平移動のためにY成分をゼロにする
	forward.normalize();
	right.y = 0.0f; // 水平移動のためにY成分をゼロにする
	right.normalize();

	bool isMove = false;

	// キー入力で加速
	if ( Input::GetKeyPress( 'D' ) )
	{
		m_Velocity += right * 50.0f * dt;
		isMove = true;
	}
	if ( Input::GetKeyPress( 'A' ) )
	{
		m_Velocity -= right * 50.0f * dt;
		isMove = true;
	}
	if ( Input::GetKeyPress( 'W' ) )
	{
		m_Velocity += forward * 50.0f * dt;
		isMove = true;
	}
	if ( Input::GetKeyPress( 'S' ) )
	{
		m_Velocity -= forward * 50.0f * dt;
		isMove = true;
	}

	if ( isMove )
	{
		SetAnimation( "Run" );
	}
	else
	{
		SetAnimation( "Idle" );
	}

	// 移動方向に回転
	m_Rotation.y = atan2f( m_Velocity.x, m_Velocity.z );

	// ジャンプ
	if ( Input::GetKeyTrigger( 'K' ) )
	{
		m_Velocity.y += 20.0f; // 撃力 (瞬間的な力)
		// スケールアニメーション
		//m_Scale.y = 2.0f;
		//m_Scale.x = 0.5f;
		//m_Scale.z = 0.5f;
		m_JumpSE->Play();
	}
	// 重力
	m_Velocity.y += -50.0f * dt;

	// 抵抗力
	m_Velocity.x += -m_Velocity.x * 5.0f * dt;
	m_Velocity.z += -m_Velocity.z * 5.0f * dt;

	// 位置更新
	m_Position += m_Velocity * dt;

	// 前の接地判定を保存するためのローカル変数
	bool oldGround = m_Ground;
	m_Ground = false;

	// 地面衝突判定
	if ( m_Position.y < 0.0f )
	{
		m_Position.y = 0.0f;
		m_Velocity.y = 0.0f;
		m_Ground = true;
	}

	//// 線形補間でScale.xを戻す
	//m_Scale.x += ( 1.0f - m_Scale.x ) * 0.1f;
	//m_Scale.y += ( 1.0f - m_Scale.y ) * 0.1f;
	//m_Scale.z += ( 1.0f - m_Scale.z ) * 0.1f;

	// 木との衝突判定
	auto trees = Manager::GetGameObjects<Tree>();
	for ( auto tree : trees )
	{
		Vector3 treePosition = tree->GetPosition();
		Vector3 playerPosition = m_Position;

		// xz平面で計算
		treePosition.y = 0.0f;
		playerPosition.y = 0.0f;
		Vector3 direction = playerPosition - treePosition; // 方向ベクトル
		float length = direction.length(); // 距離を求める

		if ( length < 1.5f )
		{
			direction /= length; // 正規化
			direction *= 1.5f - length; //当たり判定サイズ

			m_Position += direction; // 押し出し
		}
	}

	// エネミーとの衝突判定
	auto enemys = Manager::GetGameObjects<Enemy>();
	for ( auto enemy : enemys )
	{
		Vector3 enemyPosition = enemy->GetPosition();
		Vector3 playerPosition = m_Position;

		// xz平面で計算
		enemyPosition.y = 0.0f;
		playerPosition.y = 0.0f;
		Vector3 direction = playerPosition - enemyPosition; // 方向ベクトル
		float length = direction.length(); // 距離を求める

		if ( length < 1.5f )
		{
			direction /= length; // 正規化
			direction *= 1.5f - length; //当たり判定サイズ

			m_Position += direction / 2; // 押し出し
		}
	}

	// ボックスとの衝突判定
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
			if ( boxPosition.y + boxScale.y < m_Position.y &&
				m_Position.y < boxPosition.y + boxScale.y * 2.0f &&
				 m_Velocity.y < 0.0f )
			{
				// 上面に衝突
				m_Position.y = boxPosition.y + boxScale.y * 2.0f;
				m_Velocity.y = 0.0f;
				m_Ground = true;
			}
			else if ( boxPosition.y - boxScale.y < m_Position.y &&
				m_Position.y < boxPosition.y + boxScale.y )
			{
				// 側面に衝突
				m_Position.x = oldPosition.x;
				m_Position.z = oldPosition.z;
				m_Velocity.x = 0.0f;
				m_Velocity.z = 0.0f;
			}
		}
	}

	//if ( !oldGround && m_Ground )
	//{
	//	// スケールアニメーション
	//	m_Scale.y = 0.5f;
	//	m_Scale.x = 1.5f;
	//	m_Scale.z = 1.5f;
	//}

	// 弾発射
	if ( Input::GetKeyTrigger( 'J' ) )
	{
		Bullet* bullet = Manager::AddGameObject<Bullet>();
		bullet->SetPosition( m_Position );
		bullet->SetVelocity( GetForward() * 30.0f );
	}

	// 歩きアニメーション
	if ( m_Ground )
	{
		float animationSpeed = 1.0f;
		float animationHeight = 0.03;

		m_MoveAnimation += m_Velocity.length() * dt;
		/*m_Scale.y += sinf( m_MoveAnimation * animationSpeed ) * animationHeight;*/
	}

	// 隕石ループ生成
	if ( Input::GetKeyTrigger( 'L' ) )
	{
		m_MeteorLoop = !m_MeteorLoop; // 再度押すと新規生成だけ止まる
		m_MeteorSpawnTimer = 0.0f;    // 押した瞬間に1個目を出す
	}
	if ( m_MeteorLoop )
	{
		m_MeteorSpawnTimer -= dt;
		if ( m_MeteorSpawnTimer <= 0.0f )
		{
			Vector3 spawnPos = m_Position + Vector3{ (float)( rand() % 10 - 5 ), 20.0f, (float)( rand() % 10 - 5 ) };

			Meteor* meteor = Manager::AddGameObject<Meteor>();
			meteor->Setup( spawnPos, {}, 15.0f );
			m_MeteorSpawnTimer = 1.0f; // 1秒間隔でループ生成
		}
	}

	// 花火ループ生成：Oキーでトグル
	if ( Input::GetKeyTrigger( 'O' ) )
	{
		m_FireworkLoop = !m_FireworkLoop;
		m_FireworkSpawnTimer = 0.0f;
	}
	if ( m_FireworkLoop )
	{
		m_FireworkSpawnTimer -= dt;
		if ( m_FireworkSpawnTimer <= 0.0f )
		{
			Vector3 spawnPos = m_Position + Vector3{ (float)( rand() % 10 - 5 ), 0.0f, (float)( rand() % 10 - 5 ) };

			Firework* firework = Manager::AddGameObject<Firework>();
			firework->Setup( spawnPos, 5.0f, {}, 3.0f );
			m_FireworkSpawnTimer = 1.2f;
		}
	}

	m_AnimationFrame++;

	GameObject::Update();
}

void Player::Draw()
{
	Camera* camera = Manager::GetGameObject<Camera>();
	if ( camera != nullptr )
	{
		camera->Draw();
	}

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

	m_AnimationModel->Update( m_AnimationName.c_str(), m_AnimationFrame );

	GameObject::Draw(); // 継承元のDraw()を呼び出す
}

void Player::Uninit()
{
	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();

	GameObject::Uninit();
}

void Player::SetAnimation( const char* animationName )
{
	if ( m_AnimationName != animationName )
	{
		m_AnimationName = animationName;
		m_AnimationFrame = 0;
	}
}
