# Terrain Generation Benchmark Master Report

**Generated:** 2025-10-25 19:43:40

## Overview

This comprehensive benchmark analysis covers terrain generation performance across multiple resolutions and thread counts.

## Test Configurations

- **Resolutions:** 256x256, 512x512, 1024x1024, 2048x2048
- **Thread Counts:** 1, 2, 4, 8, 14, MAX
- **Runs per Configuration:** 3
- **Total Test Cases:** 72

## Resolution Reports

- [1024x1024 Report](1024x1024/BENCHMARK_REPORT.md)

## Directory Structure

```
benchmark_output/
├── MASTER_BENCHMARK_REPORT.md
├── 1024x1024/
│   ├── BENCHMARK_REPORT.md
│   ├── 1_threads_result.json
│   ├── 2_threads_result.json
│   ├── ...
│   └── MAX_threads/
│       └── run_1/
│           ├── 01_height_before_erosion.ppm
│           ├── 02_height_after_erosion.ppm
│           ├── ...
│           └── 11_biome_final.ppm
```

