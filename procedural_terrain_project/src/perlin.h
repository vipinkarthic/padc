#pragma once
#include <cmath>
#include <cstdint>

// Simple placeholder fBm / Perlin interface
float fbm_noise(float x, float y, int octaves = 4, float lacunarity = 2.0f, float persistence = 0.5f, uint32_t seed = 0);
