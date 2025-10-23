#include "ThermalErosion.h"

#include <omp.h>

#include <cassert>
#include <cmath>
#include <cstring>	// memset

namespace erosion {

double runThermalErosion(Grid2D<float>& height, const ThermalParams& params) {
	const int W = height.width();
	const int H = height.height();
	const int N = W * H;
	assert(N > 0);

	// neighbor offsets (8-neighbor)
	const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const double diagDist = params.use_diagonal_distance ? std::sqrt(2.0) : 1.0;

	// total moved for reporting
	double total_moved = 0.0;

	// prepare index->(x,y) helpers
	auto idx = [&](int x, int y) { return y * W + x; };

	// We'll keep height in a linear vector for faster access
	std::vector<float> Hbuf(N);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) Hbuf[idx(x, y)] = height(x, y);

	int maxThreads = 1;
#pragma omp parallel
	{
		if (omp_get_thread_num() == 0) maxThreads = omp_get_num_threads();
	}
	// per-thread delta buffers to avoid atomics (deterministic)
	std::vector<std::vector<float>> threadDeltas(maxThreads, std::vector<float>(N, 0.0f));

	for (int iter = 0; iter < params.iterations; ++iter) {
		// zero per-thread buffers
		for (int t = 0; t < maxThreads; ++t) std::fill(threadDeltas[t].begin(), threadDeltas[t].end(), 0.0f);

// parallel compute outgoing transfers into each thread's buffer
#pragma omp parallel
		{
			int tid = omp_get_thread_num();
			auto& delta = threadDeltas[tid];

#pragma omp for schedule(static)
			for (int y = 0; y < H; ++y) {
				for (int x = 0; x < W; ++x) {
					int i = idx(x, y);
					double h = Hbuf[i];

					// collect neighbors with positive (h - hn - talus)
					double S = 0.0;
					double excesses[8];
					int validNei[8];
					int ncount = 0;

					for (int k = 0; k < 8; ++k) {
						int nx = x + dx[k];
						int ny = y + dy[k];
						if (nx < 0 || nx >= W || ny < 0 || ny >= H) {
							excesses[k] = 0.0;
							continue;
						}
						int ni = idx(nx, ny);
						double hn = Hbuf[ni];
						double dist = (k % 2 == 1) ? diagDist : 1.0;
						// convert to "slope-like" measure: height diff / distance
						double d = (h - hn) / dist;
						double exc = (d > params.talus) ? (d - params.talus) : 0.0;
						excesses[k] = exc;
						if (exc > 0.0) {
							S += exc;
							validNei[ncount++] = k;
						}
					}

					if (S <= 0.0) continue;

					// amount to move out of this cell this iteration
					double outTotal = params.relaxation * S;
					// subtract from the source cell (we'll accumulate into delta)
					delta[i] -= static_cast<float>(outTotal);

					// distribute proportionally to neighbors
					for (int niIdx = 0; niIdx < ncount; ++niIdx) {
						int k = validNei[niIdx];
						int nx = x + dx[k];
						int ny = y + dy[k];
						int nj = idx(nx, ny);
						double portion = (excesses[k] / S) * outTotal;
						delta[nj] += static_cast<float>(portion);
					}
				}  // x
			}  // y
		}  // omp parallel

		// deterministic reduction: sum per-thread deltas into a single delta array (thread order fixed)
		std::vector<float> deltaGlobal(N, 0.0f);
		for (int t = 0; t < maxThreads; ++t) {
			const auto& buf = threadDeltas[t];
			for (int i = 0; i < N; ++i) deltaGlobal[i] += buf[i];
		}

		// apply deltas to Hbuf, compute moved amount
		double movedThisIter = 0.0;
		for (int i = 0; i < N; ++i) {
			float d = deltaGlobal[i];
			if (d != 0.0f) {
				Hbuf[i] += d;
				movedThisIter += std::abs(d);
			}
		}
		total_moved += movedThisIter;

		// small early exit if nothing moved this iteration
		if (movedThisIter < 1e-9) break;
	}  // iter

	// write back heights to Grid2D
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) height(x, y) = Hbuf[idx(x, y)];

	return total_moved;
}

}  // namespace erosion
