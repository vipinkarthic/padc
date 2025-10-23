#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

enum class Biome {
	Ocean,
	Beach,
	Lake,
	Desert,
	Savanna,
	Grassland,
	Shrubland,
	TropicalRainforest,
	SeasonalForest,
	BorealForest,
	Tundra,
	Snow,
	Rocky,
	Mountain,
	Swamp,
	Mangrove,
	Unknown
};

struct BiomeDef {
	Biome id;
	std::string name;

	float treeDensity = 0.0f;  // 0 to 1
	float rockDensity = 0.0f;
	float grassDensity = 0.0f;
	float bushDensity = 0.0f;
	float waterPlantDensity = 0.0f;

	float moistureModifier = 1.0f;
	float temperatureModifier = 1.0f;

	float prefMinElevation = 0.0f;
	float prefMaxElevation = 1.0f;

	float prefSlope = 0.0f;
	float slopeTolerance = 1.0f;

	float prefMinMoisture = 0.0f;
	float prefMaxMoisture = 1.0f;
	float prefMinTemperature = 0.0f;
	float prefMaxTemperature = 1.0f;

	bool prefersCoast = false;
	bool requiresWater = false;
	bool requiresHighElevation = false;
	bool prefersRiver = false;

	float weightElevation = 1.0f;
	float weightMoisture = 1.5f;
	float weightTemperature = 1.0f;
	float weightSlope = 0.7f;
	float weightCoastal = 1.2f;
	float weightRiver = 1.0f;

	BiomeDef() = default;
};

std::vector<BiomeDef> DEFAULT_BIOMES() {
	std::vector<BiomeDef> defs;

	auto push = [&](Biome id, const char *name, float tmod, float mmod, float treed, float rockd, float grassd, float bushd, float minE, float maxE, float minM,
					float maxM, float minT, float maxT, bool coast = false, bool reqWater = false, bool prefRiver = false, float prefSlope = 0.0f,
					float slopeTol = 1.0f) {
		BiomeDef biome;
		biome.id = id;
		biome.name = name;
		biome.temperatureModifier = tmod;
		biome.moistureModifier = mmod;
		biome.treeDensity = treed;
		biome.rockDensity = rockd;
		biome.grassDensity = grassd;
		biome.bushDensity = bushd;
		biome.prefMinElevation = minE;
		biome.prefMaxElevation = maxE;
		biome.prefMinMoisture = minM;
		biome.prefMaxMoisture = maxM;
		biome.prefMinTemperature = minT;
		biome.prefMaxTemperature = maxT;
		biome.prefersCoast = coast;
		biome.requiresWater = reqWater;
		biome.prefersRiver = prefRiver;
		biome.prefSlope = prefSlope;
		biome.slopeTolerance = slopeTol;
		defs.push_back(biome);
	};

	push(Biome::Ocean, "Ocean", 1.0f, 1.0f, 0, 0, 0, 0, 0.0f, 0.35f, 0.0f, 1.0f, 0.0f, true, true, false);
	push(Biome::Beach, "Beach", 1.0f, 1.0f, 0.01f, 0.02f, 0.15f, 0, 0.0f, 0.40f, 0.0f, 1.0f, 0.4f, true, false, false);
	push(Biome::Lake, "Lake", 1.0f, 1.0f, 0.02f, 0.03f, 0.25f, 0, 0.0f, 0.45f, 0.5f, 1.0f, 0.1f, true, true, false);
	push(Biome::Mangrove, "Mangrove", 1.0f, 1.3f, 0.5f, 0.05f, 0.15f, 0.2f, 0.0f, 0.5f, 0.65f, 1.0f, 0.6f, true, true, true);

	push(Biome::Desert, "Desert", 1.1f, 0.25f, 0.01f, 0.02f, 0.05f, 0.02f, 0.0f, 1.0f, 0.0f, 0.2f, 0.6f, false, false, false);
	push(Biome::Savanna, "Savanna", 1.05f, 0.6f, 0.15f, 0.05f, 0.25f, 0.08f, 0.0f, 0.8f, 0.35f, 0.8f, 0.6f, false, false, true);
	push(Biome::Grassland, "Grassland", 1.0f, 0.8f, 0.2f, 0.06f, 0.6f, 0.15f, 0.0f, 0.9f, 0.3f, 0.8f, 0.4f, false, false, true);
	push(Biome::Shrubland, "Shrubland", 1.0f, 0.6f, 0.12f, 0.08f, 0.35f, 0.25f, 0.0f, 1.0f, 0.2f, 0.65f, 0.3f, false, false, false);

	push(Biome::TropicalRainforest, "Tropical Rainforest", 1.02f, 1.4f, 0.9f, 0.05f, 0.8f, 0.25f, 0.15f, 0.8f, 0.6f, 1.0f, 0.6f, false, false, false);
	push(Biome::SeasonalForest, "Seasonal Forest", 1.0f, 1.0f, 0.6f, 0.08f, 0.6f, 0.2f, 0.08f, 0.9f, 0.4f, 0.8f, 0.45f, false, false, false);
	push(Biome::BorealForest, "Boreal Forest", 0.9f, 0.9f, 0.5f, 0.12f, 0.4f, 0.12f, 0.12f, 0.95f, 0.1f, 0.6f, 0.2f, false, false, false);

	push(Biome::Tundra, "Tundra", 0.85f, 0.6f, 0.02f, 0.2f, 0.08f, 0.05f, 0.0f, 0.9f, 0.0f, 1.0f, 0.1f, false, false, false);
	push(Biome::Snow, "Snow/Ice", 0.5f, 0.8f, 0.0f, 0.05f, 0.0f, 0.0f, 0.75f, 1.0f, 0.0f, 1.0f, 0.0f, false, false, false);
	push(Biome::Rocky, "Rocky", 0.9f, 0.8f, 0.02f, 0.6f, 0.02f, 0.01f, 0.4f, 1.0f, 0.0f, 1.0f, 0.0f, false, false, false);
	push(Biome::Mountain, "Mountain", 0.95f, 0.85f, 0.05f, 0.3f, 0.05f, 0.02f, 0.6f, 1.0f, 0.0f, 1.0f, 0.0f, false, false, false);

	push(Biome::Swamp, "Swamp", 1.0f, 1.5f, 0.5f, 0.05f, 0.2f, 0.3f, 0.0f, 0.6f, 0.6f, 1.0f, 0.4f, false, true, true);

	return defs;
}
