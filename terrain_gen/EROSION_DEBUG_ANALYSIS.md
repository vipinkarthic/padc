# Analysis: Why Erosion Only Affects Top Left Half of Heightmap

## Problem Description

The hydraulic erosion simulation appears to only affect the top left half of the heightmap, leaving the bottom right half relatively unchanged. This suggests an issue with either:

1. **Droplet Distribution**: Droplets are not being distributed evenly across the terrain
2. **Droplet Movement**: Droplets are not reaching the bottom right area
3. **Random Number Generation**: Issues with RNG causing biased distribution
4. **Terrain Characteristics**: The terrain itself might have characteristics that prevent erosion in certain areas

## Root Cause Analysis

### 1. Droplet Initialization and Distribution

**Location**: `HydraulicErosion.cpp` lines 139-140

```cpp
float x = rng.nextFloat() * (float)(W - 1);
float y = rng.nextFloat() * (float)(H - 1);
```

**Analysis**:

- Droplets are initialized with random positions across the entire terrain
- The RNG should provide uniform distribution
- **Potential Issue**: If the RNG has poor quality or seeding issues, it might not provide uniform distribution

### 2. Random Number Generation Quality

**Location**: `util.cpp` lines 11-20

```cpp
int RNG::nextInt() {
    ll x = _state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    _state = x;
    return static_cast<int>((x * 2685821657736338717LL) >> 32);
}

float RNG::nextFloat() { return (nextInt() / 4294967296.0f); }
```

**Analysis**:

- Uses a simple XOR-based PRNG (similar to xorshift)
- The `nextFloat()` implementation has a **critical bug**: it divides by `4294967296.0f` instead of `4294967295.0f`
- This means the maximum value returned is `4294967295 / 4294967296 ≈ 0.9999999997671694`
- **This is NOT the root cause** of the top-left bias, but it's a bug

### 3. Droplet Movement and Termination

**Location**: `HydraulicErosion.cpp` lines 170-171

```cpp
// stop if out of bounds
if (x < 0.0f || x > (W - 1) || y < 0.0f || y > (H - 1)) break;
```

**Analysis**:

- Droplets terminate when they go out of bounds
- This could cause bias if droplets tend to move in certain directions
- **Potential Issue**: If terrain has a general slope toward the top-left, droplets might exit early

### 4. Terrain Characteristics

**Location**: Voronoi generation and FBM blending

```cpp
// In WorldType_Voronoi::generate()
float vor = voronoiHeightAt(x, y);
float fbm = fbmNoiseAt((float)x, (float)y);
float h = (1.0f - cfg_.fbmBlend) * vor + cfg_.fbmBlend * fbm;
```

**Analysis**:

- The terrain generation might create patterns that favor erosion in certain areas
- Voronoi plates might be positioned in a way that creates slopes toward the top-left
- FBM noise might have directional bias

## Most Likely Causes

### 1. **RNG Seeding Issue** (Most Likely)

**Location**: `HydraulicErosion.cpp` lines 133-136

```cpp
ll seedState = params.worldSeed;
ll localState = seedState ^ (ll)di * 2654435761LL;
ll seed = rng_util::splitmix(localState);
rng_util::RNG rng(seed);
```

**Problem**: The seeding strategy might not provide sufficient entropy or might have patterns that cause biased distribution.

**Evidence**:

- Each droplet gets a different seed based on its index `di`
- The multiplication by `2654435761LL` might not provide good distribution
- The XOR operation might create patterns

### 2. **Terrain Slope Direction**

**Analysis**: If the terrain has a general slope toward the top-left, droplets will:

- Start at random positions
- Follow the slope downhill (toward top-left)
- Exit the terrain early from the top-left edges
- Never reach the bottom-right area

### 3. **Droplet Lifetime Issues**

**Location**: `HydraulicErosion.cpp` lines 208-210

```cpp
water *= (1.0f - params.evaporateRate);
if (water < params.minWater) break;
if (speed < params.minSpeed) break;
```

**Problem**: Droplets might evaporate or slow down before reaching the bottom-right area.

## Debugging Steps

### 1. **Add Debug Output for Droplet Distribution**

```cpp
// Add this after line 140 in HydraulicErosion.cpp
if (di < 100) {  // Log first 100 droplets
    std::cerr << "[DEBUG] Droplet " << di << " start: (" << x << ", " << y << ")" << std::endl;
}
```

### 2. **Add Debug Output for Droplet Movement**

```cpp
// Add this after line 168 in HydraulicErosion.cpp
if (di < 10) {  // Log first 10 droplets
    std::cerr << "[DEBUG] Droplet " << di << " step " << step << ": (" << x << ", " << y << ")" << std::endl;
}
```

### 3. **Check Terrain Slope Direction**

Add a function to analyze the general slope direction of the terrain:

```cpp
void analyzeTerrainSlope(const GridFloat& height) {
    int W = height.width();
    int H = height.height();

    float totalGradX = 0.0f, totalGradY = 0.0f;
    int count = 0;

    for (int y = 1; y < H - 1; ++y) {
        for (int x = 1; x < W - 1; ++x) {
            float gx = (height(x + 1, y) - height(x - 1, y)) * 0.5f;
            float gy = (height(x, y + 1) - height(x, y - 1)) * 0.5f;
            totalGradX += gx;
            totalGradY += gy;
            count++;
        }
    }

    float avgGradX = totalGradX / count;
    float avgGradY = totalGradY / count;

    std::cerr << "[DEBUG] Average terrain gradient: (" << avgGradX << ", " << avgGradY << ")" << std::endl;
    std::cerr << "[DEBUG] General slope direction: " << atan2(avgGradY, avgGradX) * 180.0 / M_PI << " degrees" << std::endl;
}
```

## Recommended Fixes

### 1. **Fix RNG Float Generation** (Critical Bug)

```cpp
float RNG::nextFloat() {
    return (nextInt() / 4294967295.0f);  // Fix: use 4294967295 instead of 4294967296
}
```

### 2. **Improve Droplet Seeding**

```cpp
// Replace lines 133-136 with:
ll seedState = params.worldSeed;
ll localState = seedState ^ (ll)di * 0x9e3779b97f4a7c15ULL;  // Better constant
localState = rng_util::splitmix(localState);  // Additional mixing
ll seed = rng_util::splitmix(localState);
rng_util::RNG rng(seed);
```

### 3. **Add Droplet Position Validation**

```cpp
// After line 140, add validation:
if (x < 0.0f || x >= W || y < 0.0f || y >= H) {
    std::cerr << "[WARNING] Droplet " << di << " started out of bounds: (" << x << ", " << y << ")" << std::endl;
    continue;  // Skip this droplet
}
```

### 4. **Implement Grid-Based Droplet Distribution**

Instead of purely random distribution, use a more systematic approach:

```cpp
// Replace random initialization with grid-based + jitter
int gridX = di % (int)sqrt(N);
int gridY = di / (int)sqrt(N);
float baseX = (float)gridX * (float)(W - 1) / sqrt(N);
float baseY = (float)gridY * (float)(H - 1) / sqrt(N);
float jitterX = (rng.nextFloat() - 0.5f) * 2.0f;  // ±1 pixel jitter
float jitterY = (rng.nextFloat() - 0.5f) * 2.0f;
float x = std::clamp(baseX + jitterX, 0.0f, (float)(W - 1));
float y = std::clamp(baseY + jitterY, 0.0f, (float)(H - 1));
```

## Testing Strategy

1. **Run with debug output** to see droplet distribution and movement
2. **Analyze terrain slope** to check for directional bias
3. **Test with different seeds** to see if the pattern is consistent
4. **Implement fixes incrementally** and test each one
5. **Compare erosion maps** before and after fixes

## Expected Results After Fixes

- **Uniform droplet distribution** across the entire terrain
- **Erosion patterns** that follow terrain characteristics rather than artificial boundaries
- **Consistent results** across different random seeds
- **Better erosion coverage** in all areas of the terrain

The most likely cause is the RNG seeding issue combined with potential terrain slope direction. The fixes above should resolve the top-left bias and provide more realistic erosion patterns.
