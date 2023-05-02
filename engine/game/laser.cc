#include "config.h"
#include "laser.h"
#include "render/physics.h"

namespace Game
{
Laser::Laser(const glm::vec3& _origin, const glm::quat& _orientation, uint32 _spaceShipId, uint64 _spawnTimeMillis, uint64 _despawnTimeMillis, uint32 _id) :
	origin(_origin),
	orientation(_orientation),
	spaceShipId(_spaceShipId),
	spawnTimeMillis(_spawnTimeMillis),
	despawnTimeMillis(_despawnTimeMillis),
	id(_id)
{}

Laser::~Laser() {}

float Laser::GetSecondsAlive(uint64 currentTimeMillis)
{
	return 0.001f * static_cast<float>(currentTimeMillis - spawnTimeMillis);
}

bool Laser::ShouldDespawn(uint64 currentTimeMillis)
{
	return currentTimeMillis > despawnTimeMillis;
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
}