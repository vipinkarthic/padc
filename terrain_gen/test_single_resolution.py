#!/usr/bin/env python3
"""
Test single resolution documentation generation
"""

import subprocess
import json
import time
from pathlib import Path
import statistics

def run_single_resolution_test(resolution=256):
    """Run documentation test for a single resolution."""
    print(f"Testing documentation generation for {resolution}x{resolution}...")
    
    output_dir = Path(f"benchmark_output/{resolution}x{resolution}")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Test with different thread counts
    thread_counts = [1, 2, 4]
    all_results = {}
    
    for threads in thread_counts:
        print(f"\nTesting with {threads} threads...")
        
        # Run 2 times for averaging
        run_results = []
        for run_id in range(1, 3):  # Reduced to 2 runs for testing
            print(f"  Run {run_id}/2...")
            
            cmd = [
                "build/bin/terrain-gen-benchmark.exe",
                str(resolution), str(resolution), str(threads), str(run_id), "424242"
            ]
            
            try:
                start_time = time.time()
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)  # 10 minute timeout
                end_time = time.time()
                
                if result.returncode != 0:
                    print(f"    Error: {result.stderr}")
                    continue
                    
                # Parse JSON output
                try:
                    data = json.loads(result.stdout)
                    data["wall_clock_time"] = end_time - start_time
                    run_results.append(data)
                    print(f"    Time: {data['total_time']:.3f}s, Memory: {data['peak_memory_kb']} KB")
                except json.JSONDecodeError as e:
                    print(f"    Failed to parse JSON: {e}")
                    continue
                    
            except subprocess.TimeoutExpired:
                print(f"    Timed out")
                continue
            except Exception as e:
                print(f"    Error: {e}")
                continue
        
        if run_results:
            # Calculate averages
            avg_result = run_results[0].copy()
            avg_result["total_time"] = statistics.mean([r["total_time"] for r in run_results])
            avg_result["peak_memory_kb"] = statistics.mean([r["peak_memory_kb"] for r in run_results])
            avg_result["wall_clock_time"] = statistics.mean([r["wall_clock_time"] for r in run_results])
            
            # Average stage times
            stage_times = {}
            for stage in avg_result["stage_times"].keys():
                values = [r["stage_times"][stage] for r in run_results]
                stage_times[stage] = statistics.mean(values)
            avg_result["stage_times"] = stage_times
            
            avg_result["num_runs"] = len(run_results)
            all_results[f"{threads}_threads"] = avg_result
            
            # Save result
            result_file = output_dir / f"{threads}_threads_result.json"
            with open(result_file, 'w') as f:
                json.dump(avg_result, f, indent=2)
            
            print(f"    Average: {avg_result['total_time']:.3f}s")
    
    # Generate report
    generate_report(resolution, all_results, output_dir)
    
    return all_results

def generate_report(resolution, results, output_dir):
    """Generate a report for the resolution."""
    report_file = output_dir / "BENCHMARK_REPORT.md"
    
    with open(report_file, 'w') as f:
        f.write(f"# Terrain Generation Benchmark Report - {resolution}x{resolution}\n\n")
        f.write(f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        # Performance Table
        f.write("## Performance Results\n\n")
        f.write("| Threads | Total Time (s) | Memory (KB) | Speedup |\n")
        f.write("|---------|----------------|-------------|----------|\n")
        
        baseline_time = results.get("1_threads", {}).get("total_time", 1.0)
        for thread_config, result in results.items():
            threads = thread_config.replace("_threads", "")
            total_time = result.get("total_time", 0)
            memory = result.get("peak_memory_kb", 0)
            speedup = baseline_time / total_time if total_time > 0 else 1.0
            
            f.write(f"| {threads} | {total_time:.3f} | {memory} | {speedup:.2f}x |\n")
        
        f.write("\n")
        
        # Stage Analysis
        f.write("## Stage Performance Analysis\n\n")
        if results:
            f.write("| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) |\n")
            f.write("|-------|--------------|---------------|---------------|\n")
            
            stages = ["heightmap_and_voronoi", "hydraulic_erosion", "river_generation", 
                     "biome_classification", "object_placement"]
            
            for stage in stages:
                row = f"| {stage.replace('_', ' ').title()} |"
                for thread_config in ["1_threads", "2_threads", "4_threads"]:
                    if thread_config in results:
                        time_val = results[thread_config].get("stage_times", {}).get(stage, 0)
                        row += f" {time_val:.3f} |"
                    else:
                        row += " N/A |"
                f.write(row + "\n")
        
        f.write("\n")
        
        # Image Gallery
        f.write("## Generated Images\n\n")
        f.write("For each test case, the following images are generated in order:\n\n")
        f.write("1. **Height Before Erosion** - Initial terrain heightmap\n")
        f.write("2. **Height After Erosion** - Terrain after hydraulic erosion\n")
        f.write("3. **Height After Rivers** - Final terrain with river carving\n")
        f.write("4. **Biome Before Erosion** - Initial biome classification\n")
        f.write("5. **Biome After Erosion** - Biome classification after erosion\n")
        f.write("6. **Biome After Rivers** - Final biome classification\n")
        f.write("7. **Erosion Eroded** - Areas where material was removed\n")
        f.write("8. **Erosion Deposited** - Areas where material was deposited\n")
        f.write("9. **River Map** - River network visualization\n")
        f.write("10. **Height Final** - Final heightmap\n")
        f.write("11. **Biome Final** - Final biome map\n\n")
        
        # Directory Structure
        f.write("## Directory Structure\n\n")
        f.write("```\n")
        f.write(f"{resolution}x{resolution}/\n")
        for thread_config in results.keys():
            threads = thread_config.replace("_threads", "")
            f.write(f"├── {threads}_threads/\n")
            f.write(f"│   ├── {thread_config}_result.json\n")
            f.write(f"│   └── run_1/\n")
            f.write(f"│       ├── 01_height_before_erosion.ppm\n")
            f.write(f"│       ├── 02_height_after_erosion.ppm\n")
            f.write(f"│       ├── ...\n")
            f.write(f"│       └── 11_biome_final.ppm\n")
        f.write("```\n\n")

def main():
    import sys
    
    resolution = 256
    if len(sys.argv) > 1:
        try:
            resolution = int(sys.argv[1])
        except ValueError:
            print("Invalid resolution. Using 256x256.")
    
    print(f"Testing documentation generation for {resolution}x{resolution}...")
    
    # Check if executable exists
    if not Path("build/bin/terrain-gen-benchmark.exe").exists():
        print("Error: Benchmark executable not found!")
        print("Please build the benchmark executable first.")
        return 1
    
    try:
        results = run_single_resolution_test(resolution)
        
        if results:
            print(f"\n[SUCCESS] Documentation generated for {resolution}x{resolution}!")
            print(f"Results saved to: benchmark_output/{resolution}x{resolution}/")
            print(f"Report: benchmark_output/{resolution}x{resolution}/BENCHMARK_REPORT.md")
            
            # Show summary
            print("\nPerformance Summary:")
            for thread_config, result in results.items():
                threads = thread_config.replace("_threads", "")
                print(f"  {threads} threads: {result['total_time']:.3f}s, {result['peak_memory_kb']} KB")
        else:
            print(f"\n[FAILED] No results generated for {resolution}x{resolution}")
            return 1
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
