#pragma once
#include <string>

#include "ErosionParams.h"
#include "Types.h"

namespace erosion {

struct ErosionStats {
	double totalEroded = 0.0;
	double totalDeposited = 0.0;
	int appliedDroplets = 0;
};

ErosionStats runHydraulicErosion(GridFloat &heightGrid, const ErosionParams &params, GridFloat *outEroded = nullptr, GridFloat *outDeposited = nullptr);

}  // namespace erosion
