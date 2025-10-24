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

inline std::vector<BiomeDef> DEFAULT_BIOMES() { 
	std::vector<BiomeDef> defs;
	return defs;
}
