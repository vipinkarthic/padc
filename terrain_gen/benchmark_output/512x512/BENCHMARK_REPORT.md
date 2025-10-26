# Terrain Generation Benchmark Report - 512x512

**Generated:** 2025-10-25 19:24:28

## Executive Summary

- **Baseline Performance (1 thread):** 4.201s
- **Peak Memory Usage:** 20960 KB
- **Most Time-Consuming Stage:** Hydraulic Erosion

## Performance Results

| Threads | Total Time (s) | Memory (KB) | Speedup | Efficiency |
|---------|----------------|-------------|---------|------------|
| 1 | 4.201 | 20960 | 1.00x | 1.00 |
| 2 | 3.675 | 21436 | 1.14x | 0.57 |
| 4 | 3.767 | 21948 | 1.12x | 0.28 |
| 8 | 5.206 | 22462.666666666668 | 0.81x | 0.10 |
| 14 | 7.949 | 22633.333333333332 | 0.53x | 0.04 |
| 20 | 8.819 | 22989.333333333332 | 0.48x | 0.02 |

## Stage Performance Analysis

| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) | 8 Threads (s) |
|-------|--------------|---------------|---------------|---------------|
| Heightmap And Voronoi | 0.052 | 0.031 | 0.016 | 0.014 |
| Hydraulic Erosion | 2.980 | 2.939 | 3.277 | 4.798 |
| River Generation | 0.029 | 0.025 | 0.026 | 0.029 |
| Biome Classification | 1.124 | 0.666 | 0.433 | 0.346 |
| Object Placement | 0.015 | 0.013 | 0.014 | 0.017 |

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
512x512/
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

