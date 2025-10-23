#include "perlin.h"

// NOTE: This is a placeholder "dummy noise" to let the project compile.
// Replace with real Perlin/Simplex + fBm implementation.
float fbm_noise(float x, float y, int octaves, float lacunarity, float persistence, uint32_t seed) {
    // a pseudo-noise: combination of sines (deterministic)
    float v = 0.0f;
    float amp = 1.0f, freq = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        v += amp * std::sin(freq * x * 12.9898f + freq * y * 78.233f + seed);
        amp *= persistence;
        freq *= lacunarity;
    }
    // normalize to [0,1]
    return 0.5f * (v / (1.0f) + 1.0f);
}
