#!/usr/bin/env python3
"""
Terrain Generation Benchmark Documentation Generator

This script generates comprehensive documentation for benchmark results,
including JSON analysis and PPM image galleries for each test case.
"""

import os
import json
import subprocess
import time
from pathlib import Path
from typing import Dict, List, Tuple
import statistics

class DocumentationGenerator:
    def __init__(self, benchmark_results_dir: str = "benchmark_results", 
                 output_dir: str = "benchmark_output"):
        self.benchmark_results_dir = Path(benchmark_results_dir)
        self.output_dir = Path(output_dir)
        self.resolutions = [256, 512, 1024, 2048]
        self.thread_counts = [1, 2, 4, 8, 14, "MAX"]
        self.runs_per_config = 3
        
    def get_max_threads(self) -> int:
        """Get the maximum number of threads available on the system."""
        try:
            return os.cpu_count()
        except:
            return 14  # Fallback
    
    def run_benchmark_with_images(self, width: int, height: int, threads: int, run_id: int, seed: int = 424242) -> Dict:
        """Run a benchmark and generate images for documentation."""
        cmd = [
            "build/bin/terrain-gen-benchmark.exe",
            str(width),
            str(height),
            str(threads),
            str(run_id),
            str(seed)
        ]
        
        print(f"Running benchmark with images: {' '.join(cmd)}")
        
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
    
    def generate_resolution_documentation(self, resolution: int):
        """Generate comprehensive documentation for a specific resolution."""
        print(f"\n{'='*80}")
        print(f"Generating documentation for {resolution}x{resolution}")
        print(f"{'='*80}")
        
        resolution_dir = self.output_dir / f"{resolution}x{resolution}"
        resolution_dir.mkdir(parents=True, exist_ok=True)
        
        all_results = {}
        
        for thread_count in self.thread_counts:
            actual_threads = self.get_max_threads() if thread_count == "MAX" else thread_count
            thread_key = f"{actual_threads}_threads"
            
            print(f"\nTesting with {actual_threads} threads...")
            
            # Run multiple times for averaging
            run_results = []
            for run_id in range(1, self.runs_per_config + 1):
                print(f"  Run {run_id}/{self.runs_per_config}...")
                
                result = self.run_benchmark_with_images(
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
                all_results[thread_key] = avg_result
                
                # Save individual result
                result_file = resolution_dir / f"{thread_key}_result.json"
                with open(result_file, 'w') as f:
                    json.dump(avg_result, f, indent=2)
                
                print(f"    Average total time: {avg_result['total_time']:.3f}s")
                print(f"    Peak memory: {avg_result['peak_memory_kb']} KB")
        
        # Generate comprehensive documentation
        self.generate_resolution_report(resolution, all_results)
        
        return all_results
    
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
    
    def generate_resolution_report(self, resolution: int, results: Dict):
        """Generate a comprehensive report for a specific resolution."""
        report_file = self.output_dir / f"{resolution}x{resolution}" / "BENCHMARK_REPORT.md"
        
        with open(report_file, 'w') as f:
            f.write(f"# Terrain Generation Benchmark Report - {resolution}x{resolution}\n\n")
            f.write(f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            # Executive Summary
            f.write("## Executive Summary\n\n")
            if results:
                baseline = results.get("1_threads", {})
                if baseline:
                    f.write(f"- **Baseline Performance (1 thread):** {baseline.get('total_time', 0):.3f}s\n")
                    f.write(f"- **Peak Memory Usage:** {baseline.get('peak_memory_kb', 0)} KB\n")
                    f.write(f"- **Most Time-Consuming Stage:** {self.get_slowest_stage(baseline)}\n")
            f.write("\n")
            
            # Performance Table
            f.write("## Performance Results\n\n")
            f.write("| Threads | Total Time (s) | Memory (KB) | Speedup | Efficiency |\n")
            f.write("|---------|----------------|-------------|---------|------------|\n")
            
            baseline_time = results.get("1_threads", {}).get("total_time", 1.0)
            for thread_config, result in results.items():
                threads = thread_config.replace("_threads", "")
                total_time = result.get("total_time", 0)
                memory = result.get("peak_memory_kb", 0)
                speedup = baseline_time / total_time if total_time > 0 else 1.0
                efficiency = speedup / int(threads) if threads.isdigit() else speedup
                
                f.write(f"| {threads} | {total_time:.3f} | {memory} | {speedup:.2f}x | {efficiency:.2f} |\n")
            
            f.write("\n")
            
            # Stage Analysis
            f.write("## Stage Performance Analysis\n\n")
            if results:
                f.write("| Stage | 1 Thread (s) | 2 Threads (s) | 4 Threads (s) | 8 Threads (s) |\n")
                f.write("|-------|--------------|---------------|---------------|---------------|\n")
                
                stages = ["heightmap_and_voronoi", "hydraulic_erosion", "river_generation", 
                         "biome_classification", "object_placement"]
                
                for stage in stages:
                    row = f"| {stage.replace('_', ' ').title()} |"
                    for thread_config in ["1_threads", "2_threads", "4_threads", "8_threads"]:
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
            
            # Recommendations
            f.write("## Performance Recommendations\n\n")
            if results:
                best_config = self.get_best_configuration(results)
                f.write(f"- **Optimal Thread Count:** {best_config['threads']} threads\n")
                f.write(f"- **Best Performance:** {best_config['total_time']:.3f}s\n")
                f.write(f"- **Speedup vs Single Thread:** {best_config['speedup']:.2f}x\n")
                f.write(f"- **Memory Efficiency:** {best_config['memory']} KB\n")
            
            f.write("\n")
            f.write("## Technical Notes\n\n")
            f.write("- All timings are averaged over 3 runs\n")
            f.write("- Memory usage represents peak working set\n")
            f.write("- Speedup calculated relative to single-threaded baseline\n")
            f.write("- Images are generated in PPM format for maximum compatibility\n")
    
    def get_slowest_stage(self, result: Dict) -> str:
        """Get the name of the slowest processing stage."""
        stage_times = result.get("stage_times", {})
        if not stage_times:
            return "Unknown"
        
        slowest_stage = max(stage_times.items(), key=lambda x: x[1])
        return slowest_stage[0].replace('_', ' ').title()
    
    def get_best_configuration(self, results: Dict) -> Dict:
        """Determine the best performing configuration."""
        if not results:
            return {"threads": 1, "total_time": 0, "speedup": 1.0, "memory": 0}
        
        baseline_time = results.get("1_threads", {}).get("total_time", 1.0)
        best_config = None
        best_speedup = 0
        
        for thread_config, result in results.items():
            total_time = result.get("total_time", 0)
            if total_time > 0:
                speedup = baseline_time / total_time
                if speedup > best_speedup:
                    best_speedup = speedup
                    best_config = {
                        "threads": thread_config.replace("_threads", ""),
                        "total_time": total_time,
                        "speedup": speedup,
                        "memory": result.get("peak_memory_kb", 0)
                    }
        
        return best_config or {"threads": 1, "total_time": 0, "speedup": 1.0, "memory": 0}
    
    def generate_master_documentation(self):
        """Generate master documentation covering all resolutions."""
        master_file = self.output_dir / "MASTER_BENCHMARK_REPORT.md"
        
        with open(master_file, 'w') as f:
            f.write("# Terrain Generation Benchmark Master Report\n\n")
            f.write(f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            f.write("## Overview\n\n")
            f.write("This comprehensive benchmark analysis covers terrain generation performance across multiple resolutions and thread counts.\n\n")
            
            f.write("## Test Configurations\n\n")
            f.write("- **Resolutions:** 256x256, 512x512, 1024x1024, 2048x2048\n")
            f.write("- **Thread Counts:** 1, 2, 4, 8, 14, MAX\n")
            f.write("- **Runs per Configuration:** 3\n")
            f.write("- **Total Test Cases:** 72\n\n")
            
            f.write("## Resolution Reports\n\n")
            for resolution in self.resolutions:
                f.write(f"- [{resolution}x{resolution} Report]({resolution}x{resolution}/BENCHMARK_REPORT.md)\n")
            
            f.write("\n## Directory Structure\n\n")
            f.write("```\n")
            f.write("benchmark_output/\n")
            f.write("├── MASTER_BENCHMARK_REPORT.md\n")
            for resolution in self.resolutions:
                f.write(f"├── {resolution}x{resolution}/\n")
                f.write(f"│   ├── BENCHMARK_REPORT.md\n")
                f.write(f"│   ├── 1_threads_result.json\n")
                f.write(f"│   ├── 2_threads_result.json\n")
                f.write(f"│   ├── ...\n")
                f.write(f"│   └── MAX_threads/\n")
                f.write(f"│       └── run_1/\n")
                f.write(f"│           ├── 01_height_before_erosion.ppm\n")
                f.write(f"│           ├── 02_height_after_erosion.ppm\n")
                f.write(f"│           ├── ...\n")
                f.write(f"│           └── 11_biome_final.ppm\n")
            f.write("```\n\n")
            
            f.write("## Usage Instructions\n\n")
            f.write("1. **View Results:** Open the JSON files for detailed performance data\n")
            f.write("2. **Analyze Images:** Use the PPM images to visualize terrain generation stages\n")
            f.write("3. **Compare Performance:** Use the markdown reports for performance analysis\n")
            f.write("4. **Export Data:** CSV files are available for spreadsheet analysis\n\n")
            
            f.write("## Technical Details\n\n")
            f.write("- **Image Format:** PPM (Portable Pixmap) for maximum compatibility\n")
            f.write("- **Timing Precision:** Microsecond accuracy using high-resolution clock\n")
            f.write("- **Memory Tracking:** Peak working set size measurement\n")
            f.write("- **Statistical Analysis:** Mean and standard deviation across multiple runs\n")
    
    def run_full_documentation(self):
        """Run the complete documentation generation process."""
        print("Starting comprehensive benchmark documentation generation...")
        print(f"Resolutions: {self.resolutions}")
        print(f"Thread counts: {self.thread_counts}")
        print(f"Runs per configuration: {self.runs_per_config}")
        
        # Create output directory
        self.output_dir.mkdir(exist_ok=True)
        
        all_results = {}
        
        for resolution in self.resolutions:
            print(f"\n{'='*80}")
            print(f"Processing {resolution}x{resolution}")
            print(f"{'='*80}")
            
            resolution_results = self.generate_resolution_documentation(resolution)
            all_results[resolution] = resolution_results
        
        # Generate master documentation
        self.generate_master_documentation()
        
        print(f"\n{'='*80}")
        print("Documentation generation complete!")
        print(f"Results saved to: {self.output_dir}")
        print(f"Master report: {self.output_dir}/MASTER_BENCHMARK_REPORT.md")
        print(f"{'='*80}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate comprehensive benchmark documentation")
    parser.add_argument("--output", "-o", default="benchmark_output", 
                       help="Output directory for documentation")
    parser.add_argument("--resolutions", nargs="+", type=int, default=[256, 512, 1024, 2048],
                       help="Resolutions to test")
    parser.add_argument("--threads", nargs="+", default=[1, 2, 4, 8, 14, "MAX"],
                       help="Thread counts to test")
    parser.add_argument("--runs", type=int, default=3,
                       help="Number of runs per configuration")
    
    args = parser.parse_args()
    
    # Check if benchmark executable exists
    if not os.path.exists("build/bin/terrain-gen-benchmark.exe"):
        print("Error: Benchmark executable not found at build/bin/terrain-gen-benchmark.exe")
        print("Please build the benchmark executable first.")
        return 1
    
    # Create and run documentation generator
    generator = DocumentationGenerator(output_dir=args.output)
    generator.resolutions = args.resolutions
    generator.thread_counts = args.threads
    generator.runs_per_config = args.runs
    
    try:
        generator.run_full_documentation()
    except KeyboardInterrupt:
        print("\nDocumentation generation interrupted by user")
        return 1
    except Exception as e:
        print(f"Error generating documentation: {e}")
        return 1

if __name__ == "__main__":
    exit(main())
