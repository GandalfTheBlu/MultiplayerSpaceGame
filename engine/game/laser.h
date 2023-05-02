#pragma once
#include "render/model.h"

namespace Game
{
struct Laser
{
	const glm::vec3 origin;
	const glm::quat orientation;
	const uint64 spawnTimeMillis;
	const uint64 despawnTimeMillis;
	const uint32 spaceShipId;
	const uint32 id;

	Laser(const glm::vec3& _origin, const glm::quat& _orientation, uint32 _spaceShipId, uint64 _spawnTimeMillis, uint64 _despawnTimeMillis, uint32 _id);
	~Laser();

	float GetSecondsAlive(uint64 currentTimeMillis);
	bool ShouldDespawn(uint64 currentTimeMillis);
	glm::vec3 GetCurrentPosition(uint64 currentTimeMillis, float velocity);
	glm::vec3 GetDirection();
	glm::mat4 GetLocalToWorld(uint64 currentTimeMillis, float velocity);
};
}