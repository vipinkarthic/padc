#!/usr/bin/env python3
"""
Quick test script for documentation generation
"""

import subprocess
import json
import time
from pathlib import Path

def run_single_documentation_test():
    """Run a single test case with image generation for documentation."""
    print("Running single documentation test...")
    
    # Test with 256x256, 1 thread
    cmd = [
        "build/bin/terrain-gen-benchmark.exe",
        "256", "256", "1", "1", "424242"
    ]
    
    print(f"Running: {' '.join(cmd)}")
    start_time = time.time()
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        end_time = time.time()
        
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            return False
            
        # Parse JSON output
        try:
            data = json.loads(result.stdout)
            print(f"\nBenchmark completed successfully!")
            print(f"Total time: {data['total_time']:.3f}s")
            print(f"Peak memory: {data['peak_memory_kb']} KB")
            print(f"Wall clock time: {end_time - start_time:.3f}s")
            
            # Check if images were generated
            output_dir = Path("benchmark_output/256x256/1_threads/run_1")
            if output_dir.exists():
                image_files = list(output_dir.glob("*.ppm"))
                print(f"\nGenerated {len(image_files)} images:")
                for img_file in sorted(image_files):
                    print(f"  - {img_file.name}")
                return True
            else:
                print("No output directory found")
                return False
                
        except json.JSONDecodeError as e:
            print(f"Failed to parse JSON: {e}")
            print(f"Output: {result.stdout}")
            return False
            
    except subprocess.TimeoutExpired:
        print("Test timed out")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    print("Testing documentation generation system...")
    
    # Check if executable exists
    if not Path("build/bin/terrain-gen-benchmark.exe").exists():
        print("Error: Benchmark executable not found!")
        print("Please build the benchmark executable first.")
        return 1
    
    success = run_single_documentation_test()
    
    if success:
        print("\n[SUCCESS] Documentation test passed!")
        print("Images and JSON data generated successfully.")
        print("Check the 'benchmark_output' directory for results.")
    else:
        print("\n[FAILED] Documentation test failed!")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
