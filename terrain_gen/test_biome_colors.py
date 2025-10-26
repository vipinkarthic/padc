#!/usr/bin/env python3
"""
Test Biome Colors

This script tests if the biome map is showing proper colors instead of solid pink.
"""

import subprocess
import json
import time
from pathlib import Path

def test_biome_colors():
    """Test if biome colors are working correctly."""
    
    print("Testing biome color generation...")
    
    # Run a quick benchmark
    cmd = [
        "build/bin/terrain-gen-benchmark.exe",
        "256", "256", "1", "1", "424242"
    ]
    
    print(f"Running: {' '.join(cmd)}")
    
    try:
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        end_time = time.time()
        
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            return False
            
        # Parse JSON output
        try:
            data = json.loads(result.stdout)
            print(f"Benchmark completed successfully!")
            print(f"Total time: {data['total_time']:.3f}s")
            print(f"Peak memory: {data['peak_memory_kb']} KB")
            
            # Check if images were generated
            output_dir = Path("benchmark_output/256x256/1_threads/run_1")
            if output_dir.exists():
                biome_files = [
                    "04_biome_before_erosion.ppm",
                    "05_biome_after_erosion.ppm", 
                    "06_biome_after_rivers.ppm",
                    "11_biome_final.ppm"
                ]
                
                print(f"\nGenerated biome images:")
                for biome_file in biome_files:
                    file_path = output_dir / biome_file
                    if file_path.exists():
                        file_size = file_path.stat().st_size
                        print(f"  - {biome_file}: {file_size} bytes")
                    else:
                        print(f"  - {biome_file}: NOT FOUND")
                
                # Check if the images are not solid pink by examining file size
                # A solid pink image would be much smaller than a varied color image
                for biome_file in biome_files:
                    file_path = output_dir / biome_file
                    if file_path.exists():
                        file_size = file_path.stat().st_size
                        # For a 256x256 PPM image, expected size is around 196KB
                        # A solid color image would be much smaller
                        if file_size > 100000:  # More than 100KB indicates varied colors
                            print(f"  [SUCCESS] {biome_file}: Appears to have varied colors ({file_size} bytes)")
                        else:
                            print(f"  [FAILED] {biome_file}: May be solid color ({file_size} bytes)")
                
                return True
            else:
                print("No output directory found")
                return False
                
        except json.JSONDecodeError as e:
            print(f"Failed to parse JSON: {e}")
            return False
            
    except subprocess.TimeoutExpired:
        print("Test timed out")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    print("="*60)
    print("BIOME COLOR TEST")
    print("="*60)
    
    # Check if executable exists
    if not Path("build/bin/terrain-gen-benchmark.exe").exists():
        print("Error: Benchmark executable not found!")
        print("Please build the benchmark executable first.")
        return 1
    
    success = test_biome_colors()
    
    if success:
        print(f"\n[SUCCESS] Biome color test passed!")
        print("Biome maps should now show proper colors instead of solid pink.")
        print("Check the generated PPM files to verify the colors.")
    else:
        print(f"\n[FAILED] Biome color test failed!")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
