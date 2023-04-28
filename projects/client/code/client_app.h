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

class ClientApp : public Core::App
{
public:
	ClientApp();
	~ClientApp();

	bool Open();
	void Run();
	void Exit();

	void OnServerConnect();
	void OnServerDisconnect();

private:
	// update functions
	void RenderUI();
	void UpdateNetwork();
	void TryGetControlledSpaceShip();
	void UpdateAndDrawSpaceShips(float deltaTime);
	void UpdateAndDrawLasers();

	// unpack messages from server
	void UnpackPlayer(const Protocol::Player* player, glm::vec3& position, glm::vec3& velocity, glm::vec3& acceleration, glm::quat& orientation, uint32& id);
	void UnpackLaser(const Protocol::Laser* laser, glm::vec3& origin, glm::quat& orientation, uint64& spawnTime, uint32& id);
	void HandleMessage_ClientConnect(const Protocol::PacketWrapper* packet);
	void HandleMessage_GameState(const Protocol::PacketWrapper* packet);
	void HandleMessage_SpawnPlayer(const Protocol::PacketWrapper* packet);
	void HandleMessage_DespawnPlayer(const Protocol::PacketWrapper* packet);
	void HandleMessage_UpdatePlayer(const Protocol::PacketWrapper* packet);
	void HandleMessage_TeleportPlayer(const Protocol::PacketWrapper* packet);
	void HandleMessage_SpawnLaser(const Protocol::PacketWrapper* packet);
	void HandleMessage_DespawnLaser(const Protocol::PacketWrapper* packet);
	void HandleMessage_Text(const Protocol::PacketWrapper* packet);

	// utility functions
	unsigned short CompressInputData(const Game::InputData& data);
	Game::InputData GetInputData();
	size_t SpaceShipIndex(uint32 spaceShipId);
	size_t LaserIndex(uint32 laserId);

	// operations in response to server messages
	void SpawnSpaceShip(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& velocity, uint32 spaceShipId);
	void DespawnSpaceShip(uint32 spaceShipId);
	void UpdateSpaceShipData(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& acceleration, const glm::quat& orientation, uint32 spaceShipId, bool hardReset, uint64 timeStamp);
	void SpawnLaser(const glm::vec3& origin, const glm::quat& orientation, uint32 spaceShipId, uint64 spawnTimeMillis, uint32 laserId);
	void DespawnLaser(uint32 laserId);

	Display::Window* window;
	Game::Console* console;
	Game::Client* client;
	uint64 currentTimeMillis;
	uint64 timeDiffMillis;

	std::vector<std::tuple<Render::ModelId, Physics::ColliderId, glm::mat4>> asteroids;

	std::vector<Game::SpaceShip*> spaceShips;
	bool hasReceivedSpaceShip;
	uint32 controlledShipId;
	Game::SpaceShip* controlledShip;
	Render::ModelId spaceShipModel;

	std::vector<Game::Laser*> lasers;
	Render::ModelId laserModel;
	float laserSpeed;
};