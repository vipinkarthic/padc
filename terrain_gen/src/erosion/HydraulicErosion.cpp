#include "HydraulicErosion.h"

#include <omp.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#include "util.h"

using namespace std;

namespace erosion {

static inline float sampleBilinear(const GridFloat &g, float fx, float fy) {
	int w = g.width(), h = g.height();
	if (fx < 0) fx = 0;
	if (fy < 0) fy = 0;
	if (fx > w - 1) fx = (float)(w - 1);
	if (fy > h - 1) fy = (float)(h - 1);
	int x0 = (int)floor(fx);
	int y0 = (int)floor(fy);
	int x1 = std::min(x0 + 1, w - 1);
	int y1 = std::min(y0 + 1, h - 1);
	float sx = fx - x0, sy = fy - y0;
	float v00 = g(x0, y0), v10 = g(x1, y0), v01 = g(x0, y1), v11 = g(x1, y1);
	float a = v00 * (1 - sx) + v10 * sx;
	float b = v01 * (1 - sx) + v11 * sx;
	return a * (1 - sy) + b * sy;
}

static inline void sampleHeightAndGradient(const GridFloat &g, float fx, float fy, float &heightOut, float &gx, float &gy) {
	float eps = 1.0f;
	heightOut = sampleBilinear(g, fx, fy);
	float hx = sampleBilinear(g, fx + eps, fy);
	float lx = sampleBilinear(g, fx - eps, fy);
	float hy = sampleBilinear(g, fx, fy + eps);
	float ly = sampleBilinear(g, fx, fy - eps);
	gx = (hx - lx) * 0.5f / eps;  // dH/dx
	gy = (hy - ly) * 0.5f / eps;  // dH/dy
}

static inline void accumulateToCellQuad(vector<double> &buf, int w, int h, float fx, float fy, double amount) {
	if (amount == 0.0) return;
	if (w <= 0 || h <= 0) return;

	float fxc = fx;
	float fyc = fy;
	if (fxc < 0.0f) fxc = 0.0f;
	if (fyc < 0.0f) fyc = 0.0f;
	if (fxc > (float)(w - 1)) fxc = (float)(w - 1);
	if (fyc > (float)(h - 1)) fyc = (float)(h - 1);

	int x0 = (int)floor(fxc);
	int y0 = (int)floor(fyc);
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= w) x1 = w - 1;
	if (y1 >= h) y1 = h - 1;

	float sx = fxc - (float)x0;
	float sy = fyc - (float)y0;

	double w00 = (1.0 - sx) * (1.0 - sy);
	double w10 = sx * (1.0 - sy);
	double w01 = (1.0 - sx) * sy;
	double w11 = sx * sy;

	auto idx = [&](int x, int y) -> size_t { return (size_t)y * (size_t)w + (size_t)x; };

	size_t nCells = (size_t)w * (size_t)h;
	if (idx(x0, y0) < nCells) buf[idx(x0, y0)] += amount * w00;
	if (idx(x1, y0) < nCells) buf[idx(x1, y0)] += amount * w10;
	if (idx(x0, y1) < nCells) buf[idx(x0, y1)] += amount * w01;
	if (idx(x1, y1) < nCells) buf[idx(x1, y1)] += amount * w11;
}

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

ErosionStats runHydraulicErosion(GridFloat &heightGrid, const ErosionParams &params, GridFloat *outEroded, GridFloat *outDeposited) {
	int W = heightGrid.width();
	int H = heightGrid.height();
	const int N = params.numDroplets;
	const int maxSteps = params.maxSteps;

	const int numThreads = omp_get_max_threads();

	vector<vector<double>> erodeBufs, depositBufs;
	erodeBufs.resize(numThreads);
	depositBufs.resize(numThreads);
	const size_t nCells = (size_t)W * (size_t)H;

	for (int t = 0; t < numThreads; ++t) {
		erodeBufs[t].assign(nCells, 0.0);
		depositBufs[t].assign(nCells, 0.0);
	}
	std::cerr << "[ERODE DEBUG] entering droplet loop (parallel) ..." << std::endl;

#pragma omp parallel for schedule(static)
	for (int di = 0; di < N; ++di) {
		int tid = omp_get_thread_num();
		ll seedState = params.worldSeed;
		ll localState = seedState ^ (ll)di * 2654435761LL;
		ll seed = rng_util::splitmix(localState);
		rng_util::RNG rng(seed);

		// initialize droplet
		float x = rng.nextFloat() * (float)(W - 1);
		float y = rng.nextFloat() * (float)(H - 1);
		float dirX = 0.0f, dirY = 0.0f;
		float speed = params.initSpeed;
		float water = params.initWater;
		float sediment = 0.0f;

		for (int step = 0; step < maxSteps; ++step) {
			float heightHere, gradX, gradY;
			sampleHeightAndGradient(heightGrid, x, y, heightHere, gradX, gradY);

			// update direction: inertia + slope influence
			dirX = dirX * params.inertia - gradX * (1.0f - params.inertia);
			dirY = dirY * params.inertia - gradY * (1.0f - params.inertia);
			float len = sqrtf(dirX * dirX + dirY * dirY);
			if (len == 0.0f) {
				double r = rng.nextFloat();
				double theta = r * 2.0 * 3.141592653589793;
				dirX = (float)cos(theta) * 1e-6f;
				dirY = (float)sin(theta) * 1e-6f;
				len = sqrtf(dirX * dirX + dirY * dirY);
			}
			dirX /= len;
			dirY /= len;

			// move
			x += dirX * params.stepSize;
			y += dirY * params.stepSize;

			if (x < 0.0f || x > (W - 1) || y < 0.0f || y > (H - 1)) break;

			float newHeight = sampleBilinear(heightGrid, x, y);
			float deltaH = newHeight - heightHere;

			float potential = -deltaH;	// downhill positive
			speed = sqrtf(std::max(0.0f, speed * speed + potential * params.gravity));

			float slope = std::max(1e-6f, -deltaH / params.stepSize);

			float capacity = std::max(0.0f, params.capacityFactor * speed * water * slope);

			if (sediment > capacity) {
				double deposit = params.depositRate * (sediment - capacity);
				deposit = std::min<double>(deposit, sediment);
				accumulateToCellQuad(depositBufs[tid], W, H, x, y, deposit);
				sediment -= (float)deposit;
			} else {
				double delta = params.capacityFactor * (capacity - sediment);
				double erode = params.erodeRate * delta;
				erode = std::min(erode, (double)params.maxErodePerStep);
				double localHeight = newHeight;
				erode = std::min(erode, std::max(0.0, localHeight));
				if (erode > 0.0) {
					accumulateToCellQuad(erodeBufs[tid], W, H, x, y, erode);
					sediment += (float)erode;
				}
			}

			water *= (1.0f - params.evaporateRate);
			if (water < params.minWater) break;
			if (speed < params.minSpeed) break;
			std::cerr << "[ERODE DEBUG] droplet loop completed, starting reduction ..." << std::endl;
		}
	}

	ErosionStats stats;
	vector<double> finalErode(nCells, 0.0), finalDeposit(nCells, 0.0);
#pragma omp parallel for schedule(static)
	for (int t = 0; t < numThreads; ++t) {
		const auto &eb = erodeBufs[t];
		const auto &db = depositBufs[t];
		for (size_t i = 0; i < nCells; ++i) {
			finalErode[i] += eb[i];
			finalDeposit[i] += db[i];
		}
	}

#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			size_t idx = (size_t)y * W + x;
			double delta = finalDeposit[idx] - finalErode[idx];
			stats.totalEroded += finalErode[idx];
			stats.totalDeposited += finalDeposit[idx];
			double newH = (double)heightGrid(x, y) + delta;
			if (newH < 0.0) newH = 0.0;
			heightGrid(x, y) = (float)newH;
		}
	}

	if (outEroded) {
		outEroded->resize(W, H);
#pragma omp parallel for collapse(2) schedule(static)
		for (int y = 0; y < H; ++y)
			for (int x = 0; x < W; ++x) (*outEroded)(x, y) = (float)finalErode[(size_t)y * W + x];
	}
	if (outDeposited) {
		outDeposited->resize(W, H);
#pragma omp parallel for collapse(2) schedule(static)
		for (int y = 0; y < H; ++y)
			for (int x = 0; x < W; ++x) (*outDeposited)(x, y) = (float)finalDeposit[(size_t)y * W + x];
	}

	stats.appliedDroplets = N;
	return stats;
}
}  // namespace erosion