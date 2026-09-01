#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")


#include <list>
#include <vector>

#include <DirectXMath.h>
using namespace DirectX;

#include "DirectXTex.h"

#if _DEBUG
#pragma comment(lib,"DirectXTex_Debug.lib")
#else
#pragma comment(lib,"DirectXTex_Release.lib")
#endif 


#include "vector3.h"

#pragma comment (lib, "winmm.lib")

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) do { if ((p) != nullptr) { delete (p); (p) = nullptr; } } while (0)
#endif

#define SCREEN_WIDTH	(1280)
#define SCREEN_HEIGHT	(720)


HWND GetWindow();

void Invoke( std::function<void()> Function, int Time );

