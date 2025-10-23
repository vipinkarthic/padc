#include "camera.h"
Camera::Camera(glm::vec3 pos, float yaw_, float pitch_) : position(pos), yaw(yaw_), pitch(pitch_) {}
glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0,1,0));
}
void Camera::processKeyboard(float dx, float dz, float dt) {
    // basic movement along forward/right vectors
    float speed = 10.0f * dt;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    glm::vec3 right = glm::normalize(glm::cross(glm::normalize(front), glm::vec3(0,1,0)));
    position += glm::normalize(front) * dz * speed;
    position += right * dx * speed;
}
void Camera::processMouse(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch += dPitch;
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
}
