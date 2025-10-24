#include "util.h"

#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

namespace rng_util {
RNG::RNG(ll seed) : _state(seed) {}

int RNG::nextInt() {
	ll x = _state;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	_state = x;
	return static_cast<int>((x * 2685821657736338717LL) >> 32);
}

float RNG::nextFloat() { return (nextInt() / 4294967296.0f); }

ll splitmix(ll &state) {
	ll z = (state += 2654435769LL);
	z = (z ^ (z >> 30)) * 2246822507LL;
	z = (z ^ (z >> 27)) * 3255373325LL;
	return z ^ (z >> 31);
}

}  // namespace rng_util

namespace map {

void computeSlopeMap(const std::vector<float> &height, int W, int H, std::vector<float> &out_slope) {
	out_slope.assign(W * H, 0.0f);
	auto idx = [&](int x, int y) { return y * W + x; };
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			float hz = height[idx(x, y)];
			float hxm = (x > 0) ? height[idx(x - 1, y)] : hz;
			float hxp = (x < W - 1) ? height[idx(x + 1, y)] : hz;
			float hym = (y > 0) ? height[idx(x, y - 1)] : hz;
			float hyp = (y < H - 1) ? height[idx(x, y + 1)] : hz;
			float gx = (hxp - hxm) * 0.5f;
			float gy = (hyp - hym) * 0.5f;
			out_slope[idx(x, y)] = std::sqrt(gx * gx + gy * gy);
		}
	}
}

void computeWaterMask(const std::vector<float> &height, int W, int H, float oceanThreshold, float lakeThreshold, std::vector<unsigned char> &out_waterMask) {
	out_waterMask.assign(W * H, 0);
	auto idx = [&](int x, int y) { return y * W + x; };

	std::vector<unsigned char> potential(W * H, 0);
#pragma omp parallel for schedule(static)
	for (int i = 0; i < W * H; ++i) potential[i] = (height[i] <= lakeThreshold) ? 1 : 0;

	std::vector<unsigned char> visited(W * H, 0);
	std::queue<int> q;
	for (int x = 0; x < W; ++x) {
		int i0 = idx(x, 0), i1 = idx(x, H - 1);
		if (potential[i0] && !visited[i0]) {
			visited[i0] = 1;
			q.push(i0);
		}
		if (potential[i1] && !visited[i1]) {
			visited[i1] = 1;
			q.push(i1);
		}
	}
	for (int y = 0; y < H; ++y) {
		int i0 = idx(0, y), i1 = idx(W - 1, y);
		if (potential[i0] && !visited[i0]) {
			visited[i0] = 1;
			q.push(i0);
		}
		if (potential[i1] && !visited[i1]) {
			visited[i1] = 1;
			q.push(i1);
		}
	}
	int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		int cx = cur % W, cy = cur / W;
		if (height[cur] <= oceanThreshold)
			out_waterMask[cur] = 1;
		else
			out_waterMask[cur] = 1;
		for (auto &d : dirs) {
			int nx = cx + d[0], ny = cy + d[1];
			if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
			int ni = idx(nx, ny);
			if (!visited[ni] && potential[ni]) {
				visited[ni] = 1;
				q.push(ni);
			}
		}
	}
	for (int i = 0; i < W * H; ++i) {
		if (potential[i] && !visited[i]) out_waterMask[i] = 1;
	}
}

void computeCoastDistance(const std::vector<unsigned char> &waterMask, int W, int H, std::vector<int> &out_coastDist) {
	const int INF = std::numeric_limits<int>::max() / 4;
	out_coastDist.assign(W * H, INF);
	std::queue<int> q;
	auto idx = [&](int x, int y) { return y * W + x; };
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			int i = idx(x, y);
			if (waterMask[i]) {
				out_coastDist[i] = 0;
#pragma omp critical
				q.push(i);
			}
		}
	int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		int cx = cur % W, cy = cur / W;
		for (auto &d : dirs) {
			int nx = cx + d[0], ny = cy + d[1];
			if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
			int ni = idx(nx, ny);
			if (out_coastDist[ni] > out_coastDist[cur] + 1) {
				out_coastDist[ni] = out_coastDist[cur] + 1;
				q.push(ni);
			}
		}
	}
}

}  // namespace map

namespace helper {
std::vector<float> gridToVector(const Grid2D<float> &g) {
	int W = g.width(), H = g.height();
	std::vector<float> v((size_t)W * H);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) v[(size_t)y * W + x] = g(x, y);
	return v;
}

void vectorToGrid(const std::vector<float> &v, Grid2D<float> &g) {
	int W = g.width(), H = g.height();
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) g(x, y) = v[(size_t)y * W + x];
}

std::vector<unsigned char> maskToRGB(const std::vector<uint8_t> &mask, int W, int H) {
	std::vector<unsigned char> out((size_t)W * H * 3);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			size_t i = (size_t)y * W + x;
			unsigned char m = mask[i];
			size_t idx = i * 3;
			out[idx + 0] = m;
			out[idx + 1] = m;
			out[idx + 2] = m;
		}
	return out;
}

bool writePPM(const std::string &path, int W, int H, const std::vector<unsigned char> &rgb) {
	std::ofstream f(path, std::ios::binary);
	if (!f) return false;
	f << "P6\n" << W << " " << H << "\n255\n";
	f.write((const char *)rgb.data(), rgb.size());
	f.close();
	return true;
}

std::vector<unsigned char> heightToRGB(const Grid2D<float> &g) {
	int W = g.width(), H = g.height();
	std::vector<unsigned char> out((size_t)W * H * 3);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			float v = g(x, y);
			unsigned char c = (unsigned char)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f);
			size_t idx = ((size_t)y * W + x) * 3;
			out[idx + 0] = c;
			out[idx + 1] = c;
			out[idx + 2] = c;
		}
	return out;
}

std::vector<unsigned char> biomeToRGB(const Grid2D<Biome> &g) {
	int W = g.width(), H = g.height();
	std::vector<unsigned char> out((size_t)W * H * 3);
	auto colorOf = [](Biome b) -> std::array<unsigned char, 3> {
		switch (b) {
			case Biome::Ocean:
				return {24, 64, 160};
			case Biome::Beach:
				return {238, 214, 175};
			case Biome::Lake:
				return {36, 120, 200};
			case Biome::Mangrove:
				return {31, 90, 42};
			case Biome::Desert:
				return {210, 180, 140};
			case Biome::Savanna:
				return {189, 183, 107};
			case Biome::Grassland:
				return {130, 200, 80};
			case Biome::TropicalRainforest:
				return {16, 120, 45};
			case Biome::SeasonalForest:
				return {34, 139, 34};
			case Biome::BorealForest:
				return {80, 120, 70};
			case Biome::Tundra:
				return {180, 190, 200};
			case Biome::Snow:
				return {240, 240, 250};
			case Biome::Rocky:
				return {140, 130, 120};
			case Biome::Mountain:
				return {120, 120, 140};
			case Biome::Swamp:
				return {34, 85, 45};
			default:
				return {255, 0, 255};
		}
	};
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			auto c = colorOf(g(x, y));
			size_t idx = ((size_t)y * W + x) * 3;
			out[idx + 0] = c[0];
			out[idx + 1] = c[1];
			out[idx + 2] = c[2];
		}
	return out;
}

}  // namespace helper