#pragma once
#include "render/model.h"
#include <memory>

namespace Render
{
    struct ParticleEmitter;
}

namespace Game
{

struct SpaceShip
{
    SpaceShip();
    ~SpaceShip();
    
    glm::vec3 position = glm::vec3(0);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
    glm::mat4 transform = glm::mat4(1);
    glm::vec3 linearVelocity = glm::vec3(0);

    const float normalSpeed = 1.0f;
    const float boostSpeed = normalSpeed * 2.0f;
    const float accelerationFactor = 1.0f;
    const float camOffsetY = 1.0f;
    const float cameraSmoothFactor = 10.0f;

    float currentSpeed = 0.0f;

    float rotationZ = 0;
    float rotXSmooth = 0;
    float rotYSmooth = 0;
    float rotZSmooth = 0;

    Render::ModelId model;
    Render::ParticleEmitter* particleEmitterLeft;
    Render::ParticleEmitter* particleEmitterRight;
    float emitterOffset = -0.5f;

    int id = 0;
    bool isHitByLaser = false;

    bool CheckCollisions();
    void ControlShip(float dt);
    void Update(float dt);
    
    const glm::vec3 colliderEndPoints[8] = {
        glm::vec3(-1.10657, -0.480347, -0.346542),  // right wing
        glm::vec3(1.10657, -0.480347, -0.346542),  // left wing
        glm::vec3(-0.342382, 0.25109, -0.010299),   // right top
        glm::vec3(0.342382, 0.25109, -0.010299),   // left top
        glm::vec3(-0.285614, -0.10917, 0.869609), // right front
        glm::vec3(0.285614, -0.10917, 0.869609), // left front
        glm::vec3(-0.279064, -0.10917, -0.98846),   // right back
        glm::vec3(0.279064, -0.10917, -0.98846)   // right back
    };
};

}

namespace SpaceShips
{
    extern int nextId;
    extern std::vector<std::shared_ptr<Game::SpaceShip>> spaceShips;
    extern float shipCollisionRadiusSquared;
    extern glm::vec3 spawnPoints[32];

    void InitSpawnPoints();
    std::shared_ptr<Game::SpaceShip> SpawnSpaceShip(Render::ModelId model);
    void DespawnSpaceShip(size_t index);
    void RegisterHitOnShip(size_t index);
    void RespawnShip(size_t index);
    void UpdateAndDrawSpaceShips(float deltaTime);
}