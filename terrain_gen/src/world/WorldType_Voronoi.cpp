#include "WorldType_Voronoi.h"

#include <util.h>

#include <algorithm>
#include <cmath>

static inline int mix32(int a, int b) {
	uint64_t res = (uint64_t)a * 0x9E3779B97F4A7C15ULL + b;
	return (int)((res >> 32) ^ res);
}

WorldType_Voronoi::WorldType_Voronoi(int width, int height, const VoronoiConfig& cfg) : width_(width), height_(height), cfg_(cfg) {
	std::cerr << "[VORONOI DEBUG] ctor start W=" << width << " H=" << height << " numPlates=" << cfg.numPlates << std::endl;
	// initialize noise in body (instead of in initializer list)
	noise_.init(cfg.seed + 12345);
	std::cerr << "[VORONOI DEBUG] about to allocate plates vector of size " << cfg.numPlates << std::endl;
	initPlates();
	std::cerr << "[VORONOI DEBUG] ctor finished" << std::endl;
}

void WorldType_Voronoi::initPlates() {
	plates_.clear();
	plates_.resize(cfg_.numPlates);
	int s = (int)cfg_.seed;
	
#pragma omp parallel for schedule(static)
	for (int i = 0; i < cfg_.numPlates; ++i) {
		rng_util::RNG rng(s + i); // Different seed per thread
		VoronoiPlate p;
		p.id = i;
		p.seed = (int)rng.nextInt();
		p.x = rng.nextFloat() * (float)width_;
		p.y = rng.nextFloat() * (float)height_;
		p.height = (rng.nextFloat() * 2.0f - 1.0f) * 0.6f;
		p.scale = 0.5f + rng.nextFloat() * 1.5f;
		plates_[i] = p;
	}
}

float WorldType_Voronoi::fbmNoiseAt(float fx, float fy) const {
	return noise_.fbm(fx, fy, cfg_.fbmFrequency, cfg_.fbmOctaves, cfg_.fbmLacunarity, cfg_.fbmGain);
}

float WorldType_Voronoi::voronoiHeightAt(int ix, int iy) const {
	float px = (float)ix + 0.5f;
	float py = (float)iy + 0.5f;
	float bestDist = 1e9f, secondDist = 1e9f;
	const VoronoiPlate* bestPlate = nullptr;
	for (const auto& p : plates_) {
		float dx = px - p.x;
		float dy = py - p.y;
		float d = std::sqrt(dx * dx + dy * dy);
		if (d < bestDist) {
			secondDist = bestDist;
			bestDist = d;
			bestPlate = &p;
		} else if (d < secondDist)
			secondDist = d;
	}
	float diag = std::sqrt((float)width_ * width_ + (float)height_ * height_);
	float nd = bestDist / std::max(1.0f, diag);
	float gap = (secondDist - bestDist) / std::max(1e-5f, diag);
	float ridge = std::exp(-gap * cfg_.ridgeStrength * 16.0f);
	float plateBase = bestPlate ? bestPlate->height : 0.0f;
	float falloff = 1.0f - std::clamp(nd * (bestPlate ? bestPlate->scale : 1.0f), 0.0f, 1.0f);
	float h = plateBase * 0.8f + falloff * 0.2f;
	h += ridge * 0.6f * (bestPlate ? bestPlate->height : 0.0f);
	return std::clamp(h, -1.0f, 1.0f);
}

void WorldType_Voronoi::generate(Grid2D<float>& outHeight) {
	assert(outHeight.width() == width_ && outHeight.height() == height_);
#pragma omp parallel for collapse(2)
	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
			float vor = voronoiHeightAt(x, y);			 // -1..1
			float fbm = fbmNoiseAt((float)x, (float)y);	 // -1..1
			float h = (1.0f - cfg_.fbmBlend) * vor + cfg_.fbmBlend * fbm;
			h = std::tanh(h * 1.2f);
			float mapped = (h + 1.0f) * 0.5f;
			outHeight(x, y) = mapped;
		}
	}
}
