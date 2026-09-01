#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "Title.h"
#include "polygon2D.h"
#include "Game.h"

void Title::Init()
{
	Manager::AddGameObject<Polygon2D>()->Init( 0.0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\title.png" );
}

void Title::Update()
{
	if ( Input::GetKeyTrigger( VK_RETURN ) )
	{
		Manager::SetScene<Game>();
	}
}

void Title::Draw()
{
}

void Title::Uninit()
{
}