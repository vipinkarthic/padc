#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct RiverParams {
	double flow_accum_threshold = 1000.0;
	double min_channel_depth = 0.5;
	double max_channel_depth = 8.0;
	double width_multiplier = 0.002;
	int carve_iterations = 1;
	double bed_slope_reduction = 0.5;
	double wetland_accum_threshold = 500.0;
	double wetland_slope_max = 0.01;
};

class RiverGenerator {
   public:
	// height: row-major W*H floats (elevation units)
	// biome: optional (same size) - not required; pass empty vector if unused
	RiverGenerator(int width, int height, const std::vector<float>& heightmap, const std::vector<int>& biome_map = {});

	// runs algorithm and modifies internal heightmap in-place
	void run(const RiverParams& params);

	// get results
	const std::vector<uint8_t>& getRiverMask() const;  // 0 or 255
	const std::vector<float>& getHeightmap() const;

	// write river map to PNG (implement ImageWriter in your project)
	void writeRiverPNG(const std::string& path) const;

   private:
	int W, H;
	std::vector<float> Hmap;
	std::vector<int> Biomes;
	std::vector<int> FlowDir;  // index of downslope neighbor or -1
	std::vector<float> FlowAccum;
	std::vector<uint8_t> RiverMask;

	inline int idx(int x, int y) const { return y * W + x; }
	void computeFlowDirection();
	void computeFlowAccumulation();
	void extractRivers(const RiverParams& params);
	void carveRivers(const RiverParams& params);
};
