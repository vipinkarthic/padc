# Terrain Generation Benchmark Report - 1024x1024

**Generated:** 2025-10-25 19:43:40

## Executive Summary

- **Baseline Performance (1 thread):** 19.568s
- **Peak Memory Usage:** 81860 KB
- **Most Time-Consuming Stage:** Hydraulic Erosion

## Performance Results

| Threads | Total Time (s) | Memory (KB) | Speedup | Efficiency |
|---------|----------------|-------------|---------|------------|
| 1 | 19.568 | 81860 | 1.00x | 1.00 |
| 2 | 16.513 | 82565.33333333333 | 1.18x | 0.59 |
| 4 | 16.886 | 83032 | 1.16x | 0.29 |
| 8 | 22.638 | 84668 | 0.86x | 0.11 |
| 14 | 33.108 | 85166.66666666667 | 0.59x | 0.04 |
| 20 | 34.896 | 86329.33333333333 | 0.56x | 0.03 |

## Stage Performance Analysis

| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) | 8 Threads (s) |
|-------|--------------|---------------|---------------|---------------|
| Heightmap And Voronoi | 0.202 | 0.112 | 0.066 | 0.046 |
| Hydraulic Erosion | 13.120 | 13.227 | 15.039 | 21.210 |
| River Generation | 0.168 | 0.121 | 0.112 | 0.120 |
| Biome Classification | 5.995 | 2.991 | 1.611 | 1.191 |
| Object Placement | 0.079 | 0.059 | 0.054 | 0.068 |

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
1024x1024/
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

