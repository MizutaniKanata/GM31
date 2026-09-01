#pragma once

#include "Scene.h"

class Title : public Scene
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;
};