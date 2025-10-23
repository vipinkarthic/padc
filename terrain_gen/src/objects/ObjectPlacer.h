#pragma once
#include <cstdint>
#include <json.hpp>
#include <mutex>
#include <string>
#include <vector>

// ObjectPlacer.h (inside class ObjectPlacer)
#include <atomic>
// ...

// Simplified object definition (readable from object_placement.json)
struct OPlaceDef {
	std::string name;
	std::string model;	// empty => placeholder
	bool placeholder = false;
	float density_per_1000m2 = 0.0f;
	float min_distance_m = 1.0f;
	float scale_min = 1.0f, scale_max = 1.0f;
	float yaw_variance = 180.0f;
	float elev_min = 0.0f, elev_max = 1.0f;
	float slope_min = 0.0f, slope_max = 10.0f;
	bool requires_water = false;
	bool prefers_coast = false;
	// optional cluster definition
	bool isCluster = false;
	int cluster_count = 0;
	float cluster_radius = 0.0f;
};

struct ObjInstance {
	uint64_t id;
	std::string name;
	std::string model;
	int px, py;
	float wx, wy, wz;
	float yaw, scale;
	std::string biome_id;
};

class ObjectPlacer {
   public:
	ObjectPlacer(int W, int H, float worldSizeMeters = 0.0f);
	// load object placement config (object_placement.json) and optionally merge with biomes list
	void loadPlacementConfig(const nlohmann::json &cfg_json);

	// provide biome index->id mapping (in same order/indices you used earlier)
	void setBiomeIdList(const std::vector<std::string> &biomeIds);

	// Main placement: requires height (W*H floats 0..1), slope (W*H floats), waterMask (W*H bytes), coastDist (W*H ints), biome_idx (W*H ints)
	void place(const std::vector<float> &height, const std::vector<float> &slope, const std::vector<unsigned char> &waterMask,
			   const std::vector<int> &coastDist, const std::vector<int> &biome_idx);

	const std::vector<ObjInstance> &instances() const;
	void writeCSV(const std::string &path = "out/objects.csv") const;
	void writeDebugPPM(const std::string &path = "out/objects_map.ppm") const;

   private:
	std::atomic<size_t> placedCount;
	int W, H;
	float worldSizeM;
	float cellSizeM;
	uint64_t seed;
	size_t globalMax = 500000;

	// mapping biome id -> vector<defs>
	std::vector<std::string> biomeIds;
	std::unordered_map<std::string, std::vector<OPlaceDef>> biomeObjects;

	// placement storage
	std::vector<ObjInstance> placed;
	std::vector<std::vector<int>> spatialGrid;
	int gridW, gridH;
	std::mutex mutexPlace;

	// RNG
	static uint64_t splitmix64(uint64_t &x);
	static float rand01_from(uint64_t &state);

	int gridIndexForWorld(float wx, float wy) const;
	inline int cellIndex(int x, int y) const { return y * W + x; }
	// decision & attempt
	bool attemptPlace(int x, int y, const OPlaceDef &od, const std::vector<float> &height, const std::vector<float> &slope,
					  const std::vector<unsigned char> &waterMask, const std::vector<int> &coastDist, uint64_t &cellSeed, int bidx);

	// helpers for scoring probability
	float computePlacementProbability(const OPlaceDef &od, float elev, float sl, unsigned char isWater, int coastDistTile);
};
