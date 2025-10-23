#pragma once
#include <vector>

#include "PerlinNoise.h"
#include "Types.h"

struct VoronoiPlate {
	int id;
	int seed;
	float x, y;
	float height;
	float scale;
};

struct VoronoiConfig {
	int seed = 1337;
	int numPlates = 24;
	float ridgeStrength = 1.0f;
	float fbmBlend = 0.45f;
	int fbmOctaves = 5;
	float fbmFrequency = 0.004f;
	float fbmLacunarity = 2.0f;
	float fbmGain = 0.5f;
};

class WorldType_Voronoi {
   public:
	WorldType_Voronoi(int width, int height, const VoronoiConfig& cfg);
	void generate(Grid2D<float>& outHeight);

   private:
	int width_, height_;
	VoronoiConfig cfg_;
	std::vector<VoronoiPlate> plates_;
	PerlinNoise noise_;
	void initPlates();
	float voronoiHeightAt(int ix, int iy) const;
	float fbmNoiseAt(float fx, float fy) const;
};
