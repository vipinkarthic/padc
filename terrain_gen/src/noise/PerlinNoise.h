#pragma once
#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "util.h"

using ll = long long;

class PerlinNoise {
   public:
	explicit PerlinNoise(int seed = 1337, int permSize = 256) { init(seed, permSize); }
	void init(int seed, int permSize = 256) {
		permSize_ = permSize;
		rng_util::RNG rng(seed);
		_p.resize(permSize_);

		// cab just use iota but lets just parallelize it cz why tf not
#pragma omp parallel for schedule(static)
		for (int i = 0; i < permSize_; i++) {
			_p[i] = i;
		}

		// shuffle is seqn
		// for (int i = permSize_ - 1; i > 0; --i) {
		// 	int j = rng.nextInt() % (i + 1);
		// 	std::swap(_p[i], _p[j]);
		// }
		for (int i = permSize_ - 1; i > 0; --i) {
			// Ensure we get a non-negative 32-bit value before modulus
			uint32_t r = static_cast<uint32_t>(rng.nextInt());
			int j = static_cast<int>(r % static_cast<uint32_t>(i + 1));
			// Defensive: clamp j into valid range just in case
			if (j < 0) j = 0;
			if (j > i) j = i;
			std::swap(_p[i], _p[j]);
		}

		_p.resize(permSize_ * 2);
#pragma omp parallel for schedule(static)
		for (int i = 0; i < permSize_; i++) {
			_p[i + permSize_] = _p[i];
		}
	}

	float noise(float x, float y, float frequency = 1.0f) const {
		x *= frequency;
		y *= frequency;
		int xi = fastfloor(x) & 255;
		int yi = fastfloor(y) & 255;
		float xf = x - floorf(x);
		float yf = y - floorf(y);
		float u = fade(xf);
		float v = fade(yf);
		int aa = _p[_p[xi] + yi];
		int ab = _p[_p[xi] + yi + 1];
		int ba = _p[_p[xi + 1] + yi];
		int bb = _p[_p[xi + 1] + yi + 1];
		float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0f, yf), u);
		float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);
		float res = lerp(x1, x2, v);
		return std::clamp(res, -1.0f, 1.0f);
	}

	float fbm(float x, float y, float baseFreq, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const {
		float amp = 1.0f;
		float freq = 1.0f;
		float sum = 0.0f;
		float maxAmp = 0.0f;
		for (int i = 0; i < octaves; i++) {
			float n = noise(x, y, baseFreq * freq);
			sum += n * amp;
			maxAmp += amp;
			amp *= gain;
			freq *= lacunarity;
		}
		if (maxAmp > 0.0f) sum /= maxAmp;
		return std::clamp(sum, -1.0f, 1.0f);
	}

   private:
	std::vector<int> _p;
	int permSize_ = 256;
	static inline int fastfloor(float x) { return (int)floorf(x); }
	static inline float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
	static inline float lerp(float a, float b, float t) { return a + t * (b - a); }
	static inline float grad(int hash, float x, float y) {
		int h = hash & 7;
		float u = h < 4 ? x : y;
		float v = h < 4 ? y : x;
		return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v) * 0.5f;
	}
};
