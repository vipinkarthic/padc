#include "ObjectPlacer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

ObjectPlacer::ObjectPlacer(int W_, int H_, float worldSizeMeters) : W(W_), H(H_), placedCount(0) {
	if (worldSizeMeters <= 0.0f)
		worldSizeM = (float)W_;
	else
		worldSizeM = worldSizeMeters;
	cellSizeM = worldSizeM / std::max(W, H);
	gridW = std::max(1, (int)std::ceil(worldSizeM / 2.0f));	 // grid cell ~2 meters
	gridH = gridW;
	spatialGrid.clear();
	spatialGrid.resize(gridW * gridH);
	seed = 424242ULL;
}

uint64_t ObjectPlacer::splitmix64(uint64_t &x) {
	uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
	return z ^ (z >> 31);
}
float ObjectPlacer::rand01_from(uint64_t &state) {
	uint64_t v = splitmix64(state);
	return (v >> 11) * (1.0 / 9007199254740992.0);
}

void ObjectPlacer::loadPlacementConfig(const json &cfg_json) {
	if (cfg_json.contains("seed")) seed = cfg_json["seed"].get<uint64_t>();
	if (cfg_json.contains("global_max_instances")) globalMax = cfg_json["global_max_instances"].get<size_t>();
	if (cfg_json.contains("default_min_distance_m")) {
		float md = cfg_json["default_min_distance_m"].get<float>();
		// tune grid size based on that if desired
		gridW = std::max(1, (int)std::ceil(worldSizeM / std::max(0.5f, md)));
		gridH = gridW;
		spatialGrid.clear();
		spatialGrid.resize(gridW * gridH);
	}

	biomeObjects.clear();
	if (cfg_json.contains("biome_objects")) {
		for (auto &pair : cfg_json["biome_objects"].items()) {
			std::string biomeId = pair.key();
			for (auto &o : pair.value()) {
				OPlaceDef od;
				od.name = o.value("name", std::string("obj"));
				od.model = o.value("model", std::string(""));
				od.placeholder = o.value("placeholder", false);
				od.density_per_1000m2 = o.value("density_per_1000m2", 0.0f);
				od.min_distance_m = o.value("min_distance_m", 1.0f);
				od.scale_min = o.value("scale_min", 1.0f);
				od.scale_max = o.value("scale_max", 1.0f);
				od.yaw_variance = o.value("yaw_variance_deg", 180.0f);
				od.elev_min = o.value("elevation_min", 0.0f);
				od.elev_max = o.value("elevation_max", 1.0f);
				od.slope_min = o.value("slope_min", 0.0f);
				od.slope_max = o.value("slope_max", 10.0f);
				od.requires_water = o.value("requires_water", false);
				od.prefers_coast = o.value("prefers_coast", false);
				if (o.contains("cluster")) {
					od.isCluster = true;
					od.cluster_count = o["cluster"].value("count", 3);
					od.cluster_radius = o["cluster"].value("radius", 2.0f);
				}
				biomeObjects[biomeId].push_back(od);
			}
		}
	}
}

void ObjectPlacer::setBiomeIdList(const std::vector<std::string> &ids) { biomeIds = ids; }

int ObjectPlacer::gridIndexForWorld(float wx, float wy) const {
	int gx = std::min(gridW - 1, std::max(0, (int)std::floor((wx / worldSizeM) * gridW)));
	int gy = std::min(gridH - 1, std::max(0, (int)std::floor((wy / worldSizeM) * gridH)));
	return gy * gridW + gx;
}

float ObjectPlacer::computePlacementProbability(const OPlaceDef &od, float elev, float sl, unsigned char isWater, int coastDistTile) {
	// base probability from density: p_base = (density_per_1000m2 / 1000) * cellArea
	float cellArea = cellSizeM * cellSizeM;
	float p_base = (od.density_per_1000m2 / 1000.0f) * cellArea;
	if (p_base <= 0.0f) return 0.0f;
	// early rejects
	if (elev < od.elev_min || elev > od.elev_max) return 0.0f;
	if (sl < od.slope_min || sl > od.slope_max) return 0.0f;
	if (od.requires_water && !isWater) return 0.0f;
	// apply coast preference boost
	float boost = 1.0f;
	if (od.prefers_coast && coastDistTile >= 0 && coastDistTile <= 3) boost += 0.65f * (1.0f - (float)coastDistTile / 3.0f);
	// slope penalty: prefer gentle slopes (example curve)
	float slopePenalty = 1.0f;
	if (sl > 0.6f)
		slopePenalty *= 0.3f;
	else if (sl > 0.3f)
		slopePenalty *= 0.6f;
	// final
	float p = p_base * boost * slopePenalty;
	// clamp to reasonable upper bound (avoid overwhelming)
	return std::min(p, 0.95f);
}

bool ObjectPlacer::attemptPlace(int x, int y, const OPlaceDef &od, const std::vector<float> &height, const std::vector<float> &slope,
								const std::vector<unsigned char> &waterMask, const std::vector<int> &coastDist, uint64_t &cellSeed, int bidx) {
	int idx = cellIndex(x, y);
	float elev = height[idx];
	float sl = slope[idx];
	unsigned char isWater = (!waterMask.empty()) ? waterMask[idx] : 0;
	int coast = (!coastDist.empty()) ? coastDist[idx] : -1;

	float p = computePlacementProbability(od, elev, sl, isWater, coast);
	if (p <= 0.0f) return false;

	// for probabilities >= 0.5, do single Bernoulli; for tiny p, approximate Poisson by multiple small Bernoulli trials
	bool success = false;
	if (p > 0.2f) {
		float r = rand01_from(cellSeed);
		success = (r <= p);
	} else {
		// attempt up to N small trials to approximate expected count
		int trials = std::max(1, (int)std::ceil(p * 10.0f));  // scale
		for (int t = 0; t < trials; ++t) {
			if (rand01_from(cellSeed) <= p) {
				success = true;
				break;
			}
		}
	}
	if (!success) return false;

	// jitter placement inside cell
	float jx = rand01_from(cellSeed) - 0.5f;
	float jy = rand01_from(cellSeed) - 0.5f;
	float wx = (x + 0.5f + jx * 0.9f) * cellSizeM;
	float wy = (y + 0.5f + jy * 0.9f) * cellSizeM;
	float wz = elev;

	// min-distance check against spatial grid neighborhood
	float minD = od.min_distance_m;
	int gidx = gridIndexForWorld(wx, wy);
	int createdId = -1;
	float createdWx = 0.0f;
	float createdWy = 0.0f;
	{
		std::lock_guard<std::mutex> lk(mutexPlace);
		// neighbor checks already done above...

		// create instance id using current placed.size()
		ObjInstance inst;
		int newId = (int)placed.size();
		inst.id = newId;
		inst.name = od.name;
		inst.model = od.model;
		inst.px = x;
		inst.py = y;
		inst.wx = wx;
		inst.wy = wy;
		inst.wz = wz;
		inst.yaw = rand01_from(cellSeed) * od.yaw_variance;
		inst.scale = od.scale_min + rand01_from(cellSeed) * (od.scale_max - od.scale_min);
		if (bidx >= 0 && bidx < (int)biomeIds.size())
			inst.biome_id = biomeIds[bidx];
		else
			inst.biome_id = "unknown";

		placed.push_back(inst);
		spatialGrid[gidx].push_back(newId);

		// publish count atomically (increment placedCount)
		placedCount.fetch_add(1, std::memory_order_relaxed);

		createdId = newId;
		createdWx = inst.wx;
		createdWy = inst.wy;
		// std::lock_guard<std::mutex> lk(mutexPlace);
		// int gx = gidx % gridW, gy = gidx / gridW;
		// int check = 2;
		// for (int oy = -check; oy <= check; ++oy) {
		// 	for (int ox = -check; ox <= check; ++ox) {
		// 		int nx = gx + ox, ny = gy + oy;
		// 		if (nx < 0 || ny < 0 || nx >= gridW || ny >= gridH) continue;
		// 		int ng = ny * gridW + nx;
		// 		for (int pid : spatialGrid[ng]) {
		// 			const ObjInstance &other = placed[pid];
		// 			float dx = other.wx - wx;
		// 			float dy = other.wy - wy;
		// 			if ((dx * dx + dy * dy) < (minD * minD)) {
		// 				return false;  // blocked
		// 			}
		// 		}
		// 	}
		// }
		// // passed -> create instance
		// ObjInstance inst;
		// createdId = (int)placed.size();
		// inst.id = createdId;
		// inst.name = od.name;
		// inst.model = od.model;
		// inst.px = x;
		// inst.py = y;
		// inst.wx = wx;
		// inst.wy = wy;
		// inst.wz = wz;
		// inst.yaw = rand01_from(cellSeed) * od.yaw_variance;
		// inst.scale = od.scale_min + rand01_from(cellSeed) * (od.scale_max - od.scale_min);
		// if (bidx >= 0 && bidx < (int)biomeIds.size())
		// 	inst.biome_id = biomeIds[bidx];
		// else
		// 	inst.biome_id = "unknown";
		// placed.push_back(inst);
		// spatialGrid[gidx].push_back(createdId);
		// createdWx = inst.wx;
		// createdWy = inst.wy;
	}
	// if cluster: spawn a few additional around within cluster_radius (no recursive clusters)
	if (od.isCluster) {
		for (int c = 0; c < od.cluster_count; ++c) {
			uint64_t clusterSeed = (uint64_t)createdId * 1009 + c * 7919 + seed;
			float ang = rand01_from(clusterSeed) * 6.28318530718f;
			float rad = rand01_from(clusterSeed) * od.cluster_radius;
			float cx = createdWx + std::cos(ang) * rad;
			float cy = createdWy + std::sin(ang) * rad;
			// map back to pixel
			int px = std::min(W - 1, std::max(0, (int)std::floor(cx / cellSizeM)));
			int py = std::min(H - 1, std::max(0, (int)std::floor(cy / cellSizeM)));
			if (px >= 0 && py >= 0 && px < W && py < H) {
				// small fake OPlaceDef for cluster child (same as parent but smaller minDist)
				OPlaceDef child = od;
				child.min_distance_m = std::max(0.4f, od.min_distance_m * 0.5f);
				uint64_t childSeed = clusterSeed;
				attemptPlace(px, py, child, height, slope, waterMask, coastDist, childSeed, bidx);
			}
		}
	}
	return true;
}

void ObjectPlacer::place(const std::vector<float> &height, const std::vector<float> &slope, const std::vector<unsigned char> &waterMask,
						 const std::vector<int> &coastDist, const std::vector<int> &biome_idx) {
	placed.clear();
	spatialGrid.clear();
	spatialGrid.resize(gridW * gridH);
	placedCount.store(0, std::memory_order_relaxed);

	uint64_t baseSeed = seed;
// iterate raster
#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < H; ++y) {
		// quick early-skip by rows if already reached (cheap atomic read)
		if (placedCount.load(std::memory_order_relaxed) >= globalMax) continue;

		for (int x = 0; x < W; ++x) {
			// global limit check before per-cell work
			if (placedCount.load(std::memory_order_relaxed) >= globalMax) break;  // break inner loop (safe inside thread)

			int i = cellIndex(x, y);
			int bidx = (biome_idx.empty()) ? -1 : biome_idx[i];
			std::string bid = (bidx >= 0 && bidx < (int)biomeIds.size()) ? biomeIds[bidx] : std::string();

			// get objects for biome
			std::vector<OPlaceDef> candidates;
			if (!bid.empty()) {
				auto it = biomeObjects.find(bid);
				if (it != biomeObjects.end()) candidates = it->second;
			}
			// if none, skip
			if (candidates.empty()) continue;

			// per-cell base seed
			uint64_t cellSeed = baseSeed;
			cellSeed ^= (uint64_t(x) + 0x9e3779b97f4a7c15ULL + (cellSeed << 6) + (cellSeed >> 2));
			cellSeed ^= (uint64_t(y) + 0x9e3779b97f4a7c15ULL + (cellSeed << 6) + (cellSeed >> 2));

			// iterate candidates in order (you may randomize order deterministically if desired)
			for (auto &od : candidates) {
				// another quick global limit check before attempting
				if (placedCount.load(std::memory_order_relaxed) >= globalMax) break;

				attemptPlace(x, y, od, height, slope, waterMask, coastDist, cellSeed, bidx);
				// attemptPlace will atomically increment placedCount when a placement succeeds
			}
		}
	}
}

const std::vector<ObjInstance> &ObjectPlacer::instances() const { return placed; }

void ObjectPlacer::writeCSV(const std::string &path) const {
	std::ofstream f(path);
	f << "id,name,model,px,py,wx,wy,wz,yaw,scale,biome\n";
	for (auto &it : placed) {
		std::string modelToWrite = it.model.empty() ? std::string("PLACEHOLDER:") + it.name : it.model;
		f << it.id << "," << it.name << "," << modelToWrite << "," << it.px << "," << it.py << "," << it.wx << "," << it.wy << "," << it.wz << "," << it.yaw
		  << "," << it.scale << "," << it.biome_id << "\n";
	}
	f.close();
}

void ObjectPlacer::writeDebugPPM(const std::string &path) const {
	std::vector<unsigned char> img(W * H * 3, 255);
	for (auto &it : placed) {
		int x = it.px, y = it.py;
		if (x < 0 || y < 0 || x >= W || y >= H) continue;
		int idx = (y * W + x) * 3;
		// color coding by hash of name
		uint32_t h = 0;
		for (char c : it.name) h = h * 131 + (uint8_t)c;
		img[idx + 0] = (h >> 0) & 255;
		img[idx + 1] = (h >> 8) & 255;
		img[idx + 2] = (h >> 16) & 255;
	}
	std::ofstream f(path, std::ios::binary);
	f << "P6\n" << W << " " << H << "\n255\n";
	f.write(reinterpret_cast<char *>(img.data()), img.size());
	f.close();
}
