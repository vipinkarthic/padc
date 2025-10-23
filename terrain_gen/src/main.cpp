// main.cpp
#include <filesystem>
#ifdef _WIN32
#include <direct.h>	 // _getcwd
#else
#include <unistd.h>	 // getcwd
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

// Convert Grid2D<float> -> std::vector<float> (row-major)
static std::vector<float> gridToVector(const Grid2D<float>& g) {
	int W = g.width(), H = g.height();
	std::vector<float> v((size_t)W * H);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) v[(size_t)y * W + x] = g(x, y);
	return v;
}

// Convert std::vector<float> (row-major) -> Grid2D<float>
static void vectorToGrid(const std::vector<float>& v, Grid2D<float>& g) {
	int W = g.width(), H = g.height();
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) g(x, y) = v[(size_t)y * W + x];
}

// Convert river mask (0/255 bytes) -> PPM rgb vector<unsigned char>
static std::vector<unsigned char> maskToRGB(const std::vector<uint8_t>& mask, int W, int H) {
	std::vector<unsigned char> out((size_t)W * H * 3);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			size_t i = (size_t)y * W + x;
			unsigned char m = mask[i];
			size_t idx = i * 3;
			out[idx + 0] = m;
			out[idx + 1] = m;
			out[idx + 2] = m;
		}
	return out;
}

static bool writePPM(const std::string& path, int W, int H, const std::vector<unsigned char>& rgb) {
	std::ofstream f(path, std::ios::binary);
	if (!f) return false;
	f << "P6\n" << W << " " << H << "\n255\n";
	f.write((const char*)rgb.data(), rgb.size());
	f.close();
	return true;
}

static std::vector<unsigned char> heightToRGB(const Grid2D<float>& g) {
	int W = g.width(), H = g.height();
	std::vector<unsigned char> out((size_t)W * H * 3);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			float v = g(x, y);
			unsigned char c = (unsigned char)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f);
			size_t idx = ((size_t)y * W + x) * 3;
			out[idx + 0] = c;
			out[idx + 1] = c;
			out[idx + 2] = c;
		}
	return out;
}

static std::vector<unsigned char> biomeToRGB(const Grid2D<Biome>& g) {
	int W = g.width(), H = g.height();
	std::vector<unsigned char> out((size_t)W * H * 3);
	auto colorOf = [](Biome b) -> std::array<unsigned char, 3> {
		switch (b) {
			case Biome::Ocean:
				return {24, 64, 160};
			case Biome::Beach:
				return {238, 214, 175};
			case Biome::Lake:
				return {36, 120, 200};
			case Biome::Mangrove:
				return {31, 90, 42};
			case Biome::Desert:
				return {210, 180, 140};
			case Biome::Savanna:
				return {189, 183, 107};
			case Biome::Grassland:
				return {130, 200, 80};
			case Biome::TropicalRainforest:
				return {16, 120, 45};
			case Biome::SeasonalForest:
				return {34, 139, 34};
			case Biome::BorealForest:
				return {80, 120, 70};
			case Biome::Tundra:
				return {180, 190, 200};
			case Biome::Snow:
				return {240, 240, 250};
			case Biome::Rocky:
				return {140, 130, 120};
			case Biome::Mountain:
				return {120, 120, 140};
			case Biome::Swamp:
				return {34, 85, 45};
			default:
				return {255, 0, 255};
		}
	};
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			auto c = colorOf(g(x, y));
			size_t idx = ((size_t)y * W + x) * 3;
			out[idx + 0] = c[0];
			out[idx + 1] = c[1];
			out[idx + 2] = c[2];
		}
	return out;
}

// Map Biome enum -> biomes.json id string (must match the ids used in your biomes.json)
static std::string biomeEnumToId(Biome b) {
	switch (b) {
		case Biome::Ocean:
			return "Ocean";
		case Biome::Beach:
			return "Beach";
		case Biome::Lake:
			return "Lake";
		case Biome::Mangrove:
			return "Mangrove";
		case Biome::Desert:
			return "Desert";
		case Biome::Savanna:
			return "Savanna";
		case Biome::Grassland:
			return "Grassland";
		case Biome::TropicalRainforest:
			return "Tropical Rainforest";
		case Biome::SeasonalForest:
			return "Seasonal Forest";
		case Biome::BorealForest:
			return "Boreal Forest";
		case Biome::Tundra:
			return "Tundra";
		case Biome::Snow:
			return "Snow";
		case Biome::Rocky:
			return "Rocky";
		case Biome::Mountain:
			return "Mountain";
		case Biome::Swamp:
			return "Swamp";
		default:
			return "Unknown";
	}
}

int main(int argc, char** argv) {
	// --- DEBUG: show working directory and config path (Windows-friendly) ---
	char cwdBuf[4096];
	if (_getcwd(cwdBuf, sizeof(cwdBuf)) != nullptr) {
		std::cerr << "[DEBUG] CWD = " << cwdBuf << std::endl;
	} else {
		std::cerr << "[DEBUG] CWD unknown" << std::endl;
	}

	std::string cfgRelPath = "../../assets/config.json";
	std::filesystem::path absCfg = std::filesystem::absolute(cfgRelPath);
	std::cerr << "[DEBUG] Looking for config at (relative): " << cfgRelPath << std::endl;
	std::cerr << "[DEBUG] Absolute config path: " << absCfg.string() << std::endl;
	std::cerr << "[DEBUG] Exists? " << (std::filesystem::exists(absCfg) ? "YES" : "NO") << std::endl;

	std::ifstream f(absCfg.string());
	if (!f) {
		std::cerr << "[ERROR] Missing config.json at: " << absCfg.string() << std::endl;
		std::cerr << "[HINT] Run binary from project root or copy assets to build\\bin\\assets" << std::endl;
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

	// print a few keys
	std::cerr << "[DEBUG] Loaded config keys: ";
	for (auto it = cfg.begin(); it != cfg.end(); ++it) std::cerr << it.key() << " ";
	std::cerr << std::endl;

	// ---------- Additional precise debug around grids & world construction ----------
	int W = cfg.value("width", 512);
	int H = cfg.value("height", 512);
	uint32_t seed = cfg.value("seed", 424242u);

	std::cerr << "[DEBUG] Config values: W=" << W << " H=" << H << " seed=" << seed << std::endl;
	Grid2D<float> height(W, H), temp(W, H), moist(W, H);
	Grid2D<uint8_t> rivers(W, H);
	Grid2D<Biome> biomeMap(W, H);

	// create grids inside try/catch to detect constructor failures
	try {
		std::cerr << "[DEBUG] About to construct Grid2D objects (height,temp,moist,rivers,biomeMap)" << std::endl;
		std::cerr << "[DEBUG] Constructed Grid2D objects successfully" << std::endl;

		// Voronoi config and world inside try/catch
		std::cerr << "[DEBUG] Creating VoronoiConfig and WorldType_Voronoi (constructor)" << std::endl;
		VoronoiConfig vcfg;
		vcfg.seed = seed;
		vcfg.numPlates = cfg.value("numPlates", 36);
		vcfg.fbmBlend = cfg.value("fbmBlend", 0.42f);
		vcfg.fbmFrequency = cfg.value("fbmFrequency", 0.0035f);
		vcfg.fbmOctaves = cfg.value("fbmOctaves", 5);

		// Construct world object
		WorldType_Voronoi world(W, H, vcfg);
		std::cerr << "[DEBUG] WorldType_Voronoi constructed OK" << std::endl;

		std::cerr << "[DEBUG] About to call world.generate()" << std::endl;
		world.generate(height);
		std::cerr << "[DEBUG] world.generate() returned OK" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "[EXCEPTION] during grid/world construction: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "[EXCEPTION] unknown error during grid/world construction" << std::endl;
		return 1;
	}
	// ---------- end precise debug block ----------

	// save height before erosion
	std::filesystem::create_directories("out");
	auto hRGB_before = heightToRGB(height);
	if (!writePPM("out/height_before_erosion.ppm", W, H, hRGB_before)) std::cerr << "Failed write out/height_before_erosion.ppm\n";

	// temp & moisture using PerlinNoise
	PerlinNoise pTemp(seed ^ 0xA5A5A5);
	PerlinNoise pMoist(seed ^ 0x5A5A5A);
	float baseFreq = 0.0025f;

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
	std::cerr << "[DEBUG] Computed temp & moisture maps" << std::endl;

	// load biomes JSON if present
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

	// classify before erosion (optional snapshot)
	std::cerr << "[DEBUG] Running initial biome classification (pre-erosion)" << std::endl;

	bool ok_pre = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_pre) {
		std::cerr << "Classification failed (dimension mismatch)\n";
		return 1;
	}
	auto bRGB_pre = biomeToRGB(biomeMap);
	if (!writePPM("out/biome_before_erosion.ppm", W, H, bRGB_pre)) std::cerr << "Failed write out/biome_before_erosion.ppm\n";

	std::cerr << "[DEBUG] Starting hydraulic erosion stage" << std::endl;

	// -----------------------------
	// Run hydraulic erosion stage
	// -----------------------------
	ErosionParams eparams;
	eparams.worldSeed = seed;
	// scale number of droplets with map size; these are sensible defaults but tweak as needed
	eparams.numDroplets = std::max(1000, (int)(0.4f * W * H));
	eparams.maxSteps = 45;
	eparams.stepSize = 1.0f;
	eparams.capacityFactor = 8.0f;
	eparams.erodeRate = 0.5f;
	eparams.depositRate = 0.3f;
	eparams.evaporateRate = 0.015f;
	// run erosion (produces erode/deposit maps and modifies height in-place)
	Grid2D<float> erodeMap(W, H), depositMap(W, H);
	auto stats = erosion::runHydraulicErosion(height, eparams, &erodeMap, &depositMap);

	std::cout << "[EROSION] totalEroded=" << stats.totalEroded << " totalDeposited=" << stats.totalDeposited << " droplets=" << stats.appliedDroplets
			  << std::endl;

	std::cerr << "[DEBUG] Hydraulic erosion finished" << std::endl;
	std::cerr << "[EROSION] totalEroded=" << stats.totalEroded << " totalDeposited=" << stats.totalDeposited << " droplets=" << stats.appliedDroplets
			  << std::endl;

	// save erosion outputs (PPM grayscale)
	auto erodedRGB = heightToRGB(erodeMap);
	auto depositRGB = heightToRGB(depositMap);
	auto hRGB_after = heightToRGB(height);
	if (!writePPM("out/erosion_eroded.ppm", W, H, erodedRGB)) std::cerr << "Failed write out/erosion_eroded.ppm\n";
	if (!writePPM("out/erosion_deposited.ppm", W, H, depositRGB)) std::cerr << "Failed write out/erosion_deposited.ppm\n";
	if (!writePPM("out/height_after_erosion.ppm", W, H, hRGB_after)) std::cerr << "Failed write out/height_after_erosion.ppm\n";

	// classify after erosion (pre-rivers) and save that snapshot
	std::cerr << "[DEBUG] Classifying biome map AFTER hydraulic erosion (pre-rivers)" << std::endl;
	bool ok_after_erosion = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_after_erosion) {
		std::cerr << "[ERROR] Classification failed after erosion (dimension mismatch)\n";
	} else {
		auto bRGB_after_erosion = biomeToRGB(biomeMap);
		if (!writePPM("out/biome_after_erosion.ppm", W, H, bRGB_after_erosion)) std::cerr << "Failed write out/biome_after_erosion.ppm\n";
	}

	// -----------------------------
	// River generation stage
	// -----------------------------
	std::cerr << "[DEBUG] Starting river generation stage" << std::endl;

	// Convert Grid2D -> linear vector for RiverGenerator
	std::vector<float> heightVec = gridToVector(height);

	// set up river parameters (tune these to your map size)
	RiverParams rparams;
	rparams.flow_accum_threshold = (W >= 2048 ? 4000.0 : (W >= 1024 ? 1000.0 : 200.0));
	rparams.min_channel_depth = 0.4;
	rparams.max_channel_depth = 6.0;
	rparams.width_multiplier = 0.002;  // scale of channel width (cells per sqrt(flow))
	rparams.carve_iterations = 1;
	rparams.bed_slope_reduction = 0.5;
	rparams.wetland_accum_threshold = 500.0;
	rparams.wetland_slope_max = 0.01;

	RiverGenerator rg(W, H, heightVec);
	rg.run(rparams);

	// retrieve mask and updated heights
	const std::vector<uint8_t>& riverMask = rg.getRiverMask();
	const std::vector<float>& heightAfterRiversVec = rg.getHeightmap();

	// write river mask to PPM (0/255)
	auto riverMaskRGB = maskToRGB(riverMask, W, H);
	if (!writePPM("out/river_map.ppm", W, H, riverMaskRGB)) std::cerr << "Failed to write out/river_map.ppm\n";

	// update Grid2D height with carved results
	vectorToGrid(heightAfterRiversVec, height);

	// write updated heights after rivers
	auto hRGB_after_rivers = heightToRGB(height);
	if (!writePPM("out/height_after_rivers.ppm", W, H, hRGB_after_rivers)) std::cerr << "Failed to write out/height_after_rivers.ppm\n";

	std::cerr << "[DEBUG] River generation finished; wrote out/river_map.ppm and out/height_after_rivers.ppm" << std::endl;

	// --- classify & save biome map AFTER river carving ---
	std::cerr << "[DEBUG] Classifying biome map AFTER river carving" << std::endl;
	bool ok_after_rivers = biome::classifyBiomeMap(height, temp, moist, nullptr, defs, biomeMap, opts);
	if (!ok_after_rivers) {
		std::cerr << "[ERROR] Classification failed after rivers (dimension mismatch)\n";
	} else {
		auto bRGB_after_rivers = biomeToRGB(biomeMap);
		if (!writePPM("out/biome_after_rivers.ppm", W, H, bRGB_after_rivers)) std::cerr << "Failed write out/biome_after_rivers.ppm\n";
	}

	// -----------------------------
	// Map utils + Object placement
	// -----------------------------
	std::cerr << "[DEBUG] Computing slope, water mask and coast distances for object placement" << std::endl;

	// compute slope
	std::vector<float> slope;
	map::computeSlopeMap(gridToVector(height), W, H, slope);  // helper computes using a linear array

	// compute water mask (uses ocean/lake thresholds from config)
	std::vector<unsigned char> waterMask;
	float oceanThreshold = cfg.value("oceanHeightThreshold", 0.35f);
	float lakeThreshold = cfg.value("lakeHeightThreshold", 0.45f);
	map::computeWaterMask(gridToVector(height), W, H, oceanThreshold, lakeThreshold, waterMask);

	// compute coast distances (in tiles)
	std::vector<int> coastDist;
	map::computeCoastDistance(waterMask, W, H, coastDist);

	std::unordered_map<std::string, int> biomeIdToIndex;
	for (size_t i = 0; i < defs.size(); ++i) {
		std::string idStr = biomeEnumToId(defs[i].id);
		biomeIdToIndex.insert_or_assign(idStr, (int)i);
	}

	// build linear biome_idx mapping from the grid's Biome enum -> index in defs
	std::vector<int> biome_idx((size_t)W * H, -1);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			int i = y * W + x;
			std::string id = biomeEnumToId(biomeMap(x, y));	 // convert enum to string
			auto it = biomeIdToIndex.find(id);
			if (it != biomeIdToIndex.end())
				biome_idx[i] = it->second;
			else
				biome_idx[i] = -1;
		}
	}

	// load object placement config (object_placement.json)
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

		// create placer and run
		std::cerr << "[DEBUG] Running ObjectPlacer..." << std::endl;
		ObjectPlacer placer(W, H, (float)W);
		placer.loadPlacementConfig(placeCfg);

		// build biome id list (same order as defs vector)
		std::vector<std::string> biomeIds;
		biomeIds.reserve(defs.size());
		for (auto& d : defs) {
			// defs[i].id is a Biome enum; convert to the string id used in your biomes.json
			biomeIds.push_back(biomeEnumToId(d.id));
		}
		placer.setBiomeIdList(biomeIds);

		// height as linear vector
		std::vector<float> heightLinear = gridToVector(height);

		// run placement
		placer.place(heightLinear, slope, waterMask, coastDist, biome_idx);

		// outputs
		placer.writeCSV("out/objects.csv");
		placer.writeDebugPPM("out/objects_map.ppm");
		std::cerr << "[DEBUG] Object placement complete. Wrote out/objects.csv and out/objects_map.ppm\n";
	}

	// -----------------------------
	// Final outputs (height + biome)
	// -----------------------------
	std::cerr << "[DEBUG] Saving final outputs to out/ ..." << std::endl;
	auto hRGB = heightToRGB(height);
	auto bRGB = biomeToRGB(biomeMap);
	if (!writePPM("out/height.ppm", W, H, hRGB)) std::cerr << "Failed write height\n";
	if (!writePPM("out/biome.ppm", W, H, bRGB)) std::cerr << "Failed write biome\n";

	std::cout << "Wrote out/height.ppm and out/biome.ppm (and erosion/river outputs)\n";
	return 0;
}
