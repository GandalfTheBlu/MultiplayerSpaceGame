#pragma once
#include "dead_rec.h"

namespace Render
{
    struct ParticleEmitter;
}

namespace Game
{

struct InputData
{
    bool w, a, d, up, down, left, right, space, shift;
    uint64 timeStamp;

    InputData();
};

struct SpaceShip
{
    SpaceShip();
    ~SpaceShip();
    
    InputData inputData;

    glm::vec3 position = glm::vec3(0);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::vec3 linearVelocity = glm::vec3(0);

    DeadRecBody drBody;

    glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
    glm::mat4 transform = glm::mat4(1);

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

    Render::ParticleEmitter* particleEmitterLeft;
    Render::ParticleEmitter* particleEmitterRight;
    float emitterOffset = -0.5f;

    uint32 id = 0;
    bool isHit = false;
    float timeSinceLastLaser = 0.f;

    bool CheckCollisions();
    void CompareAndSetImputData(const InputData& data);
    void FollowThisWithCamera(float dt);
    void ServerUpdate(float dt);
    void ClientUpdate(float dt);
    void SetServerData(const glm::vec3& serverPos, const glm::vec3& serverVel, const glm::vec3& serverAcc, const glm::quat& serverOri, bool hardReset, uint64 timeStamp);
    
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