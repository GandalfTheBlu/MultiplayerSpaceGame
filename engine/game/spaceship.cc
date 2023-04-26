#include "config.h"
#include "spaceship.h"
#include "render/input/inputserver.h"
#include "render/cameramanager.h"
#include "render/physics.h"
#include "render/debugrender.h"
#include "render/particlesystem.h"
#include <vector>

using namespace Input;
using namespace glm;
using namespace Render;

namespace Game
{
SpaceShip::SpaceShip()
{
    uint32_t numParticles = 2048;
    this->particleEmitterLeft = new ParticleEmitter(numParticles);
    this->particleEmitterLeft->data = {
        .origin = glm::vec4(this->position + (vec3(this->transform[2]) * emitterOffset),1),
        .dir = glm::vec4(glm::vec3(-this->transform[2]), 0),
        .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
        .endColor = glm::vec4(0,0,0,1.0f),
        .numParticles = numParticles,
        .theta = glm::radians(0.0f),
        .startSpeed = 1.2f,
        .endSpeed = 0.0f,
        .startScale = 0.025f,
        .endScale = 0.0f,
        .decayTime = 2.58f,
        .randomTimeOffsetDist = 2.58f,
        .looping = 1,
        .emitterType = 1,
        .discRadius = 0.020f
    };
    this->particleEmitterRight = new ParticleEmitter(numParticles);
    this->particleEmitterRight->data = this->particleEmitterLeft->data;

    ParticleSystem::Instance()->AddEmitter(this->particleEmitterLeft);
    ParticleSystem::Instance()->AddEmitter(this->particleEmitterRight);
}

SpaceShip::~SpaceShip()
{
    ParticleSystem::Instance()->RemoveEmitter(this->particleEmitterLeft);
    ParticleSystem::Instance()->RemoveEmitter(this->particleEmitterRight);
}

bool
SpaceShip::CheckCollisions()
{
    if (isHitByLaser)
        return true;

    glm::mat4 rotation = (glm::mat4)orientation;
    bool hit = false;
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 pos = position;
        glm::vec3 dir = rotation * glm::vec4(glm::normalize(colliderEndPoints[i]), 0.0f);
        float len = glm::length(colliderEndPoints[i]);
        Physics::RaycastPayload payload = Physics::Raycast(position, dir, len);

        // debug draw collision rays
        // Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

        if (payload.hit)
        {
            Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
            hit = true;
        }
    }

    return hit;
}

void SpaceShip::ControlShip(float dt)
{
    Mouse* mouse = Input::GetDefaultMouse();
    Keyboard* kbd = Input::GetDefaultKeyboard();

    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);

    if (kbd->held[Key::W])
    {
        if (kbd->held[Key::Shift])
            this->currentSpeed = mix(this->currentSpeed, this->boostSpeed, std::min(1.0f, dt * 30.0f));
        else
            this->currentSpeed = mix(this->currentSpeed, this->normalSpeed, std::min(1.0f, dt * 90.0f));
    }
    else
    {
        this->currentSpeed = 0;
    }

    float rotX = kbd->held[Key::Left] ? 1.0f : kbd->held[Key::Right] ? -1.0f : 0.0f;
    float rotY = kbd->held[Key::Up] ? -1.0f : kbd->held[Key::Down] ? 1.0f : 0.0f;
    float rotZ = kbd->held[Key::A] ? -1.0f : kbd->held[Key::D] ? 1.0f : 0.0f;

    const float rotationSpeed = 1.8f * dt;
    rotXSmooth = mix(rotXSmooth, rotX * rotationSpeed, dt * cameraSmoothFactor);
    rotYSmooth = mix(rotYSmooth, rotY * rotationSpeed, dt * cameraSmoothFactor);
    rotZSmooth = mix(rotZSmooth, rotZ * rotationSpeed, dt * cameraSmoothFactor);
    quat localOrientation = quat(vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
    this->orientation = this->orientation * localOrientation;
    this->rotationZ -= rotXSmooth;
    this->rotationZ = clamp(this->rotationZ, -45.0f, 45.0f);
    this->rotationZ = mix(this->rotationZ, 0.0f, dt * cameraSmoothFactor);

    // update camera view transform
    vec3 desiredCamPos = this->position + vec3(this->transform * vec4(0, camOffsetY, -4.0f, 0));
    this->camPos = mix(this->camPos, desiredCamPos, dt * cameraSmoothFactor);
    cam->view = lookAt(this->camPos, this->camPos + vec3(this->transform[2]), vec3(this->transform[1]));
}

void
SpaceShip::Update(float dt)
{
    vec3 desiredVelocity = vec3(0, 0, this->currentSpeed);
    desiredVelocity = this->transform * vec4(desiredVelocity, 0.0f);

    this->linearVelocity = mix(this->linearVelocity, desiredVelocity, dt * accelerationFactor);
    this->position += this->linearVelocity * dt * 10.0f;

    mat4 T = translate(this->position) * (mat4)this->orientation;
    this->transform = T * (mat4)quat(vec3(0, 0, rotationZ));

    const float thrusterPosOffset = 0.365f;
    this->particleEmitterLeft->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * -thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
    this->particleEmitterLeft->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);
    this->particleEmitterRight->data.origin = glm::vec4(vec3(this->position + (vec3(this->transform[0]) * thrusterPosOffset)) + (vec3(this->transform[2]) * emitterOffset), 1);
    this->particleEmitterRight->data.dir = glm::vec4(glm::vec3(-this->transform[2]), 0);
    
    float t = (currentSpeed / this->normalSpeed);
    this->particleEmitterLeft->data.startSpeed = 1.2 + (3.0f * t);
    this->particleEmitterLeft->data.endSpeed = 0.0f  + (3.0f * t);
    this->particleEmitterRight->data.startSpeed = 1.2 + (3.0f * t);
    this->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * t);
    //this->particleEmitter->data.decayTime = 0.16f;//+ (0.01f  * t);
    //this->particleEmitter->data.randomTimeOffsetDist = 0.06f;/// +(0.01f * t);
}
}

namespace SpaceShips
{
    int nextId = 0;
    std::vector<std::shared_ptr<Game::SpaceShip>> spaceShips;
    float shipCollisionRadiusSquared = 5.f * 5.f;
    glm::vec3 spawnPoints[32];

    void InitSpawnPoints()
    {
        float radius = 100.f;
        for (int i = 0; i < 32; i++)
        {
            float angle = (float)i / 33.f * 3.1415f * 2.f;
            spawnPoints[i] = glm::vec3(
                radius * glm::cos(angle),
                0.f,
                radius * glm::sin(angle)
            );
        }
    }

    std::shared_ptr<Game::SpaceShip> SpawnSpaceShip(Render::ModelId model)
    {
        std::shared_ptr<Game::SpaceShip> spaceShip = std::make_shared<Game::SpaceShip>();
        spaceShip->id = nextId;
        spaceShip->model = model;
        spaceShip->position = spawnPoints[nextId % 32];
        spaceShips.push_back(spaceShip);
        nextId++;

        return spaceShip;
    }

    void DespawnSpaceShip(size_t index)
    {
        if (index < spaceShips.size())
            spaceShips.erase(spaceShips.begin() + index);
    }

    void RegisterHitOnShip(size_t index)
    {
        if (index < spaceShips.size())
            spaceShips[index]->isHitByLaser = true;
    }

    void RespawnShip(size_t index)
    {
        if (index < spaceShips.size())
        {
            spaceShips[index]->isHitByLaser = false;
            spaceShips[index]->position = spawnPoints[nextId % 32];
            spaceShips[index]->linearVelocity = glm::vec3(0.f);
            nextId++;
        }
    }

    void UpdateAndDrawSpaceShips(float deltaTime)
    {
        for (size_t i=0; i<spaceShips.size(); i++)
        {
            spaceShips[i]->Update(deltaTime);

            if (spaceShips[i]->CheckCollisions())
            {
                RespawnShip(i);
            }

            RenderDevice::Draw(spaceShips[i]->model, spaceShips[i]->transform);
        }
    }
}