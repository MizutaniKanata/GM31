#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "Game.h"
#include "Result.h"
#include "polygon2D.h"
#include "camera.h"
#include "field.h"
#include "player.h"
#include "Enemy.h"
#include "Tree.h"
#include "Sky.h"
#include "Box.h"
#include "Particle.h"
#include "Score.h"
#include "Firework.h"

void Game::Init()
{
	Manager::AddGameObject<Camera>();
	Manager::AddGameObject<Sky>();
	Manager::AddGameObject<Field>();

	//Box* box = Manager::AddGameObject<Box>();
	//box->SetPosition( { 5.0f,0.0f,-5.0f } );
	//box->SetScale( { 3.0f,1.0f,3.0f } );

	Manager::AddGameObject<Player>();

	//Manager::AddGameObject<Enemy>()->SetPosition( { -2.0f,0.0,1.0f } );
	//Manager::AddGameObject<Enemy>()->SetPosition( { 0.0f,0.0,1.0f } );
	//Manager::AddGameObject<Enemy>()->SetPosition( { 2.0f,0.0,1.0f } );

	for ( int i = 0; i < 5; i++ )
	{
		Vector3 postision = { (float)( rand() % 40 - 20 ),0.0f,(float)( rand() % 40 - 20 ) };
		Manager::AddGameObject<Enemy>()->SetPosition( postision );
	}

	//for ( int i = 0; i < 20; i++ )
	//{
	//	Vector3 postision = { (float)( rand() % 40 - 20 ),0.0f,(float)( rand() % 40 - 20 ) };
	//	Manager::AddGameObject<Tree>()->SetPosition( postision );
	//}

	/*AddGameObject<Particle>()->SetPosition( { -2.0f,1.0f,-1.0f } );*/

	//Manager::AddGameObject<Polygon2D>()->Init( 0.0, 0.0, 200.0f, 200.0f, L"asset\\texture\\2dtest.png" );
	//Manager::AddGameObject<Score>()->SetPosition( { 100.0f,100.0f,0.0f } );
}

void Game::Update()
{
	auto enemies = Manager::GetGameObjects<Enemy>();
	if ( enemies.size() == 0 )
	{
		Manager::SetScene<Result>( 3.0f );
	}
}

void Game::Draw()
{
}

void Game::Uninit()
{
}

Game::~Game()
{
}
