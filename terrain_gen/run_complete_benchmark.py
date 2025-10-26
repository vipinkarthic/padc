import subprocess
import json
import time
from pathlib import Path
import statistics
import sys

class Benchamrker:
    def __init__(self):
        self.resolutions = [2048]
        self.thread_counts = [1, 2, 4, 8, 14, "MAX"]
        self.runs_per_config = 3
        self.output_dir = Path("benchmark_output")
        
    def get_max_threads(self) -> int:
        import os
        return os.cpu_count()
    
    def run_benchmark_with_images(self, width: int, height: int, threads: int, run_id: int, seed: int = 424242) -> dict:
        cmd = [
            "build/bin/terrain-gen-benchmark.exe",
            str(width), str(height), str(threads), str(run_id), str(seed)
        ]
         
        
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)  # 1 hour timeout
        end_time = time.time()
         
        benchmark_data = json.loads(result.stdout)
        benchmark_data["wall_clock_time"] = end_time - start_time
        return benchmark_data
    
    def calculate_averages(self, results: list) -> dict:
        if not results:
            return {}
        
        avg_result = results[0].copy()
        
        numeric_fields = ["total_time", "peak_memory_kb", "wall_clock_time"]
        for field in numeric_fields:
            if field in avg_result:
                values = [r.get(field, 0) for r in results]
                avg_result[field] = statistics.mean(values)
        
        if "stage_times" in avg_result:
            stage_times = {}
            for stage in avg_result["stage_times"].keys():
                values = [r.get("stage_times", {}).get(stage, 0) for r in results]
                stage_times[stage] = statistics.mean(values)
            avg_result["stage_times"] = stage_times
        
        avg_result["num_runs"] = len(results)
        avg_result["std_dev_total_time"] = statistics.stdev([r.get("total_time", 0) for r in results]) if len(results) > 1 else 0
        
        return avg_result
     
    def get_slowest_stage(self, result: dict) -> str:
        """Get the name of the slowest processing stage."""
        stage_times = result.get("stage_times", {})
        if not stage_times:
            return "Unknown"
        
        slowest_stage = max(stage_times.items(), key=lambda x: x[1])
        return slowest_stage[0].replace('_', ' ').title()
    
    def run_resolution_benchmark(self, resolution: int):
        print(f"\n{'='*80}")
        print(f"Processing {resolution}x{resolution}")
        print(f"{'='*80}")
        
        resolution_dir = self.output_dir / f"{resolution}x{resolution}"
        resolution_dir.mkdir(parents=True, exist_ok=True)
        
        all_results = {}
        
        for thread_count in self.thread_counts:
            actual_threads = self.get_max_threads() if thread_count == "MAX" else thread_count
            thread_key = f"{actual_threads}_threads"
            
            print(f"\nTesting with {actual_threads} threads...")
            
            run_results = []
            for run_id in range(1, self.runs_per_config + 1):
                print(f"  Run {run_id}/{self.runs_per_config}...")
                
                result = self.run_benchmark_with_images(
                    resolution, resolution, actual_threads, run_id
                )
                
                if result:
                    run_results.append(result)
                    time.sleep(1)
            
            if run_results:
                avg_result = self.calculate_averages(run_results)
                all_results[thread_key] = avg_result
                
                result_file = resolution_dir / f"{thread_key}_result.json"
                with open(result_file, 'w') as f:
                    json.dump(avg_result, f, indent=2)        
        
        return all_results
    
    def run_complete_benchmark(self):
        print("Starting comprehensive benchmark documentation generation...")
        print(f"Resolutions: {self.resolutions}")
        print(f"Thread counts: {self.thread_counts}")
        print(f"Runs per configuration: {self.runs_per_config}")
        print(f"Total test cases: {len(self.resolutions) * len(self.thread_counts) * self.runs_per_config}")
        
        self.output_dir.mkdir(exist_ok=True)
        
        all_results = {}
        
        for resolution in self.resolutions:
            resolution_results = self.run_resolution_benchmark(resolution)
            all_results[resolution] = resolution_results
        
def main():
    if not Path("build/bin/terrain-gen-benchmark.exe").exists():
        return 1
    
    runner = Benchamrker()
    runner.run_complete_benchmark()

if __name__ == "__main__":
    main()
