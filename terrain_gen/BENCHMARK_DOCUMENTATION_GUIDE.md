# Terrain Generation Benchmark Documentation System

## Overview

This comprehensive benchmarking system provides detailed performance analysis for terrain generation across multiple resolutions and thread counts, with complete documentation including JSON results and PPM image galleries.

## System Components

### 1. Benchmark Executable (`src/benchmark_main.cpp`)
- **Purpose**: Modified version of the main terrain generation program
- **Features**:
  - Command-line arguments for width, height, threads, run_id, and seed
  - Precise timing measurements for each processing stage
  - Memory usage tracking
  - Automatic PPM image generation for documentation
  - JSON output format

### 2. Documentation Generators
- **`test_documentation.py`**: Quick test for single benchmark with images
- **`test_single_resolution.py`**: Test documentation for one resolution
- **`run_complete_benchmark.py`**: Full benchmark suite for all configurations

### 3. Output Structure

```
benchmark_output/
├── MASTER_BENCHMARK_REPORT.md          # Master overview
├── 256x256/                            # Per-resolution folders
│   ├── BENCHMARK_REPORT.md             # Detailed report
│   ├── 1_threads_result.json           # Performance data
│   ├── 2_threads_result.json
│   ├── 4_threads_result.json
│   ├── 8_threads_result.json
│   ├── 14_threads_result.json
│   ├── MAX_threads_result.json
│   ├── 1_threads/                       # Per-thread folders
│   │   ├── run_1/                       # Per-run folders
│   │   │   ├── 01_height_before_erosion.ppm
│   │   │   ├── 02_height_after_erosion.ppm
│   │   │   ├── 03_height_after_rivers.ppm
│   │   │   ├── 04_biome_before_erosion.ppm
│   │   │   ├── 05_biome_after_erosion.ppm
│   │   │   ├── 06_biome_after_rivers.ppm
│   │   │   ├── 07_erosion_eroded.ppm
│   │   │   ├── 08_erosion_deposited.ppm
│   │   │   ├── 09_river_map.ppm
│   │   │   ├── 10_height_final.ppm
│   │   │   └── 11_biome_final.ppm
│   │   └── run_2/                       # Additional runs
│   │       └── ... (same image set)
│   └── 2_threads/                       # Other thread counts
│       └── ...
├── 512x512/                             # Other resolutions
│   └── ...
├── 1024x1024/
│   └── ...
└── 2048x2048/
    └── ...
```

## Test Configurations

### Resolutions
- **256x256**: Quick testing and development
- **512x512**: Standard testing
- **1024x1024**: High-resolution testing
- **2048x2048**: Maximum resolution testing

### Thread Counts
- **1**: Single-threaded baseline
- **2**: Dual-core optimization
- **4**: Quad-core optimization
- **8**: Octa-core optimization
- **14**: High-core count testing
- **MAX**: System maximum threads

### Statistical Analysis
- **Runs per Configuration**: 3 (for statistical averaging)
- **Total Test Cases**: 72 (4 resolutions × 6 thread counts × 3 runs)
- **Metrics**: Mean, standard deviation, speedup calculations

## Generated Images (Per Test Case)

Each benchmark run generates 11 PPM images in sequence:

1. **01_height_before_erosion.ppm** - Initial terrain heightmap
2. **02_height_after_erosion.ppm** - Terrain after hydraulic erosion
3. **03_height_after_rivers.ppm** - Final terrain with river carving
4. **04_biome_before_erosion.ppm** - Initial biome classification
5. **05_biome_after_erosion.ppm** - Biome classification after erosion
6. **06_biome_after_rivers.ppm** - Final biome classification
7. **07_erosion_eroded.ppm** - Areas where material was removed
8. **08_erosion_deposited.ppm** - Areas where material was deposited
9. **09_river_map.ppm** - River network visualization
10. **10_height_final.ppm** - Final heightmap
11. **11_biome_final.ppm** - Final biome map

## Performance Metrics

### JSON Output Format
```json
{
  "resolution": "256x256",
  "threads": 1,
  "run_id": 1,
  "stage_times": {
    "heightmap_and_voronoi": 0.012,
    "hydraulic_erosion": 0.652,
    "river_generation": 0.007,
    "biome_classification": 0.027,
    "object_placement": 0.003
  },
  "total_time": 0.703,
  "peak_memory_kb": 4040,
  "wall_clock_time": 0.804,
  "num_runs": 3,
  "std_dev_total_time": 0.023
}
```

### Performance Analysis
- **Stage Times**: Individual timing for each processing stage
- **Total Time**: Complete pipeline execution time
- **Peak Memory**: Maximum memory usage during execution
- **Speedup**: Performance improvement vs single-threaded baseline
- **Efficiency**: Speedup per thread (parallel efficiency)
- **Statistical Data**: Standard deviation across multiple runs

## Usage Instructions

### Quick Test
```bash
# Test single benchmark with images
python test_documentation.py
```

### Single Resolution
```bash
# Test one resolution (256x256)
python test_single_resolution.py 256

# Test different resolution
python test_single_resolution.py 512
```

### Complete Benchmark Suite
```bash
# Run full benchmark suite (all resolutions and thread counts)
python run_complete_benchmark.py
```

### Manual Execution
```bash
# Run individual benchmark
build/bin/terrain-gen-benchmark.exe 256 256 1 1 424242
```

## Documentation Reports

### Master Report (`MASTER_BENCHMARK_REPORT.md`)
- Overview of all test configurations
- Links to individual resolution reports
- Directory structure guide
- Usage instructions

### Resolution Reports (`{resolution}x{resolution}/BENCHMARK_REPORT.md`)
- Executive summary with key metrics
- Performance tables with timing and memory data
- Stage-by-stage performance analysis
- Speedup and efficiency calculations
- Image gallery descriptions
- Directory structure for that resolution

## Technical Details

### Image Format
- **PPM (Portable Pixmap)**: Maximum compatibility across platforms
- **RGB Color Space**: Standard 24-bit color representation
- **Sequential Numbering**: Easy identification of generation order

### Timing Precision
- **High-Resolution Clock**: Microsecond accuracy
- **Stage-Level Timing**: Individual measurement for each processing stage
- **Wall-Clock Time**: Total execution time including overhead

### Memory Tracking
- **Peak Working Set**: Maximum memory usage during execution
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Real-Time Monitoring**: Continuous memory usage tracking

### Statistical Analysis
- **Multiple Runs**: 3 runs per configuration for statistical reliability
- **Mean Calculation**: Average performance across runs
- **Standard Deviation**: Measure of performance variance
- **Speedup Analysis**: Performance improvement relative to baseline

## Expected Results

### Performance Characteristics
- **Single-threaded**: Baseline performance for comparison
- **2-4 threads**: Moderate speedup (1.5-2.5x typical)
- **8+ threads**: Diminishing returns due to memory bandwidth
- **Memory usage**: Scales with resolution squared
- **Erosion stage**: Typically the most time-consuming

### Typical Performance (256x256)
- **Total time**: 0.7-1.0 seconds
- **Peak memory**: 4-5 MB
- **Hydraulic erosion**: 60-80% of total time
- **Speedup**: 1.5-2.0x with 4-8 threads

## Troubleshooting

### Common Issues
1. **Build Errors**: Ensure CMake and compiler are properly configured
2. **Memory Issues**: Large resolutions (2048x2048) require significant RAM
3. **Timeout Issues**: Very large resolutions may exceed 1-hour timeout
4. **Image Generation**: Ensure sufficient disk space for PPM files

### Performance Tips
1. **Close Applications**: Run benchmarks on dedicated system
2. **Monitor Temperature**: Long runs may cause thermal throttling
3. **Disk Space**: Each test case generates ~11 MB of images
4. **System Resources**: Full suite requires ~1 GB disk space

## File Sizes

### Per Test Case
- **JSON file**: ~1 KB
- **PPM images**: ~11 MB (11 images × ~1 MB each)
- **Total per case**: ~11 MB

### Complete Suite
- **Total test cases**: 72
- **Total disk space**: ~800 MB
- **Compression**: PPM files compress well with standard tools

## Integration

### With Analysis Tools
- **Excel/CSV**: Import JSON data for spreadsheet analysis
- **Python**: Use pandas for statistical analysis
- **Image Viewers**: Standard PPM viewers for PPM files
- **Version Control**: JSON files are text-based and version-controllable

This comprehensive system provides everything needed for detailed terrain generation performance analysis, from individual test cases to complete benchmark suites with full documentation and visualization.
