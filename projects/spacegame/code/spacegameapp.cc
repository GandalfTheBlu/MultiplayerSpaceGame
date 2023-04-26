//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include "render/physics.h"
#include <chrono>
#include "game/laser.h"

using namespace Display;
using namespace Render;

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::SpaceGameApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::~SpaceGameApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/

void ClientConnected(Host* self, ENetPeer* client)
{
    printf("client connected!\n");
}

void ClientDisconnected(Host* self, ENetPeer* client)
{
    printf("client disconnected!\n");
}

void ServerConnected(Host* self, ENetPeer* server)
{
    printf("server connected!\n");
}

void ServerDisconnected(Host* self, ENetPeer* server)
{
    printf("server disconnected!\n");
}

bool
SpaceGameApp::Open()
{
	App::Open();
	this->window = new Display::Window;
    this->window->SetSize(600, 450);
    if (!this->window->Open())
        return false;

    // set clear color to gray
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    RenderDevice::Init();

    // set ui rendering function
    this->window->SetUiRender([this]()
    {
        this->RenderUI();
    });

    // setup console commands
    InitializeENet();
    this->host = nullptr;

    this->console = new Console("console", 128);
    this->console->SetCommand("server", [this](const std::string& arg) 
    {
        if (this->host != nullptr)
            return;

        Server* server = new Server();
        if (!server->Initialize(arg.c_str(), 1234, ClientConnected, ClientDisconnected))
        {
            delete server;
            return;
        }
        this->host = server;
    });
    this->console->SetCommand("client", [this](const std::string& arg)
    {
        if (this->host != nullptr)
            return;

        Client* client = new Client();
        if (!client->Initialize(ServerConnected, ServerDisconnected) || 
            !client->RequestConnectionToServer(arg.c_str(), 1234))
        {
            delete client;
            return;
        }
        this->host = client;
    });
    this->console->SetCommand("msg", [this](const std::string& arg) 
    {
        if (this->host == nullptr || this->host->type != HostType::Client)
            return;

        Client* client = dynamic_cast<Client*>(this->host);
        if (client->server == nullptr)
            return;

        client->SendData((void*)arg.c_str(), arg.size()+1, client->server);
    });

    // setup ships
    SpaceShips::InitSpawnPoints();
    ModelId shipModelId = LoadModel("assets/space/spaceship.glb");
    this->ship = SpaceShips::SpawnSpaceShip(shipModelId);
    SpaceShips::SpawnSpaceShip(shipModelId);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Run()
{
    int w;
    int h;
    this->window->GetSize(w, h);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = projection;

    // load all resources
    ModelId models[6] = {
        LoadModel("assets/space/Asteroid_1.glb"),
        LoadModel("assets/space/Asteroid_2.glb"),
        LoadModel("assets/space/Asteroid_3.glb"),
        LoadModel("assets/space/Asteroid_4.glb"),
        LoadModel("assets/space/Asteroid_5.glb"),
        LoadModel("assets/space/Asteroid_6.glb")
    };
    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };

    std::vector<std::tuple<ModelId, Physics::ColliderId, glm::mat4>> asteroids;
    
    // Setup asteroids near
    for (int i = 0; i < 100; i++)
    {
        std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
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

    // Setup asteroids far
    for (int i = 0; i < 50; i++)
    {
        std::tuple<ModelId, Physics::ColliderId, glm::mat4> asteroid;
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

    // Setup skybox
    std::vector<const char*> skybox
    {
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png"
    };
    TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
    RenderDevice::SetSkybox(skyboxId);
    
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    const int numLights = 40;
    Render::PointLightId lights[numLights];
    // Setup lights
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
        lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }

    ModelId laserId = LoadModel("assets/space/laser.glb");

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
        this->UpdateHost();

        if (kbd->pressed[Input::Key::Code::End])
        {
            ShaderResource::ReloadShaders();
        }
        if (kbd->pressed[Input::Key::Code::Space])
        {
            Lasers::AddLaser(Laser(*this->ship.get(), currentTimeMillis, laserId));
        }

        ship->ControlShip(dt);

        // Store all drawcalls in the render device
        for (auto const& asteroid : asteroids)
        {
            RenderDevice::Draw(std::get<0>(asteroid), std::get<2>(asteroid));
        }

        Lasers::UpdateAndDrawLasers(currentTimeMillis);
        SpaceShips::UpdateAndDrawSpaceShips(dt);

        // Execute the entire rendering pipeline
        RenderDevice::Render(this->window, dt);

		// transfer new frame to window
		this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

        if (kbd->pressed[Input::Key::Code::Escape])
            break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Exit()
{
    SpaceShips::spaceShips.clear();

    this->window->Close();
    delete this->window;
    delete this->console;
    delete this->host;
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::RenderUI()
{
	if (this->window->IsOpen())
	{
        /*ImGui::Begin("Debug");

        Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
        int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
        if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
            Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
        
        Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
        int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
        if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
            Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);

        ImGui::End();*/

        this->console->Draw();

        Debug::DispatchDebugTextDrawing();
	}
}

void SpaceGameApp::UpdateHost()
{
    if (this->host == nullptr)
        return;

    this->host->Update();

    struct Protocol
    {
        glm::vec3 pos;
        glm::vec3 vel;
    };

    if (this->host->type == HostType::Client)
    {
        Client* client = dynamic_cast<Client*>(this->host);
        if (client->server == nullptr)
            return;

        Protocol data
        {
            this->ship->position,
            this->ship->linearVelocity
        };

        client->SendData((void*)&data, sizeof(Protocol), client->server);
    }
    else
    {
        Data d;
        while (this->host->PopDataStack(d))
        {
            /*for (auto& c : *d.data.get())
            {
                printf("%c", (char)c);
            }*/

            Protocol data = *(Protocol*)(&d.data->front());
            printf("pos: (%f, %f, %f), vel: (%f, %f, %f)\n", data.pos.x, data.pos.y, data.pos.z, data.vel.x, data.vel.y, data.vel.z);
        }
    }
}

} // namespace Game