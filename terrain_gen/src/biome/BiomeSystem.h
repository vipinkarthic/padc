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
	
	// Ocean biome - Dark blue (24, 64, 160)
	BiomeDef ocean;
	ocean.id = Biome::Ocean;
	ocean.name = "Ocean";
	ocean.requiresWater = true;
	ocean.prefMaxElevation = 0.35f;
	ocean.weightElevation = 2.0f;
	ocean.weightMoisture = 0.5f;
	ocean.weightTemperature = 0.5f;
	defs.push_back(ocean);
	
	// Beach biome - Light tan (238, 214, 175)
	BiomeDef beach;
	beach.id = Biome::Beach;
	beach.name = "Beach";
	beach.prefersCoast = true;
	beach.prefMinElevation = 0.35f;
	beach.prefMaxElevation = 0.45f;
	beach.weightCoastal = 2.0f;
	beach.weightElevation = 1.5f;
	defs.push_back(beach);
	
	// Lake biome - Blue (36, 120, 200)
	BiomeDef lake;
	lake.id = Biome::Lake;
	lake.name = "Lake";
	lake.requiresWater = true;
	lake.prefMinElevation = 0.35f;
	lake.prefMaxElevation = 0.45f;
	lake.weightElevation = 2.0f;
	defs.push_back(lake);
	
	// Mangrove biome - Dark green (31, 90, 42)
	BiomeDef mangrove;
	mangrove.id = Biome::Mangrove;
	mangrove.name = "Mangrove";
	mangrove.requiresWater = true;
	mangrove.prefersCoast = true;
	mangrove.prefMinElevation = 0.35f;
	mangrove.prefMaxElevation = 0.45f;
	mangrove.prefMinMoisture = 0.7f;
	mangrove.prefMaxMoisture = 1.0f;
	mangrove.weightCoastal = 2.0f;
	mangrove.weightMoisture = 2.0f;
	defs.push_back(mangrove);
	
	// Desert biome - Tan (210, 180, 140)
	BiomeDef desert;
	desert.id = Biome::Desert;
	desert.name = "Desert";
	desert.prefMinElevation = 0.45f;
	desert.prefMaxElevation = 0.8f;
	desert.prefMinMoisture = 0.0f;
	desert.prefMaxMoisture = 0.3f;
	desert.prefMinTemperature = 0.4f;
	desert.prefMaxTemperature = 1.0f;
	desert.weightMoisture = 2.0f;
	desert.weightTemperature = 1.5f;
	desert.weightElevation = 1.0f;
	defs.push_back(desert);
	
	// Savanna biome - Yellow-green (189, 183, 107)
	BiomeDef savanna;
	savanna.id = Biome::Savanna;
	savanna.name = "Savanna";
	savanna.prefMinElevation = 0.45f;
	savanna.prefMaxElevation = 0.7f;
	savanna.prefMinMoisture = 0.2f;
	savanna.prefMaxMoisture = 0.5f;
	savanna.prefMinTemperature = 0.5f;
	savanna.prefMaxTemperature = 1.0f;
	savanna.weightMoisture = 1.5f;
	savanna.weightTemperature = 1.2f;
	savanna.weightElevation = 1.0f;
	defs.push_back(savanna);
	
	// Grassland biome - Green (130, 200, 80)
	BiomeDef grassland;
	grassland.id = Biome::Grassland;
	grassland.name = "Grassland";
	grassland.prefMinElevation = 0.45f;
	grassland.prefMaxElevation = 0.7f;
	grassland.prefMinMoisture = 0.3f;
	grassland.prefMaxMoisture = 0.7f;
	grassland.prefMinTemperature = 0.2f;
	grassland.prefMaxTemperature = 0.8f;
	grassland.weightMoisture = 1.5f;
	grassland.weightTemperature = 1.0f;
	grassland.weightElevation = 1.0f;
	defs.push_back(grassland);
	
	// Tropical Rainforest biome - Dark green (16, 120, 45)
	BiomeDef tropicalRainforest;
	tropicalRainforest.id = Biome::TropicalRainforest;
	tropicalRainforest.name = "Tropical Rainforest";
	tropicalRainforest.prefMinElevation = 0.45f;
	tropicalRainforest.prefMaxElevation = 0.8f;
	tropicalRainforest.prefMinMoisture = 0.7f;
	tropicalRainforest.prefMaxMoisture = 1.0f;
	tropicalRainforest.prefMinTemperature = 0.6f;
	tropicalRainforest.prefMaxTemperature = 1.0f;
	tropicalRainforest.weightMoisture = 2.5f;
	tropicalRainforest.weightTemperature = 1.5f;
	tropicalRainforest.weightElevation = 1.0f;
	defs.push_back(tropicalRainforest);
	
	// Seasonal Forest biome - Forest green (34, 139, 34)
	BiomeDef seasonalForest;
	seasonalForest.id = Biome::SeasonalForest;
	seasonalForest.name = "Seasonal Forest";
	seasonalForest.prefMinElevation = 0.45f;
	seasonalForest.prefMaxElevation = 0.8f;
	seasonalForest.prefMinMoisture = 0.5f;
	seasonalForest.prefMaxMoisture = 1.0f;
	seasonalForest.prefMinTemperature = 0.3f;
	seasonalForest.prefMaxTemperature = 0.9f;
	seasonalForest.weightMoisture = 2.0f;
	seasonalForest.weightTemperature = 1.2f;
	seasonalForest.weightElevation = 1.0f;
	defs.push_back(seasonalForest);
	
	// Boreal Forest biome - Dark green (80, 120, 70)
	BiomeDef borealForest;
	borealForest.id = Biome::BorealForest;
	borealForest.name = "Boreal Forest";
	borealForest.prefMinElevation = 0.6f;
	borealForest.prefMaxElevation = 0.9f;
	borealForest.prefMinMoisture = 0.4f;
	borealForest.prefMaxMoisture = 0.8f;
	borealForest.prefMinTemperature = 0.0f;
	borealForest.prefMaxTemperature = 0.6f;
	borealForest.weightMoisture = 1.8f;
	borealForest.weightTemperature = 1.5f;
	borealForest.weightElevation = 1.2f;
	defs.push_back(borealForest);
	
	// Tundra biome - Light gray (180, 190, 200)
	BiomeDef tundra;
	tundra.id = Biome::Tundra;
	tundra.name = "Tundra";
	tundra.prefMinElevation = 0.7f;
	tundra.prefMaxElevation = 0.9f;
	tundra.prefMinMoisture = 0.2f;
	tundra.prefMaxMoisture = 0.6f;
	tundra.prefMinTemperature = 0.0f;
	tundra.prefMaxTemperature = 0.4f;
	tundra.weightElevation = 1.5f;
	tundra.weightTemperature = 2.0f;
	tundra.weightMoisture = 1.0f;
	defs.push_back(tundra);
	
	// Snow biome - White (240, 240, 250)
	BiomeDef snow;
	snow.id = Biome::Snow;
	snow.name = "Snow";
	snow.requiresHighElevation = true;
	snow.prefMinElevation = 0.9f;
	snow.prefMaxElevation = 1.0f;
	snow.prefMinTemperature = 0.0f;
	snow.prefMaxTemperature = 0.3f;
	snow.weightElevation = 2.0f;
	snow.weightTemperature = 2.0f;
	defs.push_back(snow);
	
	// Rocky biome - Brown (140, 130, 120)
	BiomeDef rocky;
	rocky.id = Biome::Rocky;
	rocky.name = "Rocky";
	rocky.requiresHighElevation = true;
	rocky.prefMinElevation = 0.8f;
	rocky.prefMaxElevation = 1.0f;
	rocky.prefSlope = 0.3f;
	rocky.slopeTolerance = 0.5f;
	rocky.weightElevation = 2.5f;
	rocky.weightSlope = 2.0f;
	defs.push_back(rocky);
	
	// Mountain biome - Gray (120, 120, 140)
	BiomeDef mountain;
	mountain.id = Biome::Mountain;
	mountain.name = "Mountain";
	mountain.requiresHighElevation = true;
	mountain.prefMinElevation = 0.8f;
	mountain.prefMaxElevation = 1.0f;
	mountain.weightElevation = 3.0f;
	mountain.weightMoisture = 0.5f;
	mountain.weightTemperature = 0.8f;
	defs.push_back(mountain);
	
	// Swamp biome - Dark green (34, 85, 45)
	BiomeDef swamp;
	swamp.id = Biome::Swamp;
	swamp.name = "Swamp";
	swamp.requiresWater = true;
	swamp.prefMinElevation = 0.35f;
	swamp.prefMaxElevation = 0.5f;
	swamp.prefMinMoisture = 0.8f;
	swamp.prefMaxMoisture = 1.0f;
	swamp.prefMinTemperature = 0.3f;
	swamp.prefMaxTemperature = 0.8f;
	swamp.weightMoisture = 2.5f;
	swamp.weightElevation = 1.5f;
	swamp.weightTemperature = 1.0f;
	defs.push_back(swamp);
	
	return defs;
}
