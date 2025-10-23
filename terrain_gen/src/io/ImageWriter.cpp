// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

// #include "ImageWriter.h"
// #include <algorithm>
// #include <cmath>
// #include <functional>
// #include <iostream>
// #include <cstring>
// #include <cassert>

// namespace ImageWriter {

// namespace {
//     // Helper: clamp and convert float [0,1] -> 0..255
//     inline uint8_t to8bit(float v) {
//         if (std::isnan(v)) return 0;
//         if (v <= 0.0f) return 0;
//         if (v >= 1.0f) return 255;
//         return static_cast<uint8_t>(std::lroundf(v * 255.0f));
//     }

//     // Compute min/max of grid
//     void compute_min_max(const Grid2D& g, float &outMin, float &outMax) {
//         if (g.data.empty()) { outMin = outMax = 0.0f; return; }
//         outMin = g.data[0];
//         outMax = g.data[0];
//         for (size_t i = 1; i < g.data.size(); ++i) {
//             float v = g.data[i];
//             if (v < outMin) outMin = v;
//             if (v > outMax) outMax = v;
//         }
//     }

//     // Safe access for float grid with border handling (for gradient)
//     inline float sampleSafe(const Grid2D& g, int x, int y) {
//         x = std::max(0, std::min(g.width - 1, x));
//         y = std::max(0, std::min(g.height - 1, y));
//         return g.data[y * g.width + x];
//     }
// }

// // -------------------- saveHeightmapPNG --------------------
// bool saveHeightmapPNG(const Grid2D& grid, const std::string& filename) {
//     if (grid.width <= 0 || grid.height <= 0 || grid.data.empty()) {
//         std::cerr << "saveHeightmapPNG: empty grid\n";
//         return false;
//     }

//     float minv, maxv;
//     compute_min_max(grid, minv, maxv);
//     float range = maxv - minv;
//     if (range <= 1e-8f) range = 1.0f; // avoid div by zero

//     std::vector<uint8_t> pixels((size_t)grid.width * grid.height);
//     for (int y = 0; y < grid.height; ++y) {
//         for (int x = 0; x < grid.width; ++x) {
//             size_t i = (size_t)y * grid.width + x;
//             float norm = (grid.data[i] - minv) / range;
//             pixels[i] = to8bit(norm);
//         }
//     }

//     // stride in bytes
//     int stride = grid.width * 1;
//     int success = stbi_write_png(filename.c_str(), grid.width, grid.height, 1, pixels.data(), stride);
//     if (!success) {
//         std::cerr << "saveHeightmapPNG: failed to write " << filename << "\n";
//         return false;
//     }
//     return true;
// }

// // -------------------- saveColorMapPNG --------------------
// bool saveColorMapPNG(const Grid2D& grid,
//                      const std::function<RGB(float)>& colorFunc,
//                      const std::string& filename) {
//     if (grid.width <= 0 || grid.height <= 0 || grid.data.empty()) {
//         std::cerr << "saveColorMapPNG: empty grid\n";
//         return false;
//     }
//     float minv, maxv;
//     compute_min_max(grid, minv, maxv);
//     float range = maxv - minv;
//     if (range <= 1e-8f) range = 1.0f;

//     std::vector<uint8_t> pixels((size_t)grid.width * grid.height * 3);
//     for (int y = 0; y < grid.height; ++y) {
//         for (int x = 0; x < grid.width; ++x) {
//             size_t idx = (size_t)y * grid.width + x;
//             float norm = (grid.data[idx] - minv) / range;
//             norm = std::min(1.0f, std::max(0.0f, norm));
//             RGB c = colorFunc(norm);
//             size_t pi = idx * 3;
//             pixels[pi + 0] = c[0];
//             pixels[pi + 1] = c[1];
//             pixels[pi + 2] = c[2];
//         }
//     }

//     int stride = grid.width * 3;
//     int success = stbi_write_png(filename.c_str(), grid.width, grid.height, 3, pixels.data(), stride);
//     if (!success) {
//         std::cerr << "saveColorMapPNG: failed to write " << filename << "\n";
//         return false;
//     }
//     return true;
// }

// // -------------------- saveBiomeMapPNG --------------------
// bool saveBiomeMapPNG(const std::vector<uint8_t>& biomeMap, int width, int height,
//                      const std::vector<RGB>& palette, const RGB& fallback,
//                      const std::string& filename) {
//     if (width <= 0 || height <= 0) {
//         std::cerr << "saveBiomeMapPNG: invalid dims\n";
//         return false;
//     }
//     if ((size_t)width * height != biomeMap.size()) {
//         std::cerr << "saveBiomeMapPNG: biome map size mismatch\n";
//         return false;
//     }

//     std::vector<uint8_t> pixels((size_t)width * height * 3);
//     for (int y = 0; y < height; ++y) {
//         for (int x = 0; x < width; ++x) {
//             size_t idx = (size_t)y * width + x;
//             uint8_t id = biomeMap[idx];
//             RGB c = fallback;
//             if (id < palette.size()) c = palette[id];
//             size_t pi = idx * 3;
//             pixels[pi + 0] = c[0];
//             pixels[pi + 1] = c[1];
//             pixels[pi + 2] = c[2];
//         }
//     }

//     int stride = width * 3;
//     int success = stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), stride);
//     if (!success) {
//         std::cerr << "saveBiomeMapPNG: failed to write " << filename << "\n";
//         return false;
//     }
//     return true;
// }

// // -------------------- saveShadedReliefPNG --------------------
// bool saveShadedReliefPNG(const Grid2D& height, const std::array<float,3>& sunDir, float zScale, const std::string& filename) {
//     if (height.width <= 0 || height.height <= 0 || height.data.empty()) {
//         std::cerr << "saveShadedReliefPNG: empty grid\n";
//         return false;
//     }
//     // Normalize sunDir (should have z>0 ideally)
//     float sx = sunDir[0], sy = sunDir[1], sz = sunDir[2];
//     float sl = std::sqrt(sx*sx + sy*sy + sz*sz);
//     if (sl <= 0.0f) { sx = 0.0f; sy = 0.0f; sz = 1.0f; }
//     else { sx /= sl; sy /= sl; sz /= sl; }

//     const int w = height.width, h = height.height;
//     std::vector<uint8_t> pixels((size_t)w * h);

//     // Compute gradient via central differences, convert to normal, dot with sunDir
//     for (int y = 0; y < h; ++y) {
//         for (int x = 0; x < w; ++x) {
//             // sample neighbors clamped
//             float hL = sampleSafe(height, x - 1, y);
//             float hR = sampleSafe(height, x + 1, y);
//             float hU = sampleSafe(height, x, y - 1);
//             float hD = sampleSafe(height, x, y + 1);

//             // horizontal distance is 1 cell; scale vertical by zScale
//             float dx = (hR - hL) * 0.5f * zScale;
//             float dy = (hD - hU) * 0.5f * zScale;

//             // normal (pointing up)
//             float nx = -dx;
//             float ny = -dy;
//             float nz = 1.0f;
//             float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
//             nx /= nlen; ny /= nlen; nz /= nlen;

//             float dot = nx*sx + ny*sy + nz*sz; // range [-1,1]
//             float shade = (dot + 1.0f) * 0.5f; // map to [0,1]
//             pixels[(size_t)y * w + x] = to8bit(shade);
//         }
//     }

//     int stride = w * 1;
//     int success = stbi_write_png(filename.c_str(), w, h, 1, pixels.data(), stride);
//     if (!success) {
//         std::cerr << "saveShadedReliefPNG: failed to write " << filename << "\n";
//         return false;
//     }
//     return true;
// }

// } 
