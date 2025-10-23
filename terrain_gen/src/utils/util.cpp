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
	// world cell spacing assumed 1 (scale can be applied outside)
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			float hz = height[idx(x, y)];
			// central differences with clamping at borders
			float hxm = (x > 0) ? height[idx(x - 1, y)] : hz;
			float hxp = (x < W - 1) ? height[idx(x + 1, y)] : hz;
			float hym = (y > 0) ? height[idx(x, y - 1)] : hz;
			float hyp = (y < H - 1) ? height[idx(x, y + 1)] : hz;
			// gradients
			float gx = (hxp - hxm) * 0.5f;
			float gy = (hyp - hym) * 0.5f;
			out_slope[idx(x, y)] = std::sqrt(gx * gx + gy * gy);
		}
	}
}

void computeWaterMask(const std::vector<float> &height, int W, int H, float oceanThreshold, float lakeThreshold, std::vector<unsigned char> &out_waterMask) {
	// We treat ocean as height <= oceanThreshold, lakes as local basins with height <= lakeThreshold.
	// Simpler: mark ocean by threshold; mark lakes by threshold but not connected to map border (so disconnected water)
	out_waterMask.assign(W * H, 0);
	auto idx = [&](int x, int y) { return y * W + x; };

	// first mark potential water by lakeThreshold
	std::vector<unsigned char> potential(W * H, 0);
	for (int i = 0; i < W * H; ++i) potential[i] = (height[i] <= lakeThreshold) ? 1 : 0;

	// flood-fill from edges for ocean: if potential and reachable from border -> ocean
	std::vector<unsigned char> visited(W * H, 0);
	std::queue<int> q;
	// push border potential water cells
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
	// BFS
	int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		int cx = cur % W, cy = cur / W;
		// mark as ocean if height <= oceanThreshold (some border cells may be lake-threshold but above ocean)
		if (height[cur] <= oceanThreshold)
			out_waterMask[cur] = 1;
		else
			out_waterMask[cur] = 1;	 // also mark as water; simpler: everything reachable and below lakeThreshold is water
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
	// Now mark remaining potential cells that were not visited as lakes (isolated)
	for (int i = 0; i < W * H; ++i) {
		if (potential[i] && !visited[i]) out_waterMask[i] = 1;
	}
}

void computeCoastDistance(const std::vector<unsigned char> &waterMask, int W, int H, std::vector<int> &out_coastDist) {
	// distance in tiles to nearest water tile (bredth-first)
	const int INF = std::numeric_limits<int>::max() / 4;
	out_coastDist.assign(W * H, INF);
	std::queue<int> q;
	auto idx = [&](int x, int y) { return y * W + x; };
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			int i = idx(x, y);
			if (waterMask[i]) {
				out_coastDist[i] = 0;
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
