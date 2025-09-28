#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 pos, float yaw, float pitch);
    glm::mat4 getViewMatrix() const;
    void processKeyboard(float dx, float dz, float dt);
    void processMouse(float dYaw, float dPitch);
    glm::vec3 position;
    float yaw, pitch;
};
