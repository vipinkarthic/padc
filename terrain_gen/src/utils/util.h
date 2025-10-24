#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <queue>
#include <string>
#include <vector>

#include "BiomeSystem.h"
#include "Types.h"

using ll = long long;

#pragma once

namespace rng_util {
using ll = long long;
class RNG {
   public:
	explicit RNG(ll seed = 17112005);

	int nextInt();
	float nextFloat();

   private:
	ll _state;
};

ll splitmix(ll &state);
}  // namespace rng_util
namespace map {
void computeSlopeMap(const std::vector<float> &height, int W, int H, std::vector<float> &out_slope);

void computeWaterMask(const std::vector<float> &height, int W, int H, float oceanThreshold, float lakeThreshold, std::vector<unsigned char> &out_waterMask);

void computeCoastDistance(const std::vector<unsigned char> &waterMask, int W, int H, std::vector<int> &out_coastDist);

}  // namespace map

namespace helper {
std::vector<float> gridToVector(const Grid2D<float> &g);

void vectorToGrid(const std::vector<float> &v, Grid2D<float> &g);

std::vector<unsigned char> maskToRGB(const std::vector<uint8_t> &mask, int W, int H);

bool writePPM(const std::string &path, int W, int H, const std::vector<unsigned char> &rgb);

std::vector<unsigned char> heightToRGB(const Grid2D<float> &g);

std::vector<unsigned char> biomeToRGB(const Grid2D<Biome> &g);

}  // namespace helper
