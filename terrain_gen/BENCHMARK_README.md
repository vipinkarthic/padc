# Terrain Generation Benchmark System

This system provides comprehensive performance analysis for the terrain generation pipeline across different resolutions and thread counts.

## Overview

The benchmark system measures performance for the following stages:
1. **Heightmap generation and Voronoi generation** - Base terrain creation
2. **Hydraulic erosion** - Water-based terrain erosion simulation
3. **River generation** - River carving and channel formation
4. **Biome classification** - Terrain type assignment
5. **Object placement** - Vegetation and structure placement

## Test Configurations

- **Resolutions**: 256x256, 512x512, 1024x1024, 2048x2048
- **Thread counts**: 1, 2, 4, 8, 14, MAX (system maximum)
- **Runs per configuration**: 3 (for statistical averaging)

## Quick Start

### Option 1: Automated Script (Recommended)
```bash
# Windows Batch
run_benchmarks.bat

# Windows PowerShell
.\run_benchmarks.ps1
```

### Option 2: Manual Execution
```bash
# 1. Build the benchmark executable
cd build
cmake --build . --config Release
cd ..

# 2. Run the benchmark suite
python benchmark_runner.py --executable build/bin/terrain-gen-benchmark.exe --output benchmark_results --runs 3
```

## Output Structure

The benchmark creates the following output structure:

```
benchmark_results/
├── benchmark_summary.json          # Complete results in JSON format
├── benchmark_summary.csv          # Results in CSV format for analysis
├── 256x256/                       # Results for 256x256 resolution
│   ├── 1_threads_result.json
│   ├── 2_threads_result.json
│   ├── 4_threads_result.json
│   ├── 8_threads_result.json
│   ├── 14_threads_result.json
│   └── MAX_threads_result.json
├── 512x512/                       # Results for 512x512 resolution
│   └── ...
├── 1024x1024/                     # Results for 1024x1024 resolution
│   └── ...
└── 2048x2048/                     # Results for 2048x2048 resolution
    └── ...
```

## Result Format

Each result file contains:

```json
{
  "resolution": "1024x1024",
  "threads": 8,
  "run_id": 1,
  "stage_times": {
    "heightmap_and_voronoi": 0.421,
    "hydraulic_erosion": 3.252,
    "river_generation": 0.317,
    "biome_classification": 0.144,
    "object_placement": 0.882
  },
  "total_time": 5.016,
  "peak_memory_kb": 123456,
  "speedup_per_stage": {
    "heightmap_and_voronoi": 2.1,
    "hydraulic_erosion": 1.8,
    "river_generation": 2.3,
    "biome_classification": 1.9,
    "object_placement": 2.0
  },
  "total_speedup": 1.95,
  "num_runs": 3,
  "std_dev_total_time": 0.023
}
```

## Performance Metrics

- **Stage Times**: Individual timing for each processing stage
- **Total Time**: Complete pipeline execution time
- **Peak Memory**: Maximum memory usage during execution
- **Speedup**: Performance improvement compared to single-threaded execution
- **Standard Deviation**: Statistical variance across multiple runs

## Customization

### Modify Test Parameters
Edit `benchmark_runner.py` or use command-line arguments:

```bash
python benchmark_runner.py \
  --executable build/bin/terrain-gen-benchmark.exe \
  --output custom_results \
  --resolutions 256 512 1024 \
  --threads 1 2 4 8 \
  --runs 5
```

### Add New Stages
Modify `src/benchmark_main.cpp` to add timing for additional processing stages.

## Analysis Tools

### CSV Analysis
The `benchmark_summary.csv` file can be imported into Excel, Python pandas, or other analysis tools:

```python
import pandas as pd
df = pd.read_csv('benchmark_results/benchmark_summary.csv')
print(df.groupby('resolution')['total_time'].mean())
```

### JSON Analysis
For programmatic analysis:

```python
import json
with open('benchmark_results/benchmark_summary.json') as f:
    results = json.load(f)
    
# Analyze results
for resolution, thread_results in results['results'].items():
    print(f"Resolution {resolution}:")
    for thread_config, result in thread_results.items():
        print(f"  {thread_config}: {result['total_time']:.3f}s")
```

## Requirements

- **C++ Compiler**: MSVC, GCC, or Clang with C++17 support
- **CMake**: Version 3.15 or higher
- **OpenMP**: For multi-threading support
- **Python**: Version 3.6 or higher (for benchmark runner)
- **Memory**: Sufficient RAM for largest test case (2048x2048)

## Troubleshooting

### Build Issues
- Ensure all dependencies are installed
- Check that OpenMP is available
- Verify CMake configuration

### Runtime Issues
- Check available system memory
- Ensure sufficient disk space for output files
- Monitor system temperature during long runs

### Performance Issues
- Close unnecessary applications
- Run on a dedicated system if possible
- Consider reducing test parameters for initial runs

## Expected Results

Typical performance characteristics:
- **Single-threaded**: Baseline performance
- **2-4 threads**: Moderate speedup (1.5-2.5x)
- **8+ threads**: Diminishing returns due to memory bandwidth
- **Memory usage**: Scales with resolution squared
- **Erosion stage**: Typically the most time-consuming

## Notes

- Benchmark runs can take several hours for complete suite
- Large resolutions (2048x2048) require significant memory
- Results may vary based on system configuration
- Consider running during off-peak hours for consistent results
