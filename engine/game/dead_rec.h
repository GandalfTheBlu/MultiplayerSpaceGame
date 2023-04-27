#pragma once
#include "glm.hpp"

namespace Game
{
struct DeadRecBody
{
	DeadRecBody(float _serverDeltaTime);
	~DeadRecBody();

	struct Body
	{
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 acceleration;
		glm::quat orientation;
	};

	float serverDeltaTime;
	float timeSinceLastUpdate;
	uint64 timeStamp;
	Body clientAtT0;
	Body serverAtT0;

	void SetDataFromServer(const Body& newServerData, bool hardReset, uint64 _timeStamp);
	Body Interpolate(float deltaTime);
};
}