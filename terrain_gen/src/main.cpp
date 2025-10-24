#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "BiomeClassifier.h"
#include "BiomeHelpers.h"
#include "BiomeSystem.h"
#include "ErosionParams.h"
#include "HydraulicErosion.h"
#include "ObjectPlacer.h"
#include "PerlinNoise.h"
#include "RiverGenerator.h"
#include "Types.h"
#include "WorldType_Voronoi.h"
#include "json.hpp"
#include "util.h"

using json = nlohmann::json;

int main(int argc, char** argv) {
	char cwdBuf[4096];
	if (_getcwd(cwdBuf, sizeof(cwdBuf)) != nullptr) {
		std::cerr << "[DEBUG] CWD = " << cwdBuf << std::endl;
	} else {
		std::cerr << "[DEBUG] CWD unknown" << std::endl;
	}

	std::string cfgRelPath = "../../assets/config.json";
	std::filesystem::path absCfg = std::filesystem::absolute(cfgRelPath);

	std::ifstream f(absCfg.string());
	if (!f) {
		return 1;
	}

	json cfg;
	try {
		f >> cfg;
	} catch (const std::exception& e) {
		std::cerr << "[EXCEPTION] Failed parse config.json: " << e.what() << std::endl;
		return 1;
	}
	f.close();

	for (auto it = cfg.begin(); it != cfg.end(); ++it) std::cerr << it.key() << " ";

	int W = cfg.value("width", 512);
	int H = cfg.value("height", 512);
	uint32_t seed = cfg.value("seed", 424242u);

	// -----------------------------
	// Generate base heightmap using Voronoi plates + FBM
	// -----------------------------

	Grid2D<float> height(W, H), temp(W, H), moist(W, H);
	Grid2D<uint8_t> rivers(W, H);
	Grid2D<Biome> biomeMap(W, H);

	try {
		VoronoiConfig vcfg;
		vcfg.seed = seed;
		vcfg.numPlates = cfg.value("numPlates", 36);
		vcfg.fbmBlend = cfg.value("fbmBlend", 0.42f);
		vcfg.fbmFrequency = cfg.value("fbmFrequency", 0.0035f);
		vcfg.fbmOctaves = cfg.value("fbmOctaves", 5);

		WorldType_Voronoi world(W, H, vcfg);
		world.generate(height);

	} catch (const std::exception& e) {
		std::cerr << "[EXCEPTION] during grid/world construction: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "[EXCEPTION] unknown error during grid/world construction" << std::endl;
		return 1;
	}

	std::filesystem::create_directories("out");
	auto hRGB_before = helper::heightToRGB(height);
	if (!helper::writePPM("out/height_before_erosion.ppm", W, H, hRGB_before)) std::cerr << "Failed write out/height_before_erosion.ppm\n";

	// -----------------------------
	// Generate temperature and moisture maps
	// -----------------------------
	PerlinNoise pTemp(seed ^ 0xA5A5A5);
	PerlinNoise pMoist(seed ^ 0x5A5A5A);
	float baseFreq = 0.0025f;

#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			float e = height(x, y);
			float fx = (float)x;
			float fy = (float)y;
			float t = pTemp.fbm(fx + 100.0f, fy + 100.0f, baseFreq * 1.2f, 4, 2.0f, 0.6f);
			t = (t + 1.0f) * 0.5f;
			float latFactor = 1.0f - fabsf(((float)y / (float)H) * 2.0f - 1.0f);
			t = t * 0.6f + 0.4f * latFactor;
			temp(x, y) = std::clamp(t, 0.0f, 1.0f);

			float m = pMoist.fbm(fx - 100.0f, fy - 100.0f, baseFreq * 1.5f, 4, 2.0f, 0.6f);
			m = (m + 1.0f) * 0.5f;
			m = m * (0.6f + (1.0f - e) * 0.4f);
			moist(x, y) = std::clamp(m, 0.0f, 1.0f);
		}

	std::vector<BiomeDef> defs;
	std::ifstream bf("../../assets/biomes.json");
	if (bf) {
		json bj;
		bf >> bj;
		bf.close();
		defs = loadBiomeDefsFromJson(bj);
		if (defs.empty()) defs = DEFAULT_BIOMES();
	} else {
		defs = DEFAULT_BIOMES();
	}

	biome::ClassifierOptions opts;
	opts.coastDistanceTiles = cfg.value("coastDistanceTiles", 3);
	opts.oceanHeightThreshold = cfg.value("oceanHeightThreshold", 0.35f);
	opts.lakeHeightThreshold = cfg.value("lakeHeightThreshold", 0.45f);
	opts.smoothingIterations = cfg.value("smoothingIterations", 1);

	bool ok_pre = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_pre) {
		std::cerr << "Classification failed (dimension mismatch)\n";
		return 1;
	}
	auto bRGB_pre = helper::biomeToRGB(biomeMap);
	if (!helper::writePPM("out/biome_before_erosion.ppm", W, H, bRGB_pre)) std::cerr << "Failed write out/biome_before_erosion.ppm\n";

	// -----------------------------
	// Run hydraulic erosion stage
	// -----------------------------
	ErosionParams eparams;
	eparams.worldSeed = seed;
	eparams.numDroplets = std::max(1000, (int)(0.4f * W * H));
	eparams.maxSteps = 45;
	eparams.stepSize = 1.0f;
	eparams.capacityFactor = 8.0f;
	eparams.erodeRate = 0.5f;
	eparams.depositRate = 0.3f;
	eparams.evaporateRate = 0.015f;
	Grid2D<float> erodeMap(W, H), depositMap(W, H);
	auto stats = erosion::runHydraulicErosion(height, eparams, &erodeMap, &depositMap);

	std::cout << "[EROSION] totalEroded=" << stats.totalEroded << " totalDeposited=" << stats.totalDeposited << " droplets=" << stats.appliedDroplets
			  << std::endl;

	std::cerr << "[DEBUG] Hydraulic erosion finished" << std::endl;
	std::cerr << "[EROSION] totalEroded=" << stats.totalEroded << " totalDeposited=" << stats.totalDeposited << " droplets=" << stats.appliedDroplets
			  << std::endl;

	auto erodedRGB = helper::heightToRGB(erodeMap);
	auto depositRGB = helper::heightToRGB(depositMap);
	auto hRGB_after = helper::heightToRGB(height);
	if (!helper::writePPM("out/erosion_eroded.ppm", W, H, erodedRGB)) std::cerr << "Failed write out/erosion_eroded.ppm\n";
	if (!helper::writePPM("out/erosion_deposited.ppm", W, H, depositRGB)) std::cerr << "Failed write out/erosion_deposited.ppm\n";
	if (!helper::writePPM("out/height_after_erosion.ppm", W, H, hRGB_after)) std::cerr << "Failed write out/height_after_erosion.ppm\n";

	bool ok_after_erosion = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_after_erosion) {
		std::cerr << "[ERROR] Classification failed after erosion (dimension mismatch)\n";
	} else {
		auto bRGB_after_erosion = helper::biomeToRGB(biomeMap);
		if (!helper::writePPM("out/biome_after_erosion.ppm", W, H, bRGB_after_erosion)) std::cerr << "Failed write out/biome_after_erosion.ppm\n";
	}

	// -----------------------------
	// River generation stage
	// -----------------------------
	std::vector<float> heightVec = helper::gridToVector(height);

	RiverParams rparams;
	rparams.flow_accum_threshold = (W >= 2048 ? 4000.0 : (W >= 1024 ? 1000.0 : 200.0));
	rparams.min_channel_depth = 0.4;
	rparams.max_channel_depth = 6.0;
	rparams.width_multiplier = 0.002;
	rparams.carve_iterations = 1;
	rparams.bed_slope_reduction = 0.5;
	rparams.wetland_accum_threshold = 500.0;
	rparams.wetland_slope_max = 0.01;

	RiverGenerator rg(W, H, heightVec);
	rg.run(rparams);

	const std::vector<uint8_t>& riverMask = rg.getRiverMask();
	const std::vector<float>& heightAfterRiversVec = rg.getHeightmap();

	auto riverMaskRGB = helper::maskToRGB(riverMask, W, H);
	if (!helper::writePPM("out/river_map.ppm", W, H, riverMaskRGB)) std::cerr << "Failed to write out/river_map.ppm\n";

	helper::vectorToGrid(heightAfterRiversVec, height);

	auto hRGB_after_rivers = helper::heightToRGB(height);
	if (!helper::writePPM("out/height_after_rivers.ppm", W, H, hRGB_after_rivers)) std::cerr << "Failed to write out/height_after_rivers.ppm\n";

	bool ok_after_rivers = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_after_rivers) {
		std::cerr << "[ERROR] Classification failed after rivers (dimension mismatch)\n";
	} else {
		auto bRGB_after_rivers = helper::biomeToRGB(biomeMap);
		if (!helper::writePPM("out/biome_after_rivers.ppm", W, H, bRGB_after_rivers)) std::cerr << "Failed write out/biome_after_rivers.ppm\n";
	}

	// -----------------------------
	// Map utils + Object placement
	// -----------------------------
	std::vector<float> slope;
	map::computeSlopeMap(helper::gridToVector(height), W, H, slope);  // helper computes using a linear array

	std::vector<unsigned char> waterMask;
	float oceanThreshold = cfg.value("oceanHeightThreshold", 0.35f);
	float lakeThreshold = cfg.value("lakeHeightThreshold", 0.45f);
	map::computeWaterMask(helper::gridToVector(height), W, H, oceanThreshold, lakeThreshold, waterMask);

	std::vector<int> coastDist;
	map::computeCoastDistance(waterMask, W, H, coastDist);

	std::unordered_map<std::string, int> biomeIdToIndex;
	for (size_t i = 0; i < defs.size(); ++i) {
		std::string idStr = biomeToString(defs[i].id);
		biomeIdToIndex.insert_or_assign(idStr, (int)i);
	}

	std::vector<int> biome_idx((size_t)W * H, -1);
#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			int i = y * W + x;
			std::string id = biomeToString(biomeMap(x, y));	 // convert enum to string
			auto it = biomeIdToIndex.find(id);
			if (it != biomeIdToIndex.end())
				biome_idx[i] = it->second;
			else
				biome_idx[i] = -1;
		}
	}

	std::filesystem::path placementPath = std::filesystem::absolute("../../assets/object_placement.json");
	if (!std::filesystem::exists(placementPath)) {
		std::cerr << "[WARN] object_placement.json not found at " << placementPath.string() << " — skipping object placement\n";
	} else {
		std::ifstream pf(placementPath.string());
		json placeCfg;
		try {
			pf >> placeCfg;
		} catch (const std::exception& e) {
			std::cerr << "[ERROR] Failed to parse object_placement.json: " << e.what() << std::endl;
		}
		pf.close();

		ObjectPlacer placer(W, H, (float)W);
		placer.loadPlacementConfig(placeCfg);

		std::vector<std::string> biomeIds;
		biomeIds.reserve(defs.size());
		for (auto& d : defs) {
			biomeIds.push_back(biomeToString(d.id));
		}
		placer.setBiomeIdList(biomeIds);

		std::vector<float> heightLinear = helper::gridToVector(height);

		placer.place(heightLinear, slope, waterMask, coastDist, biome_idx);

		placer.writeCSV("out/objects.csv");
		placer.writeDebugPPM("out/objects_map.ppm");
	}

	// -----------------------------
	// Final outputs (height + biome)
	// -----------------------------
	auto hRGB = helper::heightToRGB(height);
	auto bRGB = helper::biomeToRGB(biomeMap);
	if (!helper::writePPM("out/height.ppm", W, H, hRGB)) std::cerr << "Failed write height\n";
	if (!helper::writePPM("out/biome.ppm", W, H, bRGB)) std::cerr << "Failed write biome\n";

	return 0;
}
