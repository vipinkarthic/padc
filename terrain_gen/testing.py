#!/usr/bin/env python3
import subprocess
import time
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Configuration
EXECUTABLE = "./build/bin/terrain-gen"
THREAD_COUNTS = [1, 2, 4, 8, 16]
RUNS_PER_TEST = 10  # Multiple runs for accuracy

def run_test(threads):
    """Run terrain generation with specified thread count."""
    env = {"OMP_NUM_THREADS": str(threads)}
    
    start = time.time()
    result = subprocess.run(
        [EXECUTABLE],
        env={**subprocess.os.environ, **env},
        capture_output=True,
        text=True
    )
    end = time.time()
    
    return end - start

def main():
    print("OpenMP Speedup Testing")
    print("=" * 50)
    
    results = []
    
    for threads in THREAD_COUNTS:
        print(f"\nTesting with {threads} thread(s)...")
        times = []
        
        for run in range(RUNS_PER_TEST):
            print(f"  Run {run + 1}/{RUNS_PER_TEST}...", end=" ", flush=True)
            elapsed = run_test(threads)
            times.append(elapsed)
            print(f"{elapsed:.2f}s")
        
        avg_time = np.mean(times)
        std_time = np.std(times)
        
        results.append({
            "threads": threads,
            "avg_time": avg_time,
            "std_time": std_time,
            "min_time": min(times),
            "max_time": max(times)
        })
        
        print(f"  Average: {avg_time:.2f}s ± {std_time:.2f}s")
    
    # Create DataFrame
    df = pd.DataFrame(results)
    
    # Calculate speedup and efficiency
    baseline = df[df["threads"] == 1]["avg_time"].values[0]
    df["speedup"] = baseline / df["avg_time"]
    df["efficiency"] = (df["speedup"] / df["threads"]) * 100
    
    # Print results table
    print("\n" + "=" * 50)
    print("RESULTS SUMMARY")
    print("=" * 50)
    print(df.to_string(index=False))
    
    # Save to CSV
    df.to_csv("speedup_analysis.csv", index=False)
    print("\nResults saved to: speedup_analysis.csv")
    
    # Generate plots
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # Plot 1: Execution Time
    axes[0].plot(df["threads"], df["avg_time"], marker='o', linewidth=2, markersize=8)
    axes[0].fill_between(
        df["threads"],
        df["avg_time"] - df["std_time"],
        df["avg_time"] + df["std_time"],
        alpha=0.3
    )
    axes[0].set_xlabel("Number of Threads", fontsize=12)
    axes[0].set_ylabel("Execution Time (seconds)", fontsize=12)
    axes[0].set_title("Execution Time vs Thread Count", fontsize=14)
    axes[0].grid(True, alpha=0.3)
    axes[0].set_xscale('log', base=2)
    
    # Plot 2: Speedup
    axes[1].plot(df["threads"], df["speedup"], marker='o', linewidth=2, 
                 markersize=8, label='Measured Speedup')
    axes[1].plot(df["threads"], df["threads"], 'k--', linewidth=1, 
                 label='Ideal Speedup', alpha=0.5)
    axes[1].set_xlabel("Number of Threads", fontsize=12)
    axes[1].set_ylabel("Speedup", fontsize=12)
    axes[1].set_title("Speedup vs Thread Count", fontsize=14)
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)
    axes[1].set_xscale('log', base=2)
    
    plt.tight_layout()
    plt.savefig("speedup_plot.png", dpi=300)
    print("Plot saved to: speedup_plot.png")
    
    plt.show()

if __name__ == "__main__":
    main()