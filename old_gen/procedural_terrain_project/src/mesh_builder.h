#pragma once
#include <vector>
#include <cstdint>
struct Vertex {
    float x,y,z;
    float nx,ny,nz;
    float u,v;
    int biome;
};
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
Mesh build_mesh_from_heightmap(const std::vector<float>& height, int W, int H, int chunk_x, int chunk_y, int chunk_size);
