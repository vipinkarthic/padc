// terrain_raytrace_debug.cpp
// Combined rasterization + simple CPU ray tracer (light at camera position).
// Original authors: 2023BCS0011 Vipin Karthic, 2023BCS0020 Sanjay
// Additions by ChatGPT: debug prints, initial texture, worker upload logs

#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cmath>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>

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
struct Tri {
    glm::vec3 a, b, c;
    glm::vec3 normal;
};

struct Chunk {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    int cx = 0, cz = 0; // chunk coordinates

    // CPU triangles for ray tracer (world-space)
    std::vector<Tri> cpuTris;
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

// raytrace toggle and resources
bool useRayTrace = false;
const int RT_W = 160;
const int RT_H = 90;
GLuint rtTexture = 0;
GLuint rtQuadVAO = 0, rtQuadVBO = 0;
GLuint quadProg = 0;
GLint quadTexLoc = -1;

// Async/worker globals
std::thread rtThread;
std::mutex rtMutex;
std::vector<unsigned char> rtPixels;    // pixel buffer produced by worker (size = RT_W*RT_H*3)
bool rtPixelsReady = false;             // true when worker produced a buffer for upload
bool rtThreadStop = false;              // tell worker to exit

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

// Quad shader for ray-traced texture
const char* quad_v = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 vUV;
void main(){ vUV = aUV; gl_Position = vec4(aPos.xy, 0.0, 1.0); }
)";

const char* quad_f = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main(){ FragColor = texture(uTex, vUV); }
)";

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// ---- helpers and chunk creation ----

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

            // also create cpu triangles in world coordinates
            auto getVertexPos = [&](unsigned int idx)->glm::vec3 {
                int base = idx * 6;
                float vx = verts[base + 0];
                float vy = verts[base + 1];
                float vz = verts[base + 2];
                return glm::vec3(vx, vy, vz);
            };
            {
                Tri t1;
                t1.a = getVertexPos(i0);
                t1.b = getVertexPos(i2);
                t1.c = getVertexPos(i1);
                t1.normal = glm::normalize(glm::cross(t1.b - t1.a, t1.c - t1.a));
                c.cpuTris.push_back(t1);

                Tri t2;
                t2.a = getVertexPos(i1);
                t2.b = getVertexPos(i2);
                t2.c = getVertexPos(i3);
                t2.normal = glm::normalize(glm::cross(t2.b - t2.a, t2.c - t2.a));
                c.cpuTris.push_back(t2);
            }
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

// HUD
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

// ---------------- Ray tracer helpers ----------------

struct Ray { glm::vec3 o; glm::vec3 d; };

// Moller-Trumbore intersection: returns true and t if hit (t along ray)
bool rayIntersectTri(const Ray &ray, const Tri &tri, float &outT, float &outU, float &outV) {
    const float EPS = 1e-6f;
    glm::vec3 edge1 = tri.b - tri.a;
    glm::vec3 edge2 = tri.c - tri.a;
    glm::vec3 pvec = glm::cross(ray.d, edge2);
    float det = glm::dot(edge1, pvec);
    if (fabs(det) < EPS) return false;
    float invDet = 1.0f / det;
    glm::vec3 tvec = ray.o - tri.a;
    outU = glm::dot(tvec, pvec) * invDet;
    if (outU < 0.0f || outU > 1.0f) return false;
    glm::vec3 qvec = glm::cross(tvec, edge1);
    outV = glm::dot(ray.d, qvec) * invDet;
    if (outV < 0.0f || outU + outV > 1.0f) return false;
    outT = glm::dot(edge2, qvec) * invDet;
    if (outT <= EPS) return false;
    return true;
}

// gather all CPU triangles currently loaded into a single vector for ray tracer
void gatherSceneTris(std::vector<Tri> &outTris) {
    outTris.clear();
    // reserve a rough amount
    outTris.reserve(chunks.size() * CHUNK_SIZE * CHUNK_SIZE * 2 / 4);
    for (auto &p : chunks) {
        const Chunk &c = p.second;
        outTris.insert(outTris.end(), c.cpuTris.begin(), c.cpuTris.end());
    }
}

// clamp helper
inline unsigned char u8clamp(float v) {
    int x = (int)(v * 255.0f);
    if (x < 0) x = 0; if (x > 255) x = 255;
    return (unsigned char)x;
}

// create ray-tracing resources
void createRayTraceResources() {
    // Create texture
    glGenTextures(1, &rtTexture);
    glBindTexture(GL_TEXTURE_2D, rtTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, RT_W, RT_H, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Initialize texture with sky color (so quad shows something immediately)
    std::vector<unsigned char> init(RT_W * RT_H * 3);
    for (int i = 0; i < RT_W * RT_H; ++i) {
        init[i*3+0] = u8clamp(0.53f);
        init[i*3+1] = u8clamp(0.81f);
        init[i*3+2] = u8clamp(0.92f);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RT_W, RT_H, GL_RGB, GL_UNSIGNED_BYTE, init.data());

    // fullscreen quad (NDC)
    float quad[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &rtQuadVAO);
    glGenBuffers(1, &rtQuadVBO);
    glBindVertexArray(rtQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rtQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    quadProg = compileShaderProgram(quad_v, quad_f);
    quadTexLoc = glGetUniformLocation(quadProg, "uTex");
}

void destroyRayTraceResources() {
    if (rtQuadVBO) glDeleteBuffers(1, &rtQuadVBO);
    if (rtQuadVAO) glDeleteVertexArrays(1, &rtQuadVAO);
    if (rtTexture) glDeleteTextures(1, &rtTexture);
    if (quadProg) glDeleteProgram(quadProg);
    rtQuadVBO = rtQuadVAO = rtTexture = 0;
    quadProg = 0;
}

// Pure CPU renderer: fills 'outPixels' with RT_W*RT_H*3 u8 bytes.
// Same brute-force algorithm as before but without GL calls.
void renderSceneIntoBuffer(std::vector<unsigned char> &outPixels) {
    std::vector<Tri> tris;
    gatherSceneTris(tris);
    if (!tris.empty()) {
        const Tri &t0 = tris[0];
        std::cerr << "[RT] tri0 a=("<<t0.a.x<<","<<t0.a.y<<","<<t0.a.z<<") normal=("<<t0.normal.x<<","<<t0.normal.y<<","<<t0.normal.z<<")\n";
    }

    std::cerr << "[RT] tris.size()=" << tris.size() << " chunks=" << chunks.size() << std::endl;

    if (tris.empty()) {
        // fill with sky color
        outPixels.assign(RT_W * RT_H * 3, 0);
        for (int i = 0; i < RT_W * RT_H; ++i) {
            outPixels[i*3+0] = u8clamp(0.53f);
            outPixels[i*3+1] = u8clamp(0.81f);
            outPixels[i*3+2] = u8clamp(0.92f);
        }
        return;
    }

    // derive camera basis from inverse view matrix (works with your Camera)
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camForward = glm::normalize(glm::vec3(invView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    glm::vec3 camUp      = glm::normalize(glm::vec3(invView * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    glm::vec3 camRight   = glm::normalize(glm::cross(camForward, camUp));
    glm::vec3 camPos     = camera.position;

    float fov = glm::radians(60.0f);
    float aspect = (float)RT_W / (float)RT_H;
    float halfH = tanf(fov * 0.5f);
    float halfW = aspect * halfH;

    outPixels.assign(RT_W * RT_H * 3, 0);

    for (int y = 0; y < RT_H; ++y) {
        for (int x = 0; x < RT_W; ++x) {
            float nx = ( (x + 0.5f) / (float)RT_W ) * 2.0f - 1.0f;
            float ny = ( (y + 0.5f) / (float)RT_H ) * 2.0f - 1.0f;
            ny = -ny;
            glm::vec3 dir = glm::normalize(camForward + nx * halfW * camRight + ny * halfH * camUp);
            Ray ray{ camPos, dir };

            float bestT = 1e30f;
            const Tri* bestTri = nullptr;
            static bool logged = false;
            for (const Tri &t : tris) {
                float tval, u, v;
                if (rayIntersectTri(ray, t, tval, u, v)) {
                    if (tval < bestT) {
                        bestT = tval;
                        bestTri = &t;
                    }
                }
                if(bestTri && !logged){
                    std::cerr << "[RT] tri a=("<<bestTri->a.x<<","<<bestTri->a.y<<","<<bestTri->a.z<<") b=("<<bestTri->b.x<<","<<bestTri->b.y<<","<<bestTri->b.z<<") c=("<<bestTri->c.x<<","<<bestTri->c.y<<","<<bestTri->c.z<<")\n";
                    logged = true;
                }
            }

            glm::vec3 finalColor(0.53f, 0.81f, 0.92f); // sky
            if (bestTri) {
                // DEBUG: simple solid color for hits (makes sure upload/draw path visible)
                finalColor = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            int idx = (y * RT_W + x) * 3;
            outPixels[idx+0] = u8clamp(finalColor.r);
            outPixels[idx+1] = u8clamp(finalColor.g);
            outPixels[idx+2] = u8clamp(finalColor.b);
        }
    }

    // debug: print first few pixels and center pixel sample
    if (!outPixels.empty()) {
        std::cerr << "[RT] outPixels.size=" << outPixels.size()
                << " first=" << (int)outPixels[0] << "," << (int)outPixels[1] << "," << (int)outPixels[2];

        int cx = RT_W/2, cy = RT_H/2;
        int cidx = (cy * RT_W + cx) * 3;
        if (cidx + 2 < (int)outPixels.size()) {
            std::cerr << " center=" << (int)outPixels[cidx] << "," << (int)outPixels[cidx+1] << "," << (int)outPixels[cidx+2];
        }
        std::cerr << std::endl;
    }
}

// Worker loop: computes pixel buffer when needed and hands it off to main thread
void rtWorkerLoop() {
    std::vector<unsigned char> localPixels;
    while (!rtThreadStop) {
        bool shouldRender = false;
        {
            // decide if we should render: render when raytrace mode is active and no pixels ready
            std::lock_guard<std::mutex> lk(rtMutex);
            if (useRayTrace && !rtPixelsReady) shouldRender = true;
        }

        if (shouldRender) {
            // heavy CPU work (no GL here)
            renderSceneIntoBuffer(localPixels);

            // move into shared buffer
            {
                std::lock_guard<std::mutex> lk(rtMutex);
                rtPixels.swap(localPixels);         // fast swap if sizes match
                rtPixelsReady = true;
                std::cerr << "[RT worker] finished frame; rtPixelsReady=1, pixels=" << rtPixels.size() << std::endl;
            }
        } else {
            // sleep a little so this loop doesn't hog CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }
}

// ---------------- End ray tracer helpers ----------------

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

    // raytrace resources (create texture and quad)
    createRayTraceResources();

    // initial chunk load (start worker AFTER chunks exist so it sees geometry)
    auto camChunk = updateLoadedChunksAndReturnCamChunk();
    int lastChunkX = camChunk.first;
    int lastChunkZ = camChunk.second;
    std::cout << "Initial camera chunk: (" << lastChunkX << ", " << lastChunkZ << ")  loaded: " << chunks.size() << std::endl;

    // start RT worker after initial scene is created
    rtThreadStop = false;
    rtThread = std::thread(rtWorkerLoop);

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

        if (!useRayTrace) {
            glUseProgram(program);
            glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(locProj, 1, GL_FALSE, &proj[0][0]);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);

            // render all loaded chunks (rasterized)
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
        } else {
            // ray-traced path (non-blocking)
            // if worker produced pixels, upload them to GPU texture (must be done on main thread)
            bool havePixels = false;
            {
                std::lock_guard<std::mutex> lk(rtMutex);
                havePixels = rtPixelsReady;
            }
            if (havePixels) {
                std::lock_guard<std::mutex> lk(rtMutex);
                glBindTexture(GL_TEXTURE_2D, rtTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RT_W, RT_H, GL_RGB, GL_UNSIGNED_BYTE, rtPixels.data());
                rtPixelsReady = false;
                std::cerr << "[Main] uploaded RT texture; size=" << RT_W << "x" << RT_H << std::endl;
            }
            // draw fullscreen quad with latest rt texture
            glDisable(GL_DEPTH_TEST);
            glUseProgram(quadProg);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rtTexture);
            glUniform1i(quadTexLoc, 0);
            glBindVertexArray(rtQuadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);
        }

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
               << " | loaded=" << chunks.size()
               << " | mode=" << (useRayTrace ? "RAY" : "RAST");
            glfwSetWindowTitle(window, ss.str().c_str());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    destroyHUD();
    destroyRayTraceResources();
    for (auto &p : chunks) {
        destroyChunk(p.second);
    }
    chunks.clear();
    glDeleteProgram(program);

    rtThreadStop = true;
    if (rtThread.joinable()) rtThread.join();

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
        } else if (key == GLFW_KEY_R) {
            useRayTrace = !useRayTrace;
            std::cout << "Ray trace: " << (useRayTrace ? "ON" : "OFF") << std::endl;
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
