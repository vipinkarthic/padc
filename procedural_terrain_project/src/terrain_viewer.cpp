#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cmath>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "shader.h"

// window
const int WIN_W = 1280;
const int WIN_H = 720;

// FPS camera
Camera camera(glm::vec3(0.0f, 10.0f, 0.0f), -90.0f, 0.0f);
double lastX = WIN_W / 2.0, lastY = WIN_H / 2.0;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

// input toggle
bool keys_pressed[1024] = {0};

// chunk definition
struct Chunk {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    int cx = 0, cz = 0; // chunk coordinates
};

// map key helper
static inline long long chunkKey(int cx, int cz) {
    return ( (long long)cx << 32 ) ^ (unsigned int)cz;
}

// chunk storage
std::unordered_map<long long, Chunk> chunks;

// parameters
const int CHUNK_SIZE = 64;        
const float CELL_SCALE = 1.0f;    
const int LOAD_RADIUS = 3;        

// wireframe toggle
bool wireframe = false;

// simple shader sources (small; you can replace with file-based shaders)
const char* vsrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vNormal;
out vec3 vWorldPos;
void main() {
    vec4 world = uModel * vec4(aPos,1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProj * uView * world;
}
)";

const char* fsrc = R"(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;
void main() {
    float h = vWorldPos.y;
    vec3 base = vec3(0.1, 0.6, 0.2); // green
    vec3 color = base + vec3(h * 0.02);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    color *= 0.3 + 0.7 * diff;
    FragColor = vec4(color, 1.0);
}
)";

const char* hud_v = R"(
#version 330 core
layout(location=0) in vec2 aPos; // clip-space coords (-1..1)
void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
)";
const char* hud_f = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

Chunk createFlatChunk(int cx, int cz, int chunkSize = CHUNK_SIZE, float scale = CELL_SCALE) {
    Chunk c;
    c.cx = cx; c.cz = cz;

    std::vector<float> verts; // x,y,z nx,ny,nz
    std::vector<unsigned int> inds;
    verts.reserve((chunkSize+1)*(chunkSize+1)*6);
    inds.reserve(chunkSize * chunkSize * 6);

    // world origin of chunk (lower-left corner)
    float originX = (float)(cx * chunkSize) * scale;
    float originZ = (float)(cz * chunkSize) * scale;

    // build grid vertices (y=0 flat)
    for (int z = 0; z <= chunkSize; ++z) {
        for (int x = 0; x <= chunkSize; ++x) {
            float wx = originX + (float)x * scale;
            float wy = 0.0f;
            float wz = originZ + (float)z * scale;
            // position
            verts.push_back(wx);
            verts.push_back(wy);
            verts.push_back(wz);
            // normal (up)
            verts.push_back(0.0f);
            verts.push_back(1.0f);
            verts.push_back(0.0f);
        }
    }

    int row = chunkSize + 1;
    for (int z = 0; z < chunkSize; ++z) {
        for (int x = 0; x < chunkSize; ++x) {
            unsigned int i0 = z * row + x;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + row;
            unsigned int i3 = i2 + 1;
            // triangle 1: i0, i2, i1
            inds.push_back(i0);
            inds.push_back(i2);
            inds.push_back(i1);
            // triangle 2: i1, i2, i3
            inds.push_back(i1);
            inds.push_back(i2);
            inds.push_back(i3);
        }
    }

    // upload to GPU
    glGenVertexArrays(1, &c.vao);
    glGenBuffers(1, &c.vbo);
    glGenBuffers(1, &c.ebo);

    glBindVertexArray(c.vao);
    glBindBuffer(GL_ARRAY_BUFFER, c.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);

    // position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
    // normal (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    c.indexCount = (int)inds.size();
    return c;
}

void destroyChunk(Chunk &c) {
    if (c.ebo) glDeleteBuffers(1, &c.ebo);
    if (c.vbo) glDeleteBuffers(1, &c.vbo);
    if (c.vao) glDeleteVertexArrays(1, &c.vao);
    c = Chunk();
}

std::pair<int,int> updateLoadedChunksAndReturnCamChunk() {
    glm::vec3 pos = camera.position;
    int cx = (int)std::floor(pos.x / (CHUNK_SIZE * CELL_SCALE));
    int cz = (int)std::floor(pos.z / (CHUNK_SIZE * CELL_SCALE));

    // mark existing keys
    std::unordered_map<long long, bool> keep;
    for (int dz = -LOAD_RADIUS; dz <= LOAD_RADIUS; ++dz) {
        for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx) {
            int ncx = cx + dx;
            int ncz = cz + dz;
            long long key = chunkKey(ncx, ncz);
            keep[key] = true;
            if (chunks.find(key) == chunks.end()) {
                // create chunk
                Chunk ch = createFlatChunk(ncx, ncz);
                chunks.emplace(key, std::move(ch));
            }
        }
    }

    // remove chunks not in 'keep'
    std::vector<long long> toRemove;
    for (auto &p : chunks) {
        if (keep.find(p.first) == keep.end()) toRemove.push_back(p.first);
    }
    for (long long k : toRemove) {
        Chunk &c = chunks[k];
        destroyChunk(c);
        chunks.erase(k);
    }

    return {cx, cz};
}

// helper to print loaded chunk coords
void printLoadedChunks() {
    std::cout << "Loaded chunks (" << chunks.size() << "): ";
    for (auto &p : chunks) {
        int cx = (int)(p.second.cx);
        int cz = (int)(p.second.cz);
        std::cout << "(" << cx << "," << cz << ") ";
    }
    std::cout << std::endl;
}

GLuint hudVAO = 0, hudVBO = 0, hudProg = 0;
void createHUD() {
    float lines[] = {
        -0.02f, 0.0f,   0.02f, 0.0f,
         0.0f, -0.02f,  0.0f, 0.02f  
    };
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    hudProg = compileShaderProgram(hud_v, hud_f);
}

// cleanup HUD
void destroyHUD() {
    if (hudVBO) glDeleteBuffers(1, &hudVBO);
    if (hudVAO) glDeleteVertexArrays(1, &hudVAO);
    if (hudProg) glDeleteProgram(hudProg);
    hudVBO = hudVAO = hudProg = 0;
}

int main() {
    std::cout << "Name & Roll: 2023BCS0011 Vipin Karthic\n";
    std::cout << "Name & Roll: 2023BCS0020 Sanjay\n";

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n"; return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "Terrain Viewer", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // compile shader
    unsigned int program = compileShaderProgram(vsrc, fsrc);
    glUseProgram(program);

    // uniform locations
    GLint locModel = glGetUniformLocation(program, "uModel");
    GLint locView = glGetUniformLocation(program, "uView");
    GLint locProj = glGetUniformLocation(program, "uProj");

    // HUD
    createHUD();
    GLint hudColorLoc = glGetUniformLocation(hudProg, "uColor");

    // initial chunk load
    auto camChunk = updateLoadedChunksAndReturnCamChunk();
    int lastChunkX = camChunk.first;
    int lastChunkZ = camChunk.second;
    std::cout << "Initial camera chunk: (" << lastChunkX << ", " << lastChunkZ << ")  loaded: " << chunks.size() << std::endl;

    // timing / title update
    float fpsTimer = 0.0f;
    int fpsFrames = 0;
    float titleUpdateTimer = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        // frame timing
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // update chunks and get current camera chunk
        auto camChunkNow = updateLoadedChunksAndReturnCamChunk();
        if (camChunkNow.first != lastChunkX || camChunkNow.second != lastChunkZ) {
            lastChunkX = camChunkNow.first;
            lastChunkZ = camChunkNow.second;
            std::cout << "Entered chunk (" << lastChunkX << ", " << lastChunkZ << ")  loaded chunks: " << chunks.size() << std::endl;
        }

        // clear
        if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setup camera & projection
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glUseProgram(program);
        glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);

        // render all loaded chunks
        for (auto &p : chunks) {
            Chunk &c = p.second;
            glBindVertexArray(c.vao);
            glDrawElements(GL_TRIANGLES, c.indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        // draw HUD crosshair
        glUseProgram(hudProg);
        glUniform3f(hudColorLoc, 0.0f, 0.0f, 0.0f);
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);

        // fps overlay / title update every 0.2s
        fpsFrames++;
        fpsTimer += deltaTime;
        titleUpdateTimer += deltaTime;
        if (titleUpdateTimer >= 0.2f) {
            float fps = (float)fpsFrames / fpsTimer;
            fpsFrames = 0;
            fpsTimer = 0.0f;
            titleUpdateTimer = 0.0f;

            // camera pos and chunk
            glm::vec3 p = camera.position;
            std::ostringstream ss;
            ss << "Terrain Viewer - FPS: " << (int)fps
               << " | pos(" << std::fixed << std::setprecision(1) << p.x << "," << p.y << "," << p.z << ")"
               << " | chunk(" << lastChunkX << "," << lastChunkZ << ")"
               << " | loaded=" << chunks.size();
            glfwSetWindowTitle(window, ss.str().c_str());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    destroyHUD();
    for (auto &p : chunks) {
        destroyChunk(p.second);
    }
    chunks.clear();
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}

// Mouse callback: update camera yaw/pitch
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos;
        firstMouse = false;
    }
    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos);
    lastX = xpos; lastY = ypos;
    camera.processMouse(xoffset * 0.1f, yoffset * 0.1f);
}

// Key callback to track pressed/released and debug keys
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // common keys
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys_pressed[key] = true;
        else if (action == GLFW_RELEASE) keys_pressed[key] = false;
    }

    // debug / one-shot keys on press
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_L) {
            printLoadedChunks();
        } else if (key == GLFW_KEY_O) {
            wireframe = !wireframe;
            std::cout << "Wireframe: " << (wireframe ? "ON" : "OFF") << std::endl;
        } else if (key == GLFW_KEY_P) {
            glm::vec3 p = camera.position;
            int cx = (int)std::floor(p.x / (CHUNK_SIZE * CELL_SCALE));
            int cz = (int)std::floor(p.z / (CHUNK_SIZE * CELL_SCALE));
            std::cout << "Camera pos: (" << p.x << "," << p.y << "," << p.z << ")  chunk: (" << cx << "," << cz << ")\n";
        }
    }
}

void processInput(GLFWwindow* window) {
    float moveX = 0.0f, moveZ = 0.0f;
    if (keys_pressed[GLFW_KEY_W]) moveZ += 1.0f;
    if (keys_pressed[GLFW_KEY_S]) moveZ -= 1.0f;
    if (keys_pressed[GLFW_KEY_A]) moveX -= 1.0f;
    if (keys_pressed[GLFW_KEY_D]) moveX += 1.0f;
    if (keys_pressed[GLFW_KEY_Q]) camera.position.y += 10.0f * deltaTime; // ascend
    if (keys_pressed[GLFW_KEY_E]) camera.position.y -= 10.0f * deltaTime; // descend

    camera.processKeyboard(moveX, moveZ, deltaTime);
}
