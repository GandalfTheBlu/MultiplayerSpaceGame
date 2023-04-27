#include "config.h"
#include "dead_rec.h"

namespace Game
{
DeadRecBody::DeadRecBody(float _serverDeltaTime)
{
	serverDeltaTime = _serverDeltaTime;
	timeSinceLastUpdate = 0.f;
	timeStamp = 0;
	clientAtT0 = { glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), glm::identity<glm::quat>() };
	serverAtT0 = {glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), glm::identity<glm::quat>()};
}

DeadRecBody::~DeadRecBody() {}

void DeadRecBody::SetDataFromServer(const Body& newServerData, bool hardReset, uint64 _timeStamp)
{
	if (_timeStamp < timeStamp)
		return;

	timeStamp = _timeStamp;

	if (hardReset)
	{
		clientAtT0 = newServerData;
	}
	else
	{
		// set client start body to current interpolated body
		clientAtT0 = Interpolate(0.f);
	}

	// reset the server body
	timeSinceLastUpdate = 0.f;
	serverAtT0 = newServerData;
}

DeadRecBody::Body DeadRecBody::Interpolate(float deltaTime)
{
	timeSinceLastUpdate += deltaTime;
	timeSinceLastUpdate = timeSinceLastUpdate > serverDeltaTime ? serverDeltaTime : timeSinceLastUpdate;

	float normT = timeSinceLastUpdate / serverDeltaTime;// range: 0->1

	glm::vec3 velBlend = clientAtT0.velocity + (serverAtT0.velocity - clientAtT0.velocity) * normT;
	glm::vec3 halfAt2 = serverAtT0.acceleration * (timeSinceLastUpdate * timeSinceLastUpdate);
	glm::vec3 posBlendClient = clientAtT0.position + velBlend * timeSinceLastUpdate + halfAt2;
	glm::vec3 posBlendServer = serverAtT0.position + serverAtT0.velocity * timeSinceLastUpdate + halfAt2;
	glm::vec3 posBlend = posBlendClient + (posBlendServer - posBlendClient) * normT;

	glm::quat oriBlend = glm::mix(clientAtT0.orientation, serverAtT0.orientation, normT);

	return {
		posBlend,
		velBlend,
		serverAtT0.acceleration,
		oriBlend
	};
}
}