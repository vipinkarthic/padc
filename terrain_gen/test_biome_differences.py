#!/usr/bin/env python3
"""
Test Biome Image Differences

This script tests if the biome images are actually different between stages.
"""

import subprocess
import json
import time
import hashlib
from pathlib import Path

def test_biome_differences():
    """Test if biome images are different between stages."""
    
    print("Testing biome image differences between stages...")
    
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
                
                print(f"\nChecking biome image differences:")
                
                # Calculate file hashes to check if they're different
                file_hashes = {}
                for biome_file in biome_files:
                    file_path = output_dir / biome_file
                    if file_path.exists():
                        with open(file_path, 'rb') as f:
                            file_content = f.read()
                            file_hash = hashlib.md5(file_content).hexdigest()
                            file_hashes[biome_file] = file_hash
                            file_size = len(file_content)
                            print(f"  - {biome_file}: {file_size} bytes, hash: {file_hash[:8]}...")
                    else:
                        print(f"  - {biome_file}: NOT FOUND")
                
                # Check for differences
                print(f"\nBiome image comparison:")
                if len(file_hashes) >= 2:
                    unique_hashes = set(file_hashes.values())
                    if len(unique_hashes) == 1:
                        print(f"  [WARNING] All biome images are identical!")
                        print(f"  This means the biome classification is not changing between stages.")
                        return False
                    else:
                        print(f"  [SUCCESS] Biome images are different between stages!")
                        print(f"  Found {len(unique_hashes)} unique images out of {len(file_hashes)} total.")
                        
                        # Show which images are different
                        hash_groups = {}
                        for filename, file_hash in file_hashes.items():
                            if file_hash not in hash_groups:
                                hash_groups[file_hash] = []
                            hash_groups[file_hash].append(filename)
                        
                        print(f"\n  Image groups (same hash = same image):")
                        for i, (hash_val, files) in enumerate(hash_groups.items(), 1):
                            print(f"    Group {i}: {', '.join(files)}")
                        
                        return True
                else:
                    print(f"  [ERROR] Not enough images to compare")
                    return False
                
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
    print("BIOME IMAGE DIFFERENCES TEST")
    print("="*60)
    
    # Check if executable exists
    if not Path("build/bin/terrain-gen-benchmark.exe").exists():
        print("Error: Benchmark executable not found!")
        print("Please build the benchmark executable first.")
        return 1
    
    success = test_biome_differences()
    
    if success:
        print(f"\n[SUCCESS] Biome images are now different between stages!")
        print("The biome classification is working correctly and showing changes.")
    else:
        print(f"\n[FAILED] Biome images are still identical between stages!")
        print("The biome classification may not be working as expected.")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())

