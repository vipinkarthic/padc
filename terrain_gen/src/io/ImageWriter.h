// #pragma once
// #include <string>
// #include <vector>
// #include <array>
// #include <cstdint>
// #include "../core/Types.h"

// using RGB = std::array<uint8_t,3>;

// namespace ImageWriter {
// bool saveHeightmapPNG(const Grid2D& grid, const std::string& filename);

// bool saveColorMapPNG(const Grid2D& grid,
//                      const std::function<RGB(float)>& colorFunc,
//                      const std::string& filename);

// bool saveBiomeMapPNG(const std::vector<uint8_t>& biomeMap, int width, int
// height,
//                      const std::vector<RGB>& palette, const RGB& fallback,
//                      const std::string& filename);

// bool saveShadedReliefPNG(const Grid2D& height,
//                          const std::array<float,3>& sunDir,
//                          float zScale,
//                          const std::string& filename);

// }
