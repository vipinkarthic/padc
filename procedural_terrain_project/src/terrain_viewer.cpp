// terrain_viewer.cpp (top of file)
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

// Load Glad first (it provides OpenGL headers)
#include <glad/glad.h>

// Prevent GLFW from including the system GL headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include "camera.h"
#include "shader.h"


// window size
int WIN_W = 1280, WIN_H = 720;
Camera camera(glm::vec3(512, 100, 512), -90.0f, 0.0f);
double lastX = 0, lastY = 0;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos);
    lastX = xpos; lastY = ypos;
    camera.processMouse(xoffset * 0.1f, yoffset * 0.1f);
}

void processInput(GLFWwindow* window) {
    float curFrame = (float)glfwGetTime();
    deltaTime = curFrame - lastFrame;
    lastFrame = curFrame;
    float dx = 0, dz = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dz += 1;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dz -= 1;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx -= 1;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx += 1;
    camera.processKeyboard(dx, dz, deltaTime);
}

int main() {
    std::cout << "Name & Roll: 2023BCS0011 Vipin Karthic\n";
    if (!glfwInit()) {
        std::cerr<<"Failed to init GLFW\n"; return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "Terrain Viewer", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr<<"Failed to init GLAD\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Simple render loop (no actual meshes yet)
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: draw terrain chunks here

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
