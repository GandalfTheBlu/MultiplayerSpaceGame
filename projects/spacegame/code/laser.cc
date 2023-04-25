#include "config.h"
#include "laser.h"
#include "render/physics.h"

Laser::Laser(const Game::SpaceShip& spaceShip, uint64 currentTimeMillis, Render::ModelId _model)
{
	origin = spaceShip.position;
	orientation = spaceShip.orientation;
	spawnTimeMillis = currentTimeMillis;
	model = _model;
	spaceShipId = spaceShip.id;
}

Laser::~Laser(){}

float Laser::GetSecondsAlive(uint64 currentTimeMillis)
{
	return 0.001f * static_cast<float>(currentTimeMillis - spawnTimeMillis);
}

glm::vec3 Laser::GetCurrentPosition(uint64 currentTimeMillis, float velocity)
{
	return origin + orientation * glm::vec3(0.f, 0.f, GetSecondsAlive(currentTimeMillis) * velocity);
}

glm::vec3 Laser::GetDirection()
{
	return orientation * glm::vec3(0.f, 0.f, 1.f);
}

glm::mat4 Laser::GetLocalToWorld(uint64 currentTimeMillis, float velocity)
{
	glm::vec3 pos = GetCurrentPosition(currentTimeMillis, velocity);
	return glm::translate(pos) * (glm::mat4)orientation;
}

namespace Lasers
{
	std::vector<Laser> lasers;
	const float laserMaxTime = 2.f;
	const float laserSpeed = 20.f;

	void AddLaser(const Laser& laser)
	{
		lasers.push_back(laser);
	}

	void RemoveLaser(size_t index)
	{
		if (index < lasers.size())
			lasers.erase(lasers.begin() + index);
	}

	void UpdateAndDrawLasers(uint64 currentTimeMillis)
	{
		for (int i=(int)lasers.size()-1; i>=0; i--)
		{
			// check timeout
			if (lasers[i].GetSecondsAlive(currentTimeMillis) >= laserMaxTime)
			{
				RemoveLaser(i);
				continue;
			}

			// check space ship collision
			bool hitShip = false;
			glm::vec3 currentPos = lasers[i].GetCurrentPosition(currentTimeMillis, laserSpeed);
			for (int j = 0; j < SpaceShips::spaceShips.size(); j++)
			{
				if (SpaceShips::spaceShips[j]->id == lasers[i].spaceShipId)// ignore the ship it was fired from
					continue;

				glm::vec3 diff = currentPos - SpaceShips::spaceShips[j]->position;
				if (glm::dot(diff, diff) < SpaceShips::shipCollisionRadiusSquared)
				{
					SpaceShips::RegisterHitOnShip(j);
					hitShip = true;
					break;
				}
			}

			if (hitShip)
				continue;

			// check asteroid collision
			glm::vec3 direction = lasers[i].GetDirection();
			float maxDist = 1.f;

			Physics::RaycastPayload raycastResult = Physics::Raycast(currentPos, direction, maxDist);
			if (raycastResult.hit)
			{
				RemoveLaser(i);
				continue;
			}

			// draw
			Render::RenderDevice::Draw(lasers[i].model, lasers[i].GetLocalToWorld(currentTimeMillis, laserSpeed));
		}
	}
}