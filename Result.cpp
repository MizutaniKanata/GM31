#include "main.h"
#include "manager.h"
#include "input.h"
#include "renderer.h"
#include "Result.h"
#include "polygon2D.h"
#include "Title.h"

void Result::Init()
{
	Manager::AddGameObject<Polygon2D>()->Init( 0.0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\result.png" );
}

void Result::Update()
{
	if ( Input::GetKeyTrigger( VK_RETURN ) )
	{
		Manager::SetScene<Title>();
	}
}

void Result::Draw()
{
}

void Result::Uninit()
{
}