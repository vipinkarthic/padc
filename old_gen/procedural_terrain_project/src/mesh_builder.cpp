#include "mesh_builder.h"
#include <cmath>

// Simple mesh builder stub (single quad mesh per point)
Mesh build_mesh_from_heightmap(const std::vector<float>& height, int W, int H, int chunk_x, int chunk_y, int chunk_size) {
    Mesh m;
    // Minimal stub: create a (chunk_size x chunk_size) grid of vertices
    int sx = chunk_x * chunk_size;
    int sy = chunk_y * chunk_size;
    for (int y = 0; y <= chunk_size; ++y) {
        for (int x = 0; x <= chunk_size; ++x) {
            int ix = std::min(sx + x, W-1);
            int iy = std::min(sy + y, H-1);
            float h = height[iy * W + ix];
            Vertex v;
            v.x = (float)ix;
            v.y = h;
            v.z = (float)iy;
            v.nx = 0; v.ny = 1; v.nz = 0;
            v.u = (float)x / chunk_size;
            v.v = (float)y / chunk_size;
            v.biome = 0;
            m.vertices.push_back(v);
        }
    }
    // create indices (two triangles per cell)
    int row = chunk_size + 1;
    for (int y = 0; y < chunk_size; ++y) {
        for (int x = 0; x < chunk_size; ++x) {
            uint32_t i0 = y * row + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + row;
            uint32_t i3 = i2 + 1;
            m.indices.push_back(i0);
            m.indices.push_back(i2);
            m.indices.push_back(i1);
            m.indices.push_back(i1);
            m.indices.push_back(i2);
            m.indices.push_back(i3);
        }
    }
    return m;
}
