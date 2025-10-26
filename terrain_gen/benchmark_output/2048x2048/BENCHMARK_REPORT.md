# Terrain Generation Benchmark Report - 2048x2048

**Generated:** 2025-10-25 17:26:42

## Executive Summary

- **Baseline Performance (1 thread):** 56.415s
- **Peak Memory Usage:** 219817.33333333334 KB
- **Most Time-Consuming Stage:** Hydraulic Erosion

## Performance Results

| Threads | Total Time (s) | Memory (KB) | Speedup | Efficiency |
|---------|----------------|-------------|---------|------------|
| 1 | 56.415 | 219817.33333333334 | 1.00x | 1.00 |
| 2 | 57.823 | 226966.66666666666 | 0.98x | 0.49 |
| 4 | 64.119 | 228008 | 0.88x | 0.22 |
| 8 | 85.897 | 230201.33333333334 | 0.66x | 0.08 |
| 14 | 119.747 | 231529.33333333334 | 0.47x | 0.03 |
| 20 | 140.308 | 233774.66666666666 | 0.40x | 0.02 |

## Stage Performance Analysis

| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) | 8 Threads (s) |
|-------|--------------|---------------|---------------|---------------|
| Heightmap And Voronoi | 0.768 | 0.419 | 0.254 | 0.164 |
| Hydraulic Erosion | 53.234 | 55.684 | 62.464 | 84.465 |
| River Generation | 0.581 | 0.508 | 0.494 | 0.453 |
| Biome Classification | 1.620 | 0.987 | 0.687 | 0.564 |
| Object Placement | 0.197 | 0.210 | 0.207 | 0.236 |

## Generated Images

For each test case, the following images are generated in order:

1. **Height Before Erosion** - Initial terrain heightmap
2. **Height After Erosion** - Terrain after hydraulic erosion
3. **Height After Rivers** - Final terrain with river carving
4. **Biome Before Erosion** - Initial biome classification
5. **Biome After Erosion** - Biome classification after erosion
6. **Biome After Rivers** - Final biome classification
7. **Erosion Eroded** - Areas where material was removed
8. **Erosion Deposited** - Areas where material was deposited
9. **River Map** - River network visualization
10. **Height Final** - Final heightmap
11. **Biome Final** - Final biome map

## Directory Structure

```
2048x2048/
├── 1_threads/
│   ├── 1_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
├── 2_threads/
│   ├── 2_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
├── 4_threads/
│   ├── 4_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
├── 8_threads/
│   ├── 8_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
├── 14_threads/
│   ├── 14_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
├── 20_threads/
│   ├── 20_threads_result.json
│   └── run_1/
│       ├── 01_height_before_erosion.ppm
│       ├── 02_height_after_erosion.ppm
│       ├── ...
│       └── 11_biome_final.ppm
```

