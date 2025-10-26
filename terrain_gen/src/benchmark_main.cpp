#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>

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

struct BenchmarkResult {
    std::string resolution;
    int threads;
    int run_id;
    std::map<std::string, double> stage_times;
    double total_time;
    size_t peak_memory_kb;
    std::map<std::string, double> speedup_per_stage;
    double total_speedup;
};

class MemoryTracker {
private:
    size_t initial_memory;
    size_t peak_memory;
    
public:
    MemoryTracker() {
        initial_memory = getCurrentMemoryUsage();
        peak_memory = initial_memory;
    }
    
    void update() {
        size_t current = getCurrentMemoryUsage();
        if (current > peak_memory) {
            peak_memory = current;
        }
    }
    
    size_t getPeakMemoryKB() const {
        return (peak_memory - initial_memory) / 1024;
    }
    
private:
    size_t getCurrentMemoryUsage() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            return usage.ru_maxrss * 1024; // ru_maxrss is in KB
        }
        return 0;
#endif
    }
};

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return duration.count() / 1000000.0; // Convert to seconds
    }
};

void setOpenMPThreads(int num_threads) {
    omp_set_num_threads(num_threads);
}

BenchmarkResult runBenchmark(int width, int height, int num_threads, int run_id, uint32_t seed) {
    BenchmarkResult result;
    result.resolution = std::to_string(width) + "x" + std::to_string(height);
    result.threads = num_threads;
    result.run_id = run_id;
    
    // Set OpenMP threads
    setOpenMPThreads(num_threads);
    
    // Initialize memory tracker
    MemoryTracker memory_tracker;
    Timer total_timer;
    total_timer.start();
    
    // Create grids
    Grid2D<float> heightMap(width, height), temp(width, height), moist(width, height);
    Grid2D<uint8_t> rivers(width, height);
    Grid2D<Biome> biomeMap(width, height);
    
    // Stage 1: Heightmap generation and Voronoi generation
    Timer stage_timer;
    stage_timer.start();
    memory_tracker.update();
    
    try {
        VoronoiConfig vcfg;
        vcfg.seed = seed;
        vcfg.numPlates = 36;
        vcfg.fbmBlend = 0.42f;
        vcfg.fbmFrequency = 0.0035f;
        vcfg.fbmOctaves = 5;

        WorldType_Voronoi world(width, height, vcfg);
        world.generate(heightMap);
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] during grid/world construction: " << e.what() << std::endl;
        return result;
    }
    
    result.stage_times["heightmap_and_voronoi"] = stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 2: Temperature and moisture generation (part of heightmap generation)
    stage_timer.start();
    
    PerlinNoise pTemp(seed ^ 0xA5A5A5);
    PerlinNoise pMoist(seed ^ 0x5A5A5A);
    float baseFreq = 0.0025f;

#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            float e = heightMap(x, y);
            float fx = (float)x;
            float fy = (float)y;
            float t = pTemp.fbm(fx + 100.0f, fy + 100.0f, baseFreq * 1.2f, 4, 2.0f, 0.6f);
            t = (t + 1.0f) * 0.5f;
            float latFactor = 1.0f - fabsf(((float)y / (float)height) * 2.0f - 1.0f);
            t = t * 0.6f + 0.4f * latFactor;
            temp(x, y) = std::clamp(t, 0.0f, 1.0f);

            float m = pMoist.fbm(fx - 100.0f, fy - 100.0f, baseFreq * 1.5f, 4, 2.0f, 0.6f);
            m = (m + 1.0f) * 0.5f;
            m = m * (0.6f + (1.0f - e) * 0.4f);
            moist(x, y) = std::clamp(m, 0.0f, 1.0f);
        }
    
    result.stage_times["heightmap_and_voronoi"] += stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 3: Biome classification (before erosion)
    stage_timer.start();
    
    // Use default biome definitions for benchmark
    std::vector<BiomeDef> defs = DEFAULT_BIOMES();

    biome::ClassifierOptions opts;
    opts.coastDistanceTiles = 3;
    opts.oceanHeightThreshold = 0.35f;
    opts.lakeHeightThreshold = 0.45f;
    opts.smoothingIterations = 1;

    bool ok_pre = biome::classifyBiomeMap(heightMap, temp, moist, nullptr, defs, biomeMap, opts);
    if (!ok_pre) {
        std::cerr << "Classification failed (dimension mismatch)\n";
        return result;
    }
    
    // Save biome map and heightmap before erosion
    Grid2D<Biome> biomeMap_before_erosion = biomeMap;
    Grid2D<float> heightMap_before_erosion = heightMap;
    
    result.stage_times["biome_classification"] = stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 4: Hydraulic erosion
    stage_timer.start();
    
    ErosionParams eparams;
    eparams.worldSeed = seed;
    eparams.numDroplets = std::max(1000, (int)(0.4f * width * height));
    eparams.maxSteps = 45;
    eparams.stepSize = 1.0f;
    eparams.capacityFactor = 8.0f;
    eparams.erodeRate = 0.5f;
    eparams.depositRate = 0.3f;
    eparams.evaporateRate = 0.015f;
    Grid2D<float> erodeMap(width, height), depositMap(width, height);
    auto stats = erosion::runHydraulicErosion(heightMap, eparams, &erodeMap, &depositMap);
    
    result.stage_times["hydraulic_erosion"] = stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 4.5: Biome classification (after erosion)
    stage_timer.start();
    
    bool ok_after_erosion = biome::classifyBiomeMap(heightMap, temp, moist, nullptr, defs, biomeMap, opts);
    if (!ok_after_erosion) {
        std::cerr << "[ERROR] Classification failed after erosion (dimension mismatch)\n";
    }
    
    // Save biome map and heightmap after erosion
    Grid2D<Biome> biomeMap_after_erosion = biomeMap;
    Grid2D<float> heightMap_after_erosion = heightMap;
    
    result.stage_times["biome_classification"] += stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 5: River generation
    stage_timer.start();
    
    std::vector<float> heightVec = helper::gridToVector(heightMap);

    RiverParams rparams;
    rparams.flow_accum_threshold = (width >= 2048 ? 4000.0 : (width >= 1024 ? 1000.0 : 200.0));
    rparams.min_channel_depth = 0.4;
    rparams.max_channel_depth = 6.0;
    rparams.width_multiplier = 0.002;
    rparams.carve_iterations = 1;
    rparams.bed_slope_reduction = 0.5;
    rparams.wetland_accum_threshold = 500.0;
    rparams.wetland_slope_max = 0.01;

    RiverGenerator rg(width, height, heightVec);
    rg.run(rparams);

    const std::vector<uint8_t>& riverMask = rg.getRiverMask();
    const std::vector<float>& heightAfterRiversVec = rg.getHeightmap();
    helper::vectorToGrid(heightAfterRiversVec, heightMap);
    
    result.stage_times["river_generation"] = stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 6: Final biome classification
    stage_timer.start();
    
    bool ok_after_rivers = biome::classifyBiomeMap(heightMap, temp, moist, nullptr, defs, biomeMap, opts);
    if (!ok_after_rivers) {
        std::cerr << "[ERROR] Classification failed after rivers (dimension mismatch)\n";
    }
    
    // Save biome map and heightmap after rivers
    Grid2D<Biome> biomeMap_after_rivers = biomeMap;
    Grid2D<float> heightMap_after_rivers = heightMap;
    
    result.stage_times["biome_classification"] += stage_timer.elapsed();
    memory_tracker.update();
    
    // Stage 7: Object placement
    stage_timer.start();
    
    std::vector<float> slope;
    map::computeSlopeMap(helper::gridToVector(heightMap), width, height, slope);

    std::vector<unsigned char> waterMask;
    float oceanThreshold = 0.35f;
    float lakeThreshold = 0.45f;
    map::computeWaterMask(helper::gridToVector(heightMap), width, height, oceanThreshold, lakeThreshold, waterMask);

    std::vector<int> coastDist;
    map::computeCoastDistance(waterMask, width, height, coastDist);

    std::unordered_map<std::string, int> biomeIdToIndex;
    for (size_t i = 0; i < defs.size(); ++i) {
        std::string idStr = biomeToString(defs[i].id);
        biomeIdToIndex.insert_or_assign(idStr, (int)i);
    }

    std::vector<int> biome_idx((size_t)width * height, -1);
#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            std::string id = biomeToString(biomeMap(x, y));
            auto it = biomeIdToIndex.find(id);
            if (it != biomeIdToIndex.end())
                biome_idx[i] = it->second;
            else
                biome_idx[i] = -1;
        }
    }

    std::filesystem::path placementPath = std::filesystem::absolute("../../assets/object_placement.json");
    if (std::filesystem::exists(placementPath)) {
        std::ifstream pf(placementPath.string());
        json placeCfg;
        try {
            pf >> placeCfg;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to parse object_placement.json: " << e.what() << std::endl;
        }
        pf.close();

        ObjectPlacer placer(width, height, (float)width);
        placer.loadPlacementConfig(placeCfg);

        std::vector<std::string> biomeIds;
        biomeIds.reserve(defs.size());
        for (auto& d : defs) {
            biomeIds.push_back(biomeToString(d.id));
        }
        placer.setBiomeIdList(biomeIds);

        std::vector<float> heightLinear = helper::gridToVector(heightMap);
        placer.place(heightLinear, slope, waterMask, coastDist, biome_idx);
    }
    
    result.stage_times["object_placement"] = stage_timer.elapsed();
    memory_tracker.update();
    
    // Calculate total time and memory
    result.total_time = total_timer.elapsed();
    result.peak_memory_kb = memory_tracker.getPeakMemoryKB();
    
    // Generate output images for documentation
    std::string output_dir = "benchmark_output/" + result.resolution + "/" + std::to_string(num_threads) + "_threads/run_" + std::to_string(run_id);
    std::filesystem::create_directories(output_dir);
    
    // Generate heightmap images
    auto hRGB_before = helper::heightToRGB(heightMap_before_erosion);
    auto hRGB_after_erosion = helper::heightToRGB(heightMap_after_erosion);
    auto hRGB_after_rivers = helper::heightToRGB(heightMap_after_rivers);
    
    helper::writePPM(output_dir + "/01_height_before_erosion.ppm", width, height, hRGB_before);
    helper::writePPM(output_dir + "/02_height_after_erosion.ppm", width, height, hRGB_after_erosion);
    helper::writePPM(output_dir + "/03_height_after_rivers.ppm", width, height, hRGB_after_rivers);
    
    // Generate biome images
    auto bRGB_before = helper::biomeToRGB(biomeMap_before_erosion);
    auto bRGB_after_erosion = helper::biomeToRGB(biomeMap_after_erosion);
    auto bRGB_after_rivers = helper::biomeToRGB(biomeMap_after_rivers);
    helper::writePPM(output_dir + "/04_biome_before_erosion.ppm", width, height, bRGB_before);
    helper::writePPM(output_dir + "/05_biome_after_erosion.ppm", width, height, bRGB_after_erosion);
    helper::writePPM(output_dir + "/06_biome_after_rivers.ppm", width, height, bRGB_after_rivers);
    
    // Generate erosion images
    auto erodedRGB = helper::heightToRGB(erodeMap);
    auto depositRGB = helper::heightToRGB(depositMap);
    helper::writePPM(output_dir + "/07_erosion_eroded.ppm", width, height, erodedRGB);
    helper::writePPM(output_dir + "/08_erosion_deposited.ppm", width, height, depositRGB);
    
    // Generate river images
    auto riverMaskRGB = helper::maskToRGB(riverMask, width, height);
    helper::writePPM(output_dir + "/09_river_map.ppm", width, height, riverMaskRGB);
    
    // Generate final images
    auto hRGB_final = helper::heightToRGB(heightMap);
    auto bRGB_final = helper::biomeToRGB(biomeMap);
    helper::writePPM(output_dir + "/10_height_final.ppm", width, height, hRGB_final);
    helper::writePPM(output_dir + "/11_biome_final.ppm", width, height, bRGB_final);
    
    return result;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <width> <height> <threads> [run_id] [seed]" << std::endl;
        return 1;
    }
    
    int width = std::atoi(argv[1]);
    int height = std::atoi(argv[2]);
    int threads = std::atoi(argv[3]);
    int run_id = (argc > 4) ? std::atoi(argv[4]) : 1;
    uint32_t seed = (argc > 5) ? std::atoi(argv[5]) : 424242u;
    
    // Run benchmark
    BenchmarkResult result = runBenchmark(width, height, threads, run_id, seed);
    
    // Output results as JSON
    json output;
    output["resolution"] = result.resolution;
    output["threads"] = result.threads;
    output["run_id"] = result.run_id;
    output["stage_times"] = result.stage_times;
    output["total_time"] = result.total_time;
    output["peak_memory_kb"] = result.peak_memory_kb;
    
    std::cout << output.dump(4) << std::endl;
    
    return 0;
}
