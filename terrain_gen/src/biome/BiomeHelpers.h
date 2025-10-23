#pragma once
#include "BiomeSystem.h"

const char* biomeToString(Biome b) {
	switch (b) {
		case Biome::Ocean:
			return "Ocean";
		case Biome::Beach:
			return "Beach";
		case Biome::Lake:
			return "Lake";
		case Biome::Desert:
			return "Desert";
		case Biome::Savanna:
			return "Savanna";
		case Biome::Grassland:
			return "Grassland";
		case Biome::Shrubland:
			return "Shrubland";
		case Biome::TropicalRainforest:
			return "Tropical Rainforest";
		case Biome::SeasonalForest:
			return "Seasonal Forest";
		case Biome::BorealForest:
			return "Boreal Forest";
		case Biome::Tundra:
			return "Tundra";
		case Biome::Snow:
			return "Snow/Ice";
		case Biome::Rocky:
			return "Rocky";
		case Biome::Mountain:
			return "Mountain";
		case Biome::Swamp:
			return "Swamp";
		case Biome::Mangrove:
			return "Mangrove";
		default:
			return "Unknown";
	}
}

Biome biomeFromString(const std::string& s) {
	if (s == "Ocean") return Biome::Ocean;
	if (s == "Beach") return Biome::Beach;
	if (s == "Lake") return Biome::Lake;
	if (s == "Desert") return Biome::Desert;
	if (s == "Savanna") return Biome::Savanna;
	if (s == "Grassland") return Biome::Grassland;
	if (s == "Shrubland") return Biome::Shrubland;
	if (s == "Tropical Rainforest" || s == "TropicalRainforest") return Biome::TropicalRainforest;
	if (s == "Seasonal Forest" || s == "SeasonalForest") return Biome::SeasonalForest;
	if (s == "Boreal Forest" || s == "BorealForest") return Biome::BorealForest;
	if (s == "Tundra") return Biome::Tundra;
	if (s == "Snow" || s == "Snow/Ice") return Biome::Snow;
	if (s == "Rocky") return Biome::Rocky;
	if (s == "Mountain") return Biome::Mountain;
	if (s == "Swamp") return Biome::Swamp;
	if (s == "Mangrove") return Biome::Mangrove;
	return Biome::Unknown;
}

static inline std::vector<BiomeDef> loadBiomeDefsFromJson(const json& j) {
	std::vector<BiomeDef> out;
	if (!j.is_array()) return out;
	for (const auto& bj : j) {
		BiomeDef b;
		std::string id = bj.value("id", std::string("Unknown"));
		b.id = biomeFromString(id);
		b.name = bj.value("name", id);
		b.treeDensity = bj.value("treeDensity", b.treeDensity);
		b.rockDensity = bj.value("rockDensity", b.rockDensity);
		b.grassDensity = bj.value("grassDensity", b.grassDensity);
		b.bushDensity = bj.value("bushDensity", b.bushDensity);
		b.waterPlantDensity = bj.value("waterPlantDensity", b.waterPlantDensity);
		b.moistureModifier = bj.value("moistureModifier", b.moistureModifier);
		b.temperatureModifier = bj.value("temperatureModifier", b.temperatureModifier);
		b.prefMinElevation = bj.value("prefMinElevation", b.prefMinElevation);
		b.prefMaxElevation = bj.value("prefMaxElevation", b.prefMaxElevation);
		b.prefMinMoisture = bj.value("prefMinMoisture", b.prefMinMoisture);
		b.prefMaxMoisture = bj.value("prefMaxMoisture", b.prefMaxMoisture);
		b.prefMinTemperature = bj.value("prefMinTemperature", b.prefMinTemperature);
		b.prefMaxTemperature = bj.value("prefMaxTemperature", b.prefMaxTemperature);
		b.prefSlope = bj.value("prefSlope", b.prefSlope);
		b.slopeTolerance = bj.value("slopeTolerance", b.slopeTolerance);
		b.prefersCoast = bj.value("prefersCoast", b.prefersCoast);
		b.requiresWater = bj.value("requiresWater", b.requiresWater);
		b.prefersRiver = bj.value("prefersRiver", b.prefersRiver);
		b.weightElevation = bj.value("weightElevation", b.weightElevation);
		b.weightMoisture = bj.value("weightMoisture", b.weightMoisture);
		b.weightTemperature = bj.value("weightTemperature", b.weightTemperature);
		b.weightSlope = bj.value("weightSlope", b.weightSlope);
		b.weightCoastal = bj.value("weightCoastal", b.weightCoastal);
		b.weightRiver = bj.value("weightRiver", b.weightRiver);
		out.push_back(b);
	}
	return out;
}
