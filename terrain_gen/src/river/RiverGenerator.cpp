#include "RiverGenerator.h"

#include <omp.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <queue>

RiverGenerator::RiverGenerator(int width, int height, const std::vector<float>& heightmap, const std::vector<int>& biome_map)
	: W(width), H(height), Hmap(heightmap), Biomes(biome_map) {
	FlowDir.assign(W * H, -1);
	FlowAccum.assign(W * H, 0.0f);
	RiverMask.assign(W * H, 0);
}

void RiverGenerator::run(const RiverParams& params) {
	computeFlowDirection();
	computeFlowAccumulation();
	extractRivers(params);
	carveRivers(params);
}

const std::vector<uint8_t>& RiverGenerator::getRiverMask() const { return RiverMask; }
const std::vector<float>& RiverGenerator::getHeightmap() const { return Hmap; }

void RiverGenerator::computeFlowDirection() {
	const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const float diagDist = std::sqrt(2.0f);

#pragma omp parallel for schedule(static)
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			int i = idx(x, y);
			float h = Hmap[i];
			int best_n = -1;
			float best_drop = 0.0f;
			for (int k = 0; k < 8; k++) {
				int nx = x + dx[k];
				int ny = y + dy[k];
				if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
				int ni = idx(nx, ny);
				float nh = Hmap[ni];
				float dist = (k % 2 == 0) ? 1.0f : diagDist;
				float drop = (h - nh) / dist;
				if (drop > best_drop) {
					best_drop = drop;
					best_n = ni;
				}
			}
			FlowDir[i] = best_n;  // -1 if none
		}
	}
}

void RiverGenerator::computeFlowAccumulation() {
	int N = W * H;
	std::vector<int> order(N);
#pragma omp parallel for schedule(static)
	for (int i = 0; i < N; i++) order[i] = i;
	std::sort(order.begin(), order.end(), [&](int a, int b) { return Hmap[a] > Hmap[b]; });

	std::fill(FlowAccum.begin(), FlowAccum.end(), 1.0f);

	for (int id = 0; id < N; id++) {
		int i = order[id];
		int d = FlowDir[i];
		if (d != -1) {
			FlowAccum[d] += FlowAccum[i];
		}
	}
}

void RiverGenerator::extractRivers(const RiverParams& params) {
	int N = W * H;
#pragma omp parallel for schedule(static)
	for (int i = 0; i < N; i++) {
		RiverMask[i] = (FlowAccum[i] >= params.flow_accum_threshold) ? 255 : 0;
	}
}

void RiverGenerator::carveRivers(const RiverParams& params) {
	int N = W * H;
	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};
	std::vector<int> dist(N, INT_MAX);
	std::queue<int> q;
#pragma omp parallel for schedule(static)
	for (int i = 0; i < N; i++) {
		if (RiverMask[i]) {
			dist[i] = 0;
#pragma omp critical
			q.push(i);
		}
	}
	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		int cx = cur % W;
		int cy = cur / W;
		for (int k = 0; k < 4; k++) {
			int nx = cx + dx[k];
			int ny = cy + dy[k];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int ni = idx(nx, ny);
			if (dist[ni] > dist[cur] + 1) {
				dist[ni] = dist[cur] + 1;
				q.push(ni);
			}
		}
	}

#pragma omp parallel for schedule(static)
	for (int i = 0; i < N; i++) {
		if (dist[i] == INT_MAX) continue;
		float flow_here = FlowAccum[i];
		double width = params.width_multiplier * std::sqrt(std::max(1.0f, flow_here));
		double depth = std::clamp(params.min_channel_depth + (params.max_channel_depth - params.min_channel_depth) * std::min(1.0, std::log1p(flow_here) / 8.0),
								  params.min_channel_depth, params.max_channel_depth);
		double falloff = 1.0;
		if (dist[i] > 0) {
			double d = (double)dist[i];
			double radius = std::max(1.0, width);
			falloff = std::max(0.0, 1.0 - (d / (radius * 1.5)));
		}
		double delta = depth * falloff;
		Hmap[i] = float(Hmap[i] - delta);
	}
}
