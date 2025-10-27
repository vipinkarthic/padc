#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

#include "BiomeHelpers.h"
#include "BiomeSystem.h"
#include "Types.h"

using GridBiome = Grid2D<Biome>;

namespace biome {

struct ClassifierOptions {
	int coastDistanceTiles = 3;
	int riverDistanceTiles = 2;
	float oceanHeightThreshold = 0.35f;	 // height < ocean
	float lakeHeightThreshold = 0.45f;	 // height < lake
	float expectedMaxGradient = 0.18f;
	int smoothingIterations = 1;
	bool requiresWater = true;
};

static inline void computeDistanceMapBFS(int width, int height, std::vector<int>& sources, std::vector<int>& outDist) {
	outDist.assign(width * height, std::numeric_limits<int>::max());
	std::vector<std::pair<int, int>> currentLevel, nextLevel;

#pragma omp parallel for collapse(2)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = y * width + x;
			if (sources[idx]) {
				outDist[idx] = 0;
#pragma omp critical
				currentLevel.emplace_back(x, y);
			}
		}
	}

	constexpr int dx[4] = {1, -1, 0, 0};
	constexpr int dy[4] = {0, 0, 1, -1};

	int distance = 0;
	while (!currentLevel.empty()) {
		distance++;
		nextLevel.clear();

#pragma omp parallel
		{
			std::vector<std::pair<int, int>> localNext;
#pragma omp for nowait
			for (size_t i = 0; i < currentLevel.size(); i++) {
				auto [x, y] = currentLevel[i];
				int idx = y * width + x;
				int cd = outDist[idx];

				for (int k = 0; k < 4; k++) {
					int nx = x + dx[k];
					int ny = y + dy[k];
					if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
					int nidx = ny * width + nx;
					if (outDist[nidx] > cd + 1) {
						outDist[nidx] = cd + 1;
						localNext.emplace_back(nx, ny);
					}
				}
			}

#pragma omp critical
			{
				nextLevel.insert(nextLevel.end(), localNext.begin(), localNext.end());
			}
		}

		currentLevel.swap(nextLevel);
	}
}

static inline void computeNearMaskFromSources(int width, int height, std::vector<int>& sources, int thresholdTiles, std::vector<int>& outNear) {
	std::vector<int> dist;
	computeDistanceMapBFS(width, height, sources, dist);
	outNear.assign(width * height, 0);
#pragma omp parallel for schedule(static)
	for (int i = 0; i < width * height; i++)
		if (dist[i] <= thresholdTiles) outNear[i] = 1;
}

static inline void computeSlopeMap(int width, int height, const std::function<float(int, int)>& heightAt, std::vector<float>& outSlope,
								   float expectedMaxGrad = 0.18f) {
	outSlope.assign(width * height, 0.0f);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float cx = heightAt(x, y);
			float left = (x > 0) ? heightAt(x - 1, y) : cx;
			float right = (x + 1 < width) ? heightAt(x + 1, y) : cx;
			float up = (y > 0) ? heightAt(x, y - 1) : cx;
			float down = (y + 1 < height) ? heightAt(x, y + 1) : cx;
			float dx = (right - left) * 0.5f;
			float dy = (down - up) * 0.5f;
			float grad = std::sqrt(dx * dx + dy * dy);
			float s = std::clamp(grad / std::max(1e-6f, expectedMaxGrad), 0.0f, 1.0f);
			outSlope[y * width + x] = s;
		}
	}
}

template <typename T>
static inline void majorityFilter(int W, int H, std::vector<T>& mapData, int iterations = 1) {
	if (iterations <= 0) return;
	std::vector<T> tmp(W * H);
	for (int it = 0; it < iterations; it++) {
		tmp = mapData;

#pragma omp parallel for collapse(2) schedule(static)
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				int counts[256] = {0};
				for (int oy = -1; oy <= 1; oy++) {
					for (int ox = -1; ox <= 1; ox++) {
						int nx = x + ox, ny = y + oy;
						if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
						int idx = ny * W + nx;
						counts[(int)mapData[idx]]++;
					}
				}
				int centerVal = (int)mapData[y * W + x];
				int bestVal = centerVal;
				int bestCount = counts[centerVal];
				for (int i = 0; i < 256; i++) {
					if (counts[i] > bestCount) {
						bestVal = i;
						bestCount = counts[i];
					}
				}
				tmp[y * W + x] = (T)bestVal;
			}
		}
		mapData.swap(tmp);
	}
}

static inline float scoreBiome(const BiomeDef& b, float elevation, float temperature, float moisture, float slope, bool nearCoast, bool nearRiver,
							   const ClassifierOptions& opts) {
	float adjTemp = std::clamp(temperature * b.temperatureModifier, 0.0f, 1.0f);
	float adjMoist = std::clamp(moisture * b.moistureModifier, 0.0f, 1.0f);

	if (b.requiresWater) {
		bool nearWater = (elevation <= opts.lakeHeightThreshold) || nearCoast || nearRiver;
		if (opts.requiresWater && !nearWater) return 0.0f;
	}
	if (b.requiresHighElevation) {
		if (elevation < b.prefMinElevation) return 0.0f;
	}

	float elevScore;
	if (elevation >= b.prefMinElevation && elevation <= b.prefMaxElevation)
		elevScore = 1.0f;
	else {
		float d = std::min(std::fabs(elevation - b.prefMinElevation), std::fabs(elevation - b.prefMaxElevation));
		elevScore = std::exp(-d * 8.0f);
	}

	float mscore;
	if (adjMoist >= b.prefMinMoisture && adjMoist <= b.prefMaxMoisture)
		mscore = 1.0f;
	else {
		float dm = std::min(std::fabs(adjMoist - b.prefMinMoisture), std::fabs(adjMoist - b.prefMaxMoisture));
		mscore = std::exp(-dm * 8.0f);
	}

	float tscore;
	if (adjTemp >= b.prefMinTemperature && adjTemp <= b.prefMaxTemperature)
		tscore = 1.0f;
	else {
		float dt = std::min(std::fabs(adjTemp - b.prefMinTemperature), std::fabs(adjTemp - b.prefMaxTemperature));
		tscore = std::exp(-dt * 8.0f);
	}

	float ds = std::fabs(slope - b.prefSlope) / std::max(1e-6f, b.slopeTolerance);
	float slopeScore = std::exp(-ds * 4.0f);

	float coastBoost = 1.0f;
	if (b.prefersCoast && nearCoast) coastBoost = 1.5f;
	if (b.prefersCoast && !nearCoast) coastBoost = 0.85f;

	float riverBoost = 1.0f;
	if (b.prefersRiver && nearRiver) riverBoost = 1.35f;

	float weightsSum = b.weightElevation + b.weightMoisture + b.weightTemperature + b.weightSlope + b.weightCoastal + b.weightRiver;
	float weighted = (b.weightElevation * elevScore + b.weightMoisture * mscore + b.weightTemperature * tscore + b.weightSlope * slopeScore +
					  b.weightCoastal * (nearCoast ? 1.0f : 0.0f) + b.weightRiver * (nearRiver ? 1.0f : 0.0f)) /
					 std::max(1e-6f, weightsSum);

	float finalScore = weighted * coastBoost * riverBoost;

	if ((b.prefMinMoisture > 0.7f) && (adjMoist < 0.15f)) finalScore *= 0.07f;

	return finalScore;
}

static inline Biome chooseBestBiome(const std::vector<BiomeDef>& defs, float elevation, float temperature, float moisture, float slope, bool nearCoast,
									bool nearRiver, const ClassifierOptions& opts) {
	float bestScore = -1.0f;
	Biome best = Biome::Unknown;
	for (const auto& d : defs) {
		float s = scoreBiome(d, elevation, temperature, moisture, slope, nearCoast, nearRiver, opts);
		if (s > bestScore) {
			bestScore = s;
			best = d.id;
		}
	}
	if (bestScore <= 1e-5f) {
		for (const auto& d : defs)
			if (d.id == Biome::Grassland) return Biome::Grassland;
		return best;
	}
	return best;
}

static inline bool classifyBiomeMap(const GridFloat& heightGrid, const GridFloat& tempGrid, const GridFloat& moistGrid, const GridInt* riverMaskGrid,
									const std::vector<BiomeDef>& defs, GridBiome& outBiomeGrid, const ClassifierOptions& opts = ClassifierOptions()) {
	const int W = heightGrid.width();
	const int H = heightGrid.height();
	if (tempGrid.width() != W || tempGrid.height() != H) return false;
	if (moistGrid.width() != W || moistGrid.height() != H) return false;
	if (outBiomeGrid.width() != W || outBiomeGrid.height() != H) return false;
	if (riverMaskGrid && (riverMaskGrid->width() != W || riverMaskGrid->height() != H)) return false;

	std::vector<int> oceanMask(W * H, 0);
	std::vector<int> lakeMask(W * H, 0);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			float e = heightGrid(x, y);
			int idx = y * W + x;
			if (e < opts.oceanHeightThreshold)
				oceanMask[idx] = 1;
			else if (e < opts.lakeHeightThreshold)
				lakeMask[idx] = 1;
		}
	}

	std::vector<int> riverMask(W * H, 0);
	if (riverMaskGrid) {
#pragma omp parallel for collapse(2) schedule(static)
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++) riverMask[y * W + x] = ((*riverMaskGrid)(x, y) ? 1 : 0);
	}

	std::vector<int> nearCoast(W * H, 0), nearRiver(W * H, 0);
	computeNearMaskFromSources(W, H, oceanMask, opts.coastDistanceTiles, nearCoast);
	if (!riverMask.empty()) computeNearMaskFromSources(W, H, riverMask, opts.riverDistanceTiles, nearRiver);

	std::vector<float> slopeMap;
	computeSlopeMap(W, H, [&](int x, int y) -> float { return heightGrid(x, y); }, slopeMap, opts.expectedMaxGradient);

	std::vector<Biome> chosen(W * H, Biome::Unknown);
#pragma omp parallel for collapse(2)
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			int idx = y * W + x;
			float e = heightGrid(x, y);
			float t = tempGrid(x, y);
			float m = moistGrid(x, y);
			float s = slopeMap[idx];
			bool nc = (nearCoast[idx] != 0);
			bool nr = (nearRiver[idx] != 0) || (riverMask[idx] != 0);
			Biome b = chooseBestBiome(defs, e, t, m, s, nc, nr, opts);
			chosen[idx] = b;
		}
	}

	if (opts.smoothingIterations > 0) {
		majorityFilter<Biome>(W, H, chosen, opts.smoothingIterations);
	}

#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			outBiomeGrid(x, y) = chosen[y * W + x];
		}
	}
	return true;
}

}  // namespace biome
