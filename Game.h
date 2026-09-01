#pragma once

#include "Scene.h"

class Game : public Scene
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;
	void Uninit()override;
	virtual ~Game() override;
};