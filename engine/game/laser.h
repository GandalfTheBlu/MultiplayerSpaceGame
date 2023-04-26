#pragma once
#include "render/model.h"
#include <vector>
#include "spaceship.h"

struct Laser
{
	glm::vec3 origin;
	glm::quat orientation;
	uint64 spawnTimeMillis;
	Render::ModelId model;
	int spaceShipId;

	Laser(const Game::SpaceShip& spaceShip, uint64 currentTimeMillis, Render::ModelId _model);
	~Laser();

	float GetSecondsAlive(uint64 currentTimeMillis);
	glm::vec3 GetCurrentPosition(uint64 currentTimeMillis, float velocity);
	glm::vec3 GetDirection();
	glm::mat4 GetLocalToWorld(uint64 currentTimeMillis, float velocity);
};

namespace Lasers
{
	extern std::vector<Laser> lasers;
	extern const float laserMaxTime;
	extern const float laserSpeed;

	void AddLaser(const Laser& laser);
	void RemoveLaser(size_t index);
	void UpdateAndDrawLasers(uint64 currentTimeMillis);
}