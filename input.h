#pragma once

#include "main.h"

class Input
{
private:
	static BYTE m_OldKeyState[ 256 ];
	static BYTE m_KeyState[ 256 ];

public:
	static void Init();
	static void Update();
	static void Uninit();

	static bool GetKeyPress( BYTE KeyCode );
	static bool GetTrigger( BYTE KeyCode );
	static bool GetKeyTrigger( BYTE KeyCode );
};