#include <iostream>
#include <vector>
#include <chrono>
#include "perlin.h"
#include "mesh_builder.h"
#ifdef _OPENMP
#include <omp.h>
#endif

int main() {
#ifdef _OPENMP
    std::cout << "OpenMP available. Max threads: " << omp_get_max_threads() << "\n";
#else
    std::cout << "OpenMP not enabled in build.\n";
#endif
    std::cout << "Name & Roll: 2023BCS0011 Vipin Karthic\n";
    const int W = 1024, H = 1024;
    std::vector<float> height(W*H);

    auto t0 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float nx = (float)x / W * 8.0f;
            float ny = (float)y / H * 8.0f;
            height[y*W + x] = fbm_noise(nx, ny, 5, 2.0f, 0.5f, 42u);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double gen_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Heightmap generation time: " << gen_ms << " ms\n";

    // Build one chunk as demonstration
    Mesh m = build_mesh_from_heightmap(height, W, H, 0, 0, 64);
    std::cout << "Demo mesh vertices: " << m.vertices.size() << " indices: " << m.indices.size() << "\n";
    return 0;
}
