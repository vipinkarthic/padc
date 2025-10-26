# Terrain Generation Benchmark Report - 256x256

**Generated:** 2025-10-25 16:50:47

## Executive Summary

- **Baseline Performance (1 thread):** 0.679s
- **Peak Memory Usage:** 4048 KB
- **Most Time-Consuming Stage:** Hydraulic Erosion

## Performance Results

| Threads | Total Time (s) | Memory (KB) | Speedup | Efficiency |
|---------|----------------|-------------|---------|------------|
| 1 | 0.679 | 4048 | 1.00x | 1.00 |
| 2 | 0.693 | 4364 | 0.98x | 0.49 |
| 4 | 0.771 | 4478.666666666667 | 0.88x | 0.22 |
| 8 | 0.979 | 4785.333333333333 | 0.69x | 0.09 |
| 14 | 1.268 | 4952 | 0.54x | 0.04 |
| 20 | 1.599 | 5218.666666666667 | 0.42x | 0.02 |

## Stage Performance Analysis

| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) | 8 Threads (s) |
|-------|--------------|---------------|---------------|---------------|
| Heightmap And Voronoi | 0.012 | 0.007 | 0.005 | 0.005 |
| Hydraulic Erosion | 0.631 | 0.655 | 0.735 | 0.940 |
| River Generation | 0.006 | 0.006 | 0.006 | 0.007 |
| Biome Classification | 0.026 | 0.021 | 0.021 | 0.023 |
| Object Placement | 0.003 | 0.003 | 0.004 | 0.004 |

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
256x256/
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

