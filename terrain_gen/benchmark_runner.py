#!/usr/bin/env python3
"""
Terrain Generation Benchmark Runner

This script runs comprehensive benchmarks for the terrain generation system
across different resolutions and thread counts, measuring performance and memory usage.
"""

import os
import sys
import json
import subprocess
import time
import argparse
from pathlib import Path
from typing import Dict, List, Tuple
import statistics

class BenchmarkRunner:
    def __init__(self, executable_path: str, output_dir: str = "benchmark_results"):
        self.executable_path = Path(executable_path)
        self.output_dir = Path(output_dir)
        self.resolutions = [256, 512, 1024, 2048]
        self.thread_counts = [1, 2, 4, 8, 14, "MAX"]
        self.runs_per_config = 3  # Number of runs for averaging
        
        # Create output directory structure
        self.output_dir.mkdir(exist_ok=True)
        
    def get_max_threads(self) -> int:
        """Get the maximum number of threads available on the system."""
        try:
            return os.cpu_count()
        except:
            return 14  # Fallback
    
    def run_single_benchmark(self, width: int, height: int, threads: int, run_id: int, seed: int = 424242) -> Dict:
        """Run a single benchmark and return the results."""
        cmd = [
            str(self.executable_path),
            str(width),
            str(height),
            str(threads),
            str(run_id),
            str(seed)
        ]
        
        print(f"Running: {' '.join(cmd)}")
        
        try:
            start_time = time.time()
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)  # 1 hour timeout
            end_time = time.time()
            
            if result.returncode != 0:
                print(f"Error running benchmark: {result.stderr}")
                return None
                
            # Parse JSON output
            try:
                benchmark_data = json.loads(result.stdout)
                benchmark_data["wall_clock_time"] = end_time - start_time
                return benchmark_data
            except json.JSONDecodeError as e:
                print(f"Failed to parse JSON output: {e}")
                print(f"Output: {result.stdout}")
                return None
                
        except subprocess.TimeoutExpired:
            print(f"Benchmark timed out after 1 hour")
            return None
        except Exception as e:
            print(f"Error running benchmark: {e}")
            return None
    
    def calculate_speedup(self, baseline_times: Dict[str, float], current_times: Dict[str, float]) -> Dict[str, float]:
        """Calculate speedup for each stage and total."""
        speedup = {}
        for stage, baseline_time in baseline_times.items():
            if stage in current_times and baseline_time > 0:
                speedup[stage] = baseline_time / current_times[stage]
            else:
                speedup[stage] = 1.0
        
        # Calculate total speedup
        baseline_total = sum(baseline_times.values())
        current_total = sum(current_times.values())
        if baseline_total > 0 and current_total > 0:
            speedup["total"] = baseline_total / current_total
        else:
            speedup["total"] = 1.0
            
        return speedup
    
    def run_benchmark_suite(self):
        """Run the complete benchmark suite."""
        print("Starting comprehensive terrain generation benchmark...")
        print(f"Resolutions: {self.resolutions}")
        print(f"Thread counts: {self.thread_counts}")
        print(f"Runs per configuration: {self.runs_per_config}")
        
        all_results = {}
        baseline_results = {}  # Store 1-thread results for speedup calculation
        
        for resolution in self.resolutions:
            print(f"\n{'='*60}")
            print(f"Testing resolution: {resolution}x{resolution}")
            print(f"{'='*60}")
            
            all_results[resolution] = {}
            
            for thread_count in self.thread_counts:
                actual_threads = self.get_max_threads() if thread_count == "MAX" else thread_count
                thread_key = f"{actual_threads}_threads"
                
                print(f"\nTesting with {actual_threads} threads...")
                
                # Run multiple times for averaging
                run_results = []
                for run_id in range(1, self.runs_per_config + 1):
                    print(f"  Run {run_id}/{self.runs_per_config}...")
                    
                    result = self.run_single_benchmark(
                        resolution, resolution, actual_threads, run_id
                    )
                    
                    if result:
                        run_results.append(result)
                        time.sleep(1)  # Brief pause between runs
                    else:
                        print(f"    Failed run {run_id}")
                
                if run_results:
                    # Calculate averages
                    avg_result = self.calculate_averages(run_results)
                    
                    # Calculate speedup if this is not the baseline (1 thread)
                    if actual_threads != 1 and resolution in baseline_results:
                        avg_result["speedup_per_stage"] = self.calculate_speedup(
                            baseline_results[resolution]["stage_times"],
                            avg_result["stage_times"]
                        )
                        avg_result["total_speedup"] = baseline_results[resolution]["total_time"] / avg_result["total_time"]
                    else:
                        avg_result["speedup_per_stage"] = {stage: 1.0 for stage in avg_result["stage_times"].keys()}
                        avg_result["total_speedup"] = 1.0
                    
                    all_results[resolution][thread_key] = avg_result
                    
                    # Store baseline results (1 thread)
                    if actual_threads == 1:
                        baseline_results[resolution] = avg_result
                    
                    # Save individual result
                    self.save_result(resolution, thread_key, avg_result)
                    
                    print(f"    Average total time: {avg_result['total_time']:.3f}s")
                    print(f"    Peak memory: {avg_result['peak_memory_kb']} KB")
                    if actual_threads != 1:
                        print(f"    Total speedup: {avg_result['total_speedup']:.2f}x")
                else:
                    print(f"    All runs failed for {actual_threads} threads")
        
        # Save summary results
        self.save_summary(all_results)
        print(f"\nBenchmark complete! Results saved to {self.output_dir}")
    
    def calculate_averages(self, results: List[Dict]) -> Dict:
        """Calculate average values from multiple benchmark runs."""
        if not results:
            return {}
        
        # Initialize with first result
        avg_result = results[0].copy()
        
        # Calculate averages for numeric values
        numeric_fields = ["total_time", "peak_memory_kb", "wall_clock_time"]
        for field in numeric_fields:
            if field in avg_result:
                values = [r.get(field, 0) for r in results]
                avg_result[field] = statistics.mean(values)
        
        # Calculate averages for stage times
        if "stage_times" in avg_result:
            stage_times = {}
            for stage in avg_result["stage_times"].keys():
                values = [r.get("stage_times", {}).get(stage, 0) for r in results]
                stage_times[stage] = statistics.mean(values)
            avg_result["stage_times"] = stage_times
        
        # Add metadata
        avg_result["num_runs"] = len(results)
        avg_result["std_dev_total_time"] = statistics.stdev([r.get("total_time", 0) for r in results]) if len(results) > 1 else 0
        
        return avg_result
    
    def save_result(self, resolution: int, thread_config: str, result: Dict):
        """Save individual benchmark result."""
        resolution_dir = self.output_dir / f"{resolution}x{resolution}"
        resolution_dir.mkdir(exist_ok=True)
        
        filename = f"{thread_config}_result.json"
        filepath = resolution_dir / filename
        
        with open(filepath, 'w') as f:
            json.dump(result, f, indent=2)
    
    def save_summary(self, all_results: Dict):
        """Save summary of all benchmark results."""
        summary = {
            "benchmark_info": {
                "resolutions": self.resolutions,
                "thread_counts": self.thread_counts,
                "runs_per_config": self.runs_per_config,
                "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
            },
            "results": all_results
        }
        
        summary_file = self.output_dir / "benchmark_summary.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        # Create a CSV summary for easy analysis
        self.create_csv_summary(all_results)
    
    def create_csv_summary(self, all_results: Dict):
        """Create a CSV summary of benchmark results."""
        csv_file = self.output_dir / "benchmark_summary.csv"
        
        with open(csv_file, 'w') as f:
            f.write("resolution,threads,total_time,peak_memory_kb,heightmap_and_voronoi,hydraulic_erosion,river_generation,biome_classification,object_placement,total_speedup\n")
            
            for resolution, thread_results in all_results.items():
                for thread_config, result in thread_results.items():
                    threads = thread_config.replace("_threads", "")
                    stage_times = result.get("stage_times", {})
                    
                    f.write(f"{resolution}x{resolution},{threads},{result.get('total_time', 0):.3f},")
                    f.write(f"{result.get('peak_memory_kb', 0)},{stage_times.get('heightmap_and_voronoi', 0):.3f},")
                    f.write(f"{stage_times.get('hydraulic_erosion', 0):.3f},{stage_times.get('river_generation', 0):.3f},")
                    f.write(f"{stage_times.get('biome_classification', 0):.3f},{stage_times.get('object_placement', 0):.3f},")
                    f.write(f"{result.get('total_speedup', 1.0):.2f}\n")

def main():
    parser = argparse.ArgumentParser(description="Run terrain generation benchmarks")
    parser.add_argument("--executable", "-e", default="build/bin/terrain-gen-benchmark.exe", 
                       help="Path to the benchmark executable")
    parser.add_argument("--output", "-o", default="benchmark_results", 
                       help="Output directory for results")
    parser.add_argument("--resolutions", nargs="+", type=int, default=[256, 512, 1024, 2048],
                       help="Resolutions to test")
    parser.add_argument("--threads", nargs="+", default=[1, 2, 4, 8, 14, "MAX"],
                       help="Thread counts to test")
    parser.add_argument("--runs", type=int, default=3,
                       help="Number of runs per configuration")
    
    args = parser.parse_args()
    
    # Check if executable exists
    if not os.path.exists(args.executable):
        print(f"Error: Executable not found at {args.executable}")
        print("Please build the benchmark executable first.")
        sys.exit(1)
    
    # Create and run benchmark
    runner = BenchmarkRunner(args.executable, args.output)
    runner.resolutions = args.resolutions
    runner.thread_counts = args.threads
    runner.runs_per_config = args.runs
    
    try:
        runner.run_benchmark_suite()
    except KeyboardInterrupt:
        print("\nBenchmark interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Error running benchmark: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
