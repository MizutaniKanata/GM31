#include "main.h"
#include "manager.h"
#include <thread>

const char* CLASS_NAME = "AppClass";
const char* WINDOW_NAME = "DX11ƒQ[ƒ€";

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );


HWND g_Window;

HWND GetWindow()
{
	return g_Window;
}

static void EnsureWorkingDirectory()
{
	wchar_t modulePath[ MAX_PATH ]{};
	if ( GetModuleFileNameW( nullptr, modulePath, MAX_PATH ) == 0 )
	{
		return;
	}

	wchar_t dir[ MAX_PATH ]{};
	wcscpy_s( dir, modulePath );
	wchar_t* lastSlash = wcsrchr( dir, L'\\' );
	if ( !lastSlash )
	{
		return;
	}
	*lastSlash = L'\0';

	auto isExistingDirectory = []( const wchar_t* path ) -> bool
	{
		const DWORD attr = GetFileAttributesW( path );
		return ( attr != INVALID_FILE_ATTRIBUTES ) && ( attr & FILE_ATTRIBUTE_DIRECTORY );
	};

	for ( int i = 0; i < 8; i++ )
	{
		wchar_t assetPath[ MAX_PATH ]{};
		wchar_t shaderPath[ MAX_PATH ]{};
		swprintf_s( assetPath, L"%s\\asset", dir );
		swprintf_s( shaderPath, L"%s\\shader", dir );

		if ( isExistingDirectory( assetPath ) && isExistingDirectory( shaderPath ) )
		{
			SetCurrentDirectoryW( dir );
			return;
		}

		lastSlash = wcsrchr( dir, L'\\' );
		if ( !lastSlash )
		{
			return;
		}
		*lastSlash = L'\0';
	}
}


int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{

	EnsureWorkingDirectory();

	WNDCLASSEX wcex;
	{
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassEx( &wcex );


		RECT rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
		AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

		g_Window = CreateWindowEx( 0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr );
	}

	CoInitializeEx( nullptr, COINITBASE_MULTITHREADED );


	Manager::Init();



	ShowWindow( g_Window, nCmdShow );
	UpdateWindow( g_Window );




	DWORD dwExecLastTime;
	DWORD dwCurrentTime;
	timeBeginPeriod( 1 );
	dwExecLastTime = timeGetTime();
	dwCurrentTime = 0;



	MSG msg;
	while ( 1 )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				break;
			}
			else
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		else
		{
			dwCurrentTime = timeGetTime();

			if ( ( dwCurrentTime - dwExecLastTime ) >= ( 1000 / 60 ) )
			{
				dwExecLastTime = dwCurrentTime;

				Manager::Update();
				Manager::Draw();
			}
		}
	}

	timeEndPeriod( 1 );

	UnregisterClass( CLASS_NAME, wcex.hInstance );

	Manager::Uninit();

	CoUninitialize();

	return (int)msg.wParam;
}




LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

	switch ( uMsg )
	{
		case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

		case WM_KEYDOWN:
		switch ( wParam )
		{
			case VK_ESCAPE:
			DestroyWindow( hWnd );
			break;
		}
		break;

		default:
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

