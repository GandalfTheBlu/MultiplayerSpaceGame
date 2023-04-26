#pragma once

#include "core/app.h"
#include "render/window.h"
#include "render/model.h"
#include "render/physics.h"
#include "game/console.h"
#include "game/network.h"
#include "game/spaceship.h"
#include "game/laser.h"
#include <vector>
#include "../build/generated/flat/proto.h"// include directory doesn't want to work in cmake :(
#include <unordered_map>

class ServerApp : public Core::App
{
public:
	ServerApp();
	~ServerApp();

	bool Open();
	void Run();
	void Exit();

	void OnClientConnect(ENetPeer* client);
	void OnClientDisconnect(ENetPeer* client);

private:
	void RenderUI();
	void UpdateNetwork();
	void HandleMessage_Input(ENetPeer* sender, const Protocol::PacketWrapper* packet);
	void HandleMessage_Text(ENetPeer* sender, const Protocol::PacketWrapper* packet);

	void InitSpawnPoints();
	void SpawnSpaceShip(ENetPeer* client);
	void UpdateSpaceShipData(ENetPeer* client);
	void DespawnSpaceShip(ENetPeer* client);
	void RespawnSpaceShip(ENetPeer* client);
	void UpdateAndDrawSpaceShips(float deltaTime);

	void SpawnLaser(const glm::vec3& origin, const glm::quat& orientation, uint32 spaceShipId, uint64 currentTimeMillis);
	void DespawnLaser(size_t index);
	void UpdateAndDrawLasers(uint64 currentTimeMillis);


	Display::Window* window;
	Console* console;
	Game::Server* server;
	uint64 currentTimeMillis;

	std::vector<std::tuple<Render::ModelId, Physics::ColliderId, glm::mat4>> asteroids;

	std::unordered_map<ENetPeer*, Game::SpaceShip*> spaceShips;
	Render::ModelId spaceShipModel;
	uint32 nextSpaceShipId;
	std::vector<glm::vec3> spawnPoints;
	float spaceShipCollisionRadiusSquared;

	std::vector<Game::Laser*> lasers;
	Render::ModelId laserModel;
	uint32 nextLaserId;
	float laserMaxTime;
	float laserSpeed;
};