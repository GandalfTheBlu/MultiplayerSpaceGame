#include "config.h"
#include "server_app.h"
#include "render/renderdevice.h"
#include "render/cameramanager.h"
#include "render/shaderresource.h"
#include "render/textureresource.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "render/input/inputserver.h"
#include "core/random.h"
#include <chrono>

ServerApp::ServerApp():
	window(nullptr),
	console(nullptr),
	server(nullptr),
    currentTimeMillis(0),
    spaceShipModel(0),
    nextSpaceShipId(0),
    spaceShipCollisionRadiusSquared(0.f),
    laserModel(0),
    nextLaserId(0),
    laserMaxTime(0.f),
    laserSpeed(0.f),
    spectate(false),
    spectateIndex(0)
{}

ServerApp::~ServerApp(){}

bool ServerApp::Open()
{
    int width = 1000; 
    int height = 800;

	// setup window and rendering
	App::Open();
	this->window = new Display::Window;
	this->window->SetSize(width, height);

	if (!this->window->Open())
		return false;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	Render::RenderDevice::Init();

	this->window->SetUiRender([this]()
	{
		this->RenderUI();
	});

    // setup main camera
    Render::Camera* cam = Render::CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = glm::perspective(glm::radians(90.0f), float(width) / float(height), 0.01f, 1000.f);
    cam->view = glm::lookAt(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	// setup server
	if (!Game::InitializeENet())
		return false;

	// setup console commands
    this->console = new Console("console", 128);
	this->console->SetCommand("server", [this](const std::string& arg)
	{
        if (this->server != nullptr)
            return;

        auto connected = [this](ENetPeer* client)
        {
            this->OnClientConnect(client);
        };

        auto disconnected = [this](ENetPeer* client)
        {
            this->OnClientDisconnect(client);
        };

        this->server = new Game::Server();
        if (!this->server->Initialize(arg.c_str(), 1234, connected, disconnected))
        {
            delete this->server;
            this->server = nullptr;
        }
	});

	// setup space ships and lasers
	this->InitSpawnPoints();
    this->spaceShipModel = Render::LoadModel("assets/space/spaceship.glb");
    this->spaceShipCollisionRadiusSquared = 5.f * 5.f;
    this->laserModel = Render::LoadModel("assets/space/laser.glb");
    this->laserMaxTime = 2.f;
    this->laserSpeed = 20.f;

    // load all resources
    Render::ModelId models[6] = {
        Render::LoadModel("assets/space/Asteroid_1.glb"),
        Render::LoadModel("assets/space/Asteroid_2.glb"),
        Render::LoadModel("assets/space/Asteroid_3.glb"),
        Render::LoadModel("assets/space/Asteroid_4.glb"),
        Render::LoadModel("assets/space/Asteroid_5.glb"),
        Render::LoadModel("assets/space/Asteroid_6.glb")
    };
    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };

    // setup asteroids near
    for (int i = 0; i < 100; i++)
    {
        std::tuple<Render::ModelId, Physics::ColliderId, glm::mat4> asteroid;
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        std::get<0>(asteroid) = models[resourceIndex];
        float span = 20.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
        std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
        std::get<2>(asteroid) = transform;
        asteroids.push_back(asteroid);
    }

    // setup asteroids far
    for (int i = 0; i < 50; i++)
    {
        std::tuple<Render::ModelId, Physics::ColliderId, glm::mat4> asteroid;
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        std::get<0>(asteroid) = models[resourceIndex];
        float span = 80.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);
        std::get<1>(asteroid) = Physics::CreateCollider(colliderMeshes[resourceIndex], transform);
        std::get<2>(asteroid) = transform;
        asteroids.push_back(asteroid);
    }

    // setup skybox
    std::vector<const char*> skybox
    {
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png"
    };
    Render::TextureResourceId skyboxId = Render::TextureResource::LoadCubemap("skybox", skybox, true);
    Render::RenderDevice::SetSkybox(skyboxId);

    // setup lights
    const int numLights = 40;
    for (int i = 0; i < numLights; i++)
    {
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f
        );
        glm::vec3 color = glm::vec3(
            Core::RandomFloat(),
            Core::RandomFloat(),
            Core::RandomFloat()
        );
        Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }

	return true;
}

void ServerApp::Run()
{
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    std::clock_t c_start = std::clock();
    double dt = 0.01667f;

    // game loop
    while (this->window->IsOpen())
    {
        auto timeStart = std::chrono::steady_clock::now();
        this->currentTimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(timeStart.time_since_epoch()).count();

        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        this->window->Update();

        this->UpdateAndDrawLasers(this->currentTimeMillis);
        this->UpdateAndDrawSpaceShips(dt);
        this->UpdateNetwork();

        if (kbd->pressed[Input::Key::Code::End])
        {
            Render::ShaderResource::ReloadShaders();
        }
        if (kbd->pressed[Input::Key::S])
        {
            this->spectate = !this->spectate;
        }

        // spectating
        size_t nShips = this->spaceShips.size();
        if (this->spectate && nShips > 0)
        {
            if (kbd->pressed[Input::Key::Left])
                this->spectateIndex = (nShips + this->spectateIndex - 1) % nShips;
            else if (kbd->pressed[Input::Key::Right])
                this->spectateIndex = (this->spectateIndex + 1) % nShips;

            size_t i = 0;
            Game::SpaceShip* spectatedShip = nullptr;
            // this must be the ugliest solution to this problem ever (getting map-element by index)
            for (auto& spaceShip : this->spaceShips)
            {
                if (i == this->spectateIndex)
                {
                    spectatedShip = spaceShip.second;
                    break;
                }
                i++;
            }

            spectatedShip->FollowThisWithCamera(dt);
        }
        else
        {
            this->spectate = false;
            this->spectateIndex = 0;

            Render::Camera* cam = Render::CameraManager::GetCamera(CAMERA_MAIN);
            cam->view = glm::lookAt(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
        }

        // Store all drawcalls in the render device
        for (auto const& asteroid : this->asteroids)
        {
            Render::RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
        }

        // Execute the entire rendering pipeline
        Render::RenderDevice::Render(this->window, dt);

        // transfer new frame to window
        this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

        if (kbd->pressed[Input::Key::Code::Escape])
            break;
    }
}

void ServerApp::Exit()
{
    for (auto& spaceShip : this->spaceShips)
        delete spaceShip.second;

    for (size_t i = 0; i < this->lasers.size(); i++)
        delete this->lasers[i];

    this->window->Close();
    delete this->window;
    delete this->console;
    delete this->server;
}

void ServerApp::OnClientConnect(ENetPeer* client)
{
    printf("client connected\n");
    this->SpawnSpaceShip(client);
    this->SendGameState(client);
    this->SendClientConnect(client);
}

void ServerApp::OnClientDisconnect(ENetPeer* client)
{
    printf("client disconnected\n");
    this->DespawnSpaceShip(client);
}

void ServerApp::RenderUI()
{
    if (this->window->IsOpen())
    {
        this->console->Draw();
        Debug::DispatchDebugTextDrawing();
    }
}

void ServerApp::UpdateNetwork()
{
    if (this->server == nullptr)
        return;

    this->server->Update();

    Game::Data d;
    while (this->server->PopDataStack(d))
    {
        auto packet = Protocol::GetPacketWrapper(&d.data->front());
        Protocol::PacketType packetType = packet->packet_type();
        switch (packetType)
        {
        case Protocol::PacketType::PacketType_InputC2S:
            this->HandleMessage_Input(d.sender, packet);
            break;
        case Protocol::PacketType::PacketType_TextC2S:
            this->HandleMessage_Text(d.sender, packet);
            break;
        }
    }
}

void ServerApp::HandleMessage_Input(ENetPeer* sender, const Protocol::PacketWrapper* packet)
{
    if (this->spaceShips.count(sender) == 0)
        return;

    const Protocol::InputC2S* inPacket = static_cast<const Protocol::InputC2S*>(packet->packet());

    unsigned short inputData = inPacket->bitmap();

    Game::InputData data;
    data.w = inputData & 1;
    data.a = inputData & 2;
    data.d = inputData & 4;
    data.up = inputData & 8;
    data.down = inputData & 16;
    data.left = inputData & 32;
    data.right = inputData & 64;
    data.space = inputData & 128;
    data.shift = inputData & 256;
    data.timeStamp = inPacket->time();

    this->spaceShips[sender]->CompareAndSetImputData(data);
}

void ServerApp::HandleMessage_Text(ENetPeer* sender, const Protocol::PacketWrapper* packet)
{
    const Protocol::TextC2S* inPacket = static_cast<const Protocol::TextC2S*>(packet->packet());

    // print incomming text
    printf("[MESSAGE] %s\n", inPacket->text()->c_str());

    // send text to all others (exluding sender)
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    auto text = builder.CreateString(inPacket->text()->c_str());
    auto outPacket = Protocol::CreateTextC2S(builder, text);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_TextC2S, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize(), sender);
}

void ServerApp::InitSpawnPoints()
{
    float radius = 100.f;
    for (int i = 0; i < 32; i++)
    {
        float angle = (float)i / 33.f * 3.1415f * 2.f;
        spawnPoints.push_back(glm::vec3(
            radius * glm::cos(angle),
            0.f,
            radius * glm::sin(angle)
        ));
    }
}

void PackPlayer(Game::SpaceShip* spaceShip, Protocol::Player& p_player)
{
    auto p_position = Protocol::Vec3(spaceShip->position.x, spaceShip->position.y, spaceShip->position.z);
    auto p_velocity = Protocol::Vec3(spaceShip->linearVelocity.x, spaceShip->linearVelocity.y, spaceShip->linearVelocity.z);
    auto p_acceleration = Protocol::Vec3(0.f, 0.f, 0.f);
    auto p_orientation = Protocol::Vec4(spaceShip->orientation.x, spaceShip->orientation.y, spaceShip->orientation.z, spaceShip->orientation.w);
    p_player = Protocol::Player(spaceShip->id, p_position, p_velocity, p_acceleration, p_orientation);
}

void PackLaser(Game::Laser* laser, Protocol::Laser& p_laser)
{
    auto p_origin = Protocol::Vec3(laser->origin.x, laser->origin.y, laser->origin.z);
    auto p_orientation = Protocol::Vec4(laser->orientation.x, laser->orientation.y, laser->orientation.z, laser->orientation.w);
    p_laser = Protocol::Laser(laser->id, laser->spawnTimeMillis, 0, p_origin, p_orientation);
}

void ServerApp::SpawnSpaceShip(ENetPeer* client)
{
    static size_t spawnIndex = 0;
    Game::SpaceShip* spaceShip = new Game::SpaceShip();
    spaceShip->id = this->nextSpaceShipId;
    spaceShip->position = this->spawnPoints[spawnIndex++ % 32];
    this->spaceShips[client] = spaceShip;
    this->nextSpaceShipId++;

    // send messages to others
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    Protocol::Player p_player;
    PackPlayer(spaceShip, p_player);
    auto outPacket = Protocol::CreateSpawnPlayerS2C(builder, &p_player);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_SpawnPlayerS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    // exlude client, since it will receive everything in the gameState-message
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize(), client);
}

void ServerApp::UpdateSpaceShipData(ENetPeer* client)
{
    Game::SpaceShip* spaceShip = spaceShips[client];

    // send messages to others
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    Protocol::Player p_player;
    PackPlayer(spaceShip, p_player);
    auto outPacket = Protocol::CreateUpdatePlayerS2C(builder, this->currentTimeMillis, &p_player);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_UpdatePlayerS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize());
}

void ServerApp::DespawnSpaceShip(ENetPeer* client)
{
    uint32 id = this->spaceShips[client]->id;
    delete this->spaceShips[client];
    this->spaceShips.erase(client);

    // send message to others (exluding the sender, since they are not connected any more)
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    auto outPacket = Protocol::CreateDespawnPlayerS2C(builder, id);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_DespawnPlayerS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize(), client);
}

void ServerApp::RespawnSpaceShip(ENetPeer* client)
{
    static size_t spawnIndex = 16;
    Game::SpaceShip* spaceShip = this->spaceShips[client];
    spaceShip->isHitByLaser = false;
    spaceShip->position = this->spawnPoints[spawnIndex++ % 32];
    spaceShip->orientation = glm::identity<glm::quat>();
    spaceShip->linearVelocity = glm::vec3(0.f);
    this->nextSpaceShipId++;

    // send message to others
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    Protocol::Player p_player;
    PackPlayer(spaceShip, p_player);
    auto outPacket = Protocol::CreateTeleportPlayerS2C(builder, this->currentTimeMillis, &p_player);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_TeleportPlayerS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize());
}

void ServerApp::UpdateAndDrawSpaceShips(float deltaTime)
{
    for (auto& spaceShip : this->spaceShips)
    {
        if (spaceShip.second->inputData.space)
        {
            this->SpawnLaser(spaceShip.second->position, spaceShip.second->orientation, 
                spaceShip.second->id, this->currentTimeMillis);
        }

        spaceShip.second->Update(deltaTime);

        if (spaceShip.second->CheckCollisions())
        {
            this->RespawnSpaceShip(spaceShip.first);
        }
        else
        {
            this->UpdateSpaceShipData(spaceShip.first);
        }

        Render::RenderDevice::Draw(this->spaceShipModel, spaceShip.second->transform);
    }
}

void ServerApp::SendGameState(ENetPeer* client)
{
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    
    std::vector<Protocol::Player> p_players;
    for (auto& spaceShip : this->spaceShips)
    {
        Protocol::Player p_player;
        PackPlayer(spaceShip.second, p_player);
        p_players.push_back(p_player);
    }

    std::vector<Protocol::Laser> p_lasers;
    for (auto& laser : this->lasers)
    {
        Protocol::Laser p_laser;
        PackLaser(laser, p_laser);
        p_lasers.push_back(p_laser);
    }

    auto outPacket = Protocol::CreateGameStateS2CDirect(builder, &p_players, &p_lasers);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_GameStateS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->SendData(builder.GetBufferPointer(), builder.GetSize(), client);
}

void ServerApp::SendClientConnect(ENetPeer* client)
{
    uint32 id = spaceShips[client]->id;
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    auto outPacket = Protocol::CreateClientConnectS2C(builder, id);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_ClientConnectS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->SendData(builder.GetBufferPointer(), builder.GetSize(), client);
}

void ServerApp::SpawnLaser(const glm::vec3& origin, const glm::quat& orientation, uint32 spaceShipId, uint64 currentTimeMillis)
{
    Game::Laser* laser = new Game::Laser(origin, orientation, spaceShipId, currentTimeMillis, this->nextLaserId);
    this->lasers.push_back(laser);
    this->nextLaserId++;

    // send message to others
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    Protocol::Laser p_laser;
    PackLaser(laser, p_laser);
    auto outPacket = Protocol::CreateSpawnLaserS2C(builder, &p_laser);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_SpawnLaserS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize());
}

void ServerApp::DespawnLaser(size_t index)
{
    uint32 id = this->lasers[index]->id;
    delete this->lasers[index];
    this->lasers.erase(this->lasers.begin() + index);
    
    // send message to others
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    auto outPacket = Protocol::CreateDespawnLaserS2C(builder, id);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_DespawnLaserS2C, outPacket.Union());
    builder.Finish(packetWrapper);
    this->server->BroadcastData(builder.GetBufferPointer(), builder.GetSize());
}

void ServerApp::UpdateAndDrawLasers(uint64 currentTimeMillis)
{
    for (int i = (int)this->lasers.size() - 1; i >= 0; i--)
    {
        // check timeout
        if (this->lasers[i]->GetSecondsAlive(currentTimeMillis) >= this->laserMaxTime)
        {
            this->DespawnLaser(i);
            continue;
        }

        // check space ship collision
        bool hitShip = false;
        glm::vec3 currentPos = this->lasers[i]->GetCurrentPosition(currentTimeMillis, this->laserSpeed);
        for (auto& spaceShip : this->spaceShips)
        {
            if (spaceShip.second->id == this->lasers[i]->spaceShipId)// ignore the ship it was fired from
                continue;

            glm::vec3 diff = currentPos - spaceShip.second->position;
            if (glm::dot(diff, diff) < this->spaceShipCollisionRadiusSquared)
            {
                spaceShip.second->isHitByLaser = true;
                hitShip = true;
                break;
            }
        }

        if (hitShip)
            continue;

        // check asteroid collision
        glm::vec3 direction = this->lasers[i]->GetDirection();
        float maxDist = 1.f;

        Physics::RaycastPayload raycastResult = Physics::Raycast(currentPos, direction, maxDist);
        if (raycastResult.hit)
        {
            this->DespawnLaser(i);
            continue;
        }

        // draw
        Render::RenderDevice::Draw(this->laserModel, this->lasers[i]->GetLocalToWorld(currentTimeMillis, this->laserSpeed));
    }
}