#pragma once

class Scene
{
public:
	virtual void Init() {}
	virtual void Update() {}
	virtual void Draw() {}
	virtual void Uninit() {}
	virtual ~Scene() {}
};