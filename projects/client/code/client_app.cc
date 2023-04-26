#include "config.h"
#include "client_app.h"
#include "render/renderdevice.h"
#include "render/cameramanager.h"
#include "render/shaderresource.h"
#include "render/textureresource.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "render/input/inputserver.h"
#include "core/random.h"
#include <chrono>

ClientApp::ClientApp() :
    window(nullptr),
    console(nullptr),
    client(nullptr),
    controlledShipId(0),
    controlledShip(nullptr),
    spaceShipModel(0),
    laserModel(0),
    laserSpeed(0.f)
{}

ClientApp::~ClientApp() {}

bool ClientApp::Open()
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

    // setup client
    if (!Game::InitializeENet())
        return false;

    // setup console commands
    this->console = new Console("console", 128);
    this->console->SetCommand("client", [this](const std::string& arg)
    {
        if (this->client != nullptr)
            return;

        auto connected = [this](ENetPeer* server)
        {
            this->OnServerConnect();
        };

        auto disconnected = [this](ENetPeer* server)
        {
            this->OnServerDisconnect();
        };

        this->client = new Game::Client();
        if (!this->client->Initialize(connected, disconnected) ||
            !this->client->RequestConnectionToServer(arg.c_str(), 1234))
        {
            delete this->client;
            this->client = nullptr;
        }
    });
    this->console->SetCommand("msg", [this](const std::string& arg)
    {
        if (this->client == nullptr || this->client->server == nullptr)
            return;

        this->client->SendData((void*)arg.c_str(), arg.size() + 1, this->client->server);
    });

    // setup space ships and lasers
    this->spaceShipModel = Render::LoadModel("assets/space/spaceship.glb");
    this->laserModel = Render::LoadModel("assets/space/laser.glb");
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

Game::InputData GetInputData()
{
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();
    Game::InputData data;

    data.w = kbd->held[Input::Key::W];
    data.shift = kbd->held[Input::Key::Shift];
    data.left = kbd->held[Input::Key::Left];
    data.right = kbd->held[Input::Key::Right];
    data.up = kbd->held[Input::Key::Up];
    data.down = kbd->held[Input::Key::Down];
    data.a = kbd->held[Input::Key::A];
    data.d = kbd->held[Input::Key::D];
    data.space = kbd->pressed[Input::Key::Space];

    return data;
}

void ClientApp::Run()
{
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    std::clock_t c_start = std::clock();
    double dt = 0.01667f;

    // game loop
    while (this->window->IsOpen())
    {
        auto timeStart = std::chrono::steady_clock::now();
        uint64 currentTimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(timeStart.time_since_epoch()).count();

        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        this->window->Update();

        if (this->controlledShip != nullptr)
        {
            this->controlledShip->CompareAndSetImputData(GetInputData());
            this->controlledShip->FollowThisWithCamera(dt);
        }
        else if (this->spaceShips.size() > 0)
        {
            // wait for the packet with the game state to arrive before trying to control any ship
            this->ControllSpaceShip(this->controlledShipId);
        }

        this->UpdateAndDrawLasers(currentTimeMillis);
        this->UpdateAndDrawSpaceShips(dt);

        if (kbd->pressed[Input::Key::Code::End])
        {
            Render::ShaderResource::ReloadShaders();
        }

        // Store all drawcalls in the render device
        for (auto const& asteroid : this->asteroids)
        {
            Render::RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
        }

        // Execute the entire rendering pipeline
        Render::RenderDevice::Render(this->window, dt);

        this->UpdateNetwork(currentTimeMillis);

        // transfer new frame to window
        this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

        if (kbd->pressed[Input::Key::Code::Escape])
            break;
    }
}

void ClientApp::Exit()
{
    this->window->Close();
    delete this->window;
    delete this->console;
    delete this->client;
}

void ClientApp::OnServerConnect()
{
    printf("server connected\n");
}

void ClientApp::OnServerDisconnect()
{
    printf("server disconnected\n");
}

void ClientApp::RenderUI()
{
    if (this->window->IsOpen())
    {
        this->console->Draw();
        Debug::DispatchDebugTextDrawing();
    }
}

unsigned short CompressInputData(const Game::InputData& data)
{
    return data.w | (data.a << 1) | (data.d << 2) | (data.up << 3) | (data.down << 4) | 
        (data.left << 5) | (data.right << 6) | (data.space << 7) | (data.shift << 8);
}

void ClientApp::UpdateNetwork(uint64 currentTimeMillis)
{
    if (this->client == nullptr)
        return;

    // get input data and send it to server
    unsigned short inputData = CompressInputData(GetInputData());
    flatbuffers::FlatBufferBuilder builder = flatbuffers::FlatBufferBuilder();
    auto outPacket = Protocol::CreateInputC2S(builder, currentTimeMillis, inputData);
    auto packetWrapper = Protocol::CreatePacketWrapper(builder, Protocol::PacketType_InputC2S, outPacket.Union());
    builder.Finish(packetWrapper);

    this->client->SendData(builder.GetBufferPointer(), builder.GetSize(), this->client->server);
    

    // read data from server
    this->client->Update();

    Game::Data d;
    while (this->client->PopDataStack(d))
    {
        // TODO: parse incomming data and call the correct functions on that data
        auto packet = Protocol::GetPacketWrapper(&d.data->front());
        Protocol::PacketType packetType = packet->packet_type();
        switch (packetType)
        {
        case Protocol::PacketType::PacketType_ClientConnectS2C:
            this->HandleMessage_ClientConnect(packet);
            break;
        case Protocol::PacketType::PacketType_GameStateS2C:
            this->HandleMessage_GameState(packet);
            break;
        case Protocol::PacketType::PacketType_SpawnPlayerS2C:
            this->HandleMessage_SpawnPlayer(packet);
            break;
        case Protocol::PacketType::PacketType_DespawnPlayerS2C:
            this->HandleMessage_DespawnPlayer(packet);
            break;
        case Protocol::PacketType::PacketType_UpdatePlayerS2C:
            this->HandleMessage_UpdatePlayer(packet);
            break;
        case Protocol::PacketType::PacketType_TeleportPlayerS2C:
            this->HandleMessage_TeleportPlayer(packet);
            break;
        case Protocol::PacketType::PacketType_SpawnLaserS2C:
            this->HandleMessage_SpawnLaser(packet);
            break;
        case Protocol::PacketType::PacketType_DespawnLaserS2C:
            this->HandleMessage_DespawnLaser(packet);
            break;
        case Protocol::PacketType::PacketType_TextS2C:
            this->HandleMessage_Text(packet);
            break;
        }
    }
}

void ClientApp::HandleMessage_ClientConnect(const Protocol::PacketWrapper* packet)
{
    const Protocol::ClientConnectS2C* inPacket = static_cast<const Protocol::ClientConnectS2C*>(packet->packet());
    this->controlledShipId = inPacket->uuid();
}

void UnpackPlayer(const Protocol::Player* player, glm::vec3& position, glm::quat& orientation, glm::vec3& velocity, uint32& id)
{
    auto& p_pos = player->position();
    auto& p_vel = player->velocity();
    auto& p_dir = player->direction();
    id = player->uuid();

    position = glm::vec3(p_pos.x(), p_pos.y(), p_pos.z());
    orientation = glm::quat(p_dir.x(), p_dir.y(), p_dir.z(), p_dir.w());
    velocity = glm::vec3(p_vel.x(), p_vel.y(), p_vel.z());
}

void UnpackLaser(const Protocol::Laser* laser, glm::vec3& origin, glm::quat& orientation, uint32& spaceShipId, uint64& spawnTime, uint32& id)
{
    auto& p_origin = laser->origin();
    auto& p_dir = laser->direction();
    spawnTime = laser->start_time();
    id = laser->uuid();

    origin = glm::vec3(p_origin.x(), p_origin.y(), p_origin.z());
    orientation = glm::quat(p_dir.x(), p_dir.y(), p_dir.z(), p_dir.w());

    spaceShipId = 0;// temporary (not implemented in protocol yet)
}

void ClientApp::HandleMessage_GameState(const Protocol::PacketWrapper* packet) 
{
    const Protocol::GameStateS2C* inPacket = static_cast<const Protocol::GameStateS2C*>(packet->packet());
    auto p_players = inPacket->players();
    auto p_lasers = inPacket->lasers();

    for (size_t i = 0; i < p_players->size(); i++)
    {
        auto p_player = p_players->operator[](i);
        glm::vec3 position;
        glm::quat orientation;
        glm::vec3 velocity;
        uint32 id;
        UnpackPlayer(p_player, position, orientation, velocity, id);
        this->SpawnSpaceShip(position, orientation, velocity, id);
    }

    for (size_t i = 0; i < p_lasers->size(); i++)
    {
        auto p_laser = p_lasers->operator[](i);
        glm::vec3 origin;
        glm::quat orientation;
        uint32 spaceShipId;
        uint64 spawnTime;
        uint32 id;
        UnpackLaser(p_laser, origin, orientation, spaceShipId, spawnTime, id);
        this->SpawnLaser(origin, orientation, spaceShipId, spawnTime, id);
    }
}

void ClientApp::HandleMessage_SpawnPlayer(const Protocol::PacketWrapper* packet)
{
    const Protocol::SpawnPlayerS2C* inPacket = static_cast<const Protocol::SpawnPlayerS2C*>(packet->packet());
    auto p_player = inPacket->player();
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 velocity;
    uint32 id;
    UnpackPlayer(p_player, position, orientation, velocity, id);

    this->SpawnSpaceShip(position, orientation, velocity, id);
}

void ClientApp::HandleMessage_DespawnPlayer(const Protocol::PacketWrapper* packet)
{
    const Protocol::DespawnPlayerS2C* inPacket = static_cast<const Protocol::DespawnPlayerS2C*>(packet->packet());
    this->DespawnSpaceShip(inPacket->uuid());
}

void ClientApp::HandleMessage_UpdatePlayer(const Protocol::PacketWrapper* packet)
{
    const Protocol::UpdatePlayerS2C* inPacket = static_cast<const Protocol::UpdatePlayerS2C*>(packet->packet());
    auto p_player = inPacket->player();
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 velocity;
    uint32 id;
    UnpackPlayer(p_player, position, orientation, velocity, id);

    // TODO: use dead reckoning
    this->UpdateSpaceShipData(position, orientation, velocity, id);
}

void ClientApp::HandleMessage_TeleportPlayer(const Protocol::PacketWrapper* packet)
{
    const Protocol::TeleportPlayerS2C* inPacket = static_cast<const Protocol::TeleportPlayerS2C*>(packet->packet());
    auto p_player = inPacket->player();
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 velocity;
    uint32 id;
    UnpackPlayer(p_player, position, orientation, velocity, id);

    this->UpdateSpaceShipData(position, orientation, velocity, id);
}

void ClientApp::HandleMessage_SpawnLaser(const Protocol::PacketWrapper* packet)
{
    const Protocol::SpawnLaserS2C* inPacket = static_cast<const Protocol::SpawnLaserS2C*>(packet->packet());
    auto p_laser = inPacket->laser();
    glm::vec3 origin;
    glm::quat orientation;
    uint32 spaceShipId;
    uint64 spawnTime;
    uint32 id;
    UnpackLaser(p_laser, origin, orientation, spaceShipId, spawnTime, id);
    this->SpawnLaser(origin, orientation, spaceShipId, spawnTime, id);
}

void ClientApp::HandleMessage_DespawnLaser(const Protocol::PacketWrapper* packet)
{
    const Protocol::DespawnLaserS2C* inPacket = static_cast<const Protocol::DespawnLaserS2C*>(packet->packet());
    this->DespawnLaser(inPacket->uuid());
}

void ClientApp::HandleMessage_Text(const Protocol::PacketWrapper* packet)
{
    const Protocol::TextS2C* inPacket = static_cast<const Protocol::TextS2C*>(packet->packet());
    printf("[MESSAGE] %s\n", inPacket->text()->c_str());
}

size_t ClientApp::SpaceShipIndex(uint32 spaceShipId)
{
    size_t index = 0;
    for (; index < this->spaceShips.size() && this->spaceShips[index]->id != spaceShipId; index++);

    return index;
}

void ClientApp::ControllSpaceShip(uint32 spaceShipId)
{
    size_t index = this->SpaceShipIndex(spaceShipId);
    if(index < spaceShips.size())
        this->controlledShip = spaceShips[index];
}

void ClientApp::SpawnSpaceShip(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& velocity, uint32 spaceShipId)
{
    Game::SpaceShip* spaceShip = new Game::SpaceShip();
    spaceShip->id = spaceShipId;
    spaceShip->position = position;
    this->spaceShips.push_back(spaceShip);
}

void ClientApp::DespawnSpaceShip(uint32 spaceShipId)
{
    size_t index = this->SpaceShipIndex(spaceShipId);

    delete this->spaceShips[index];
    this->spaceShips.erase(this->spaceShips.begin() + index);
}

void ClientApp::RespawnSpaceShip(const glm::vec3& position, const glm::quat& orientation, uint32 spaceShipId)
{
    size_t index = this->SpaceShipIndex(spaceShipId);

    this->spaceShips[index]->isHitByLaser = false;
    this->spaceShips[index]->position = position;
    this->spaceShips[index]->orientation = orientation;
    this->spaceShips[index]->linearVelocity = glm::vec3(0.f);
}

void ClientApp::UpdateSpaceShipData(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& velocity, uint32 spaceShipId)
{
    size_t index = this->SpaceShipIndex(spaceShipId);

    this->spaceShips[index]->position = position;
    this->spaceShips[index]->orientation = orientation;
    this->spaceShips[index]->linearVelocity = velocity;
}

void ClientApp::UpdateAndDrawSpaceShips(float deltaTime)
{
    for (size_t i = 0; i < this->spaceShips.size(); i++)
    {
        this->spaceShips[i]->Update(deltaTime);
        Render::RenderDevice::Draw(this->spaceShipModel, this->spaceShips[i]->transform);
    }
}

void ClientApp::SpawnLaser(const glm::vec3& origin, const glm::quat& orientation, uint32 spaceShipId, uint64 spawnTimeMillis, uint32 laserId)
{
    Game::Laser* laser = new Game::Laser(origin, orientation, spaceShipId, spawnTimeMillis, laserId);
    this->lasers.push_back(laser);
}

void ClientApp::DespawnLaser(uint32 laserId)
{
    size_t index = 0;
    for (; index < this->lasers.size() && this->lasers[index]->id != laserId; index++);

    delete this->lasers[index];
    this->lasers.erase(this->lasers.begin() + index);
}

void ClientApp::UpdateAndDrawLasers(uint64 currentTimeMillis)
{
    for (size_t i = 0; i < this->lasers.size(); i++)
    {
        Render::RenderDevice::Draw(this->laserModel, this->lasers[i]->GetLocalToWorld(currentTimeMillis, this->laserSpeed));
    }
}