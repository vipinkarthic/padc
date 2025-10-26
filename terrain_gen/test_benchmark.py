#!/usr/bin/env python3
"""
Simple test script to run a small benchmark
"""

import subprocess
import json
import time

def run_single_test(width, height, threads, run_id):
    """Run a single benchmark test"""
    cmd = [
        "build/bin/terrain-gen-benchmark.exe",
        str(width),
        str(height),
        str(threads),
        str(run_id)
    ]
    
    print(f"Running: {' '.join(cmd)}")
    start_time = time.time()
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)  # 5 minute timeout
        end_time = time.time()
        
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            return None
            
        # Parse JSON output
        try:
            data = json.loads(result.stdout)
            data["wall_clock_time"] = end_time - start_time
            return data
        except json.JSONDecodeError as e:
            print(f"Failed to parse JSON: {e}")
            print(f"Output: {result.stdout}")
            return None
            
    except subprocess.TimeoutExpired:
        print("Test timed out")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def main():
    print("Running small benchmark test...")
    
    # Test with 256x256, 1 thread
    result = run_single_test(256, 256, 1, 1)
    
    if result:
        print("\nResults:")
        print(f"Resolution: {result['resolution']}")
        print(f"Threads: {result['threads']}")
        print(f"Total time: {result['total_time']:.3f}s")
        print(f"Peak memory: {result['peak_memory_kb']} KB")
        print(f"Wall clock time: {result['wall_clock_time']:.3f}s")
        print("\nStage times:")
        for stage, time_val in result['stage_times'].items():
            print(f"  {stage}: {time_val:.3f}s")
    else:
        print("Benchmark failed!")

if __name__ == "__main__":
    main()
