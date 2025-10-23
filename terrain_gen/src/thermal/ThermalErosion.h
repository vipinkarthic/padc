// #pragma once
// #include <vector>

// struct ThermalParams {
// 	int iterations = 30;
// 	double talus = 0.03;		  // slope threshold (height units per cell)
// 	double transfer_coeff = 0.5;  // fraction of excess moved each iteration [0..1]
// 	double min_diff = 1e-6;		  // ignore tiny diffs
// };

// class ThermalEroder {
//    public:
// 	ThermalEroder(int width, int height, std::vector<float>& height_linear);

// 	// run thermal erosion in-place on the internal linear height buffer
// 	// After run(), the internal heights are updated; to sync back to Grid2D call getHeightVector()
// 	void run(const ThermalParams& params);

// 	// returns reference to internal linear height vector (row-major)
// 	const std::vector<float>& getHeightVector() const;

//    private:
// 	int W, H, N;
// 	std::vector<float>& Hmap;  // reference to a vector you pass (so you can reuse memory)
// };

#pragma once
#include <vector>

#include "Types.h"	// for Grid2D

struct ThermalParams {
	int iterations = 20;
	float talus = 0.02f;	  // angle-of-repose threshold in height units
	float relaxation = 0.5f;  // fraction of transferable excess moved each iteration (0..1)
	bool use_diagonal_distance = true;
};

namespace erosion {
// In-place: modifies 'height' Grid2D
// returns total_moved (sum of absolute material moved)
double runThermalErosion(Grid2D<float>& height, const ThermalParams& params);
}  // namespace erosion
