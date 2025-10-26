# PowerShell script to run terrain generation benchmarks

Write-Host "Building benchmark executable..." -ForegroundColor Green
Set-Location build
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Running benchmarks..." -ForegroundColor Green
Set-Location ..

# Check if Python is available
try {
    python --version
} catch {
    Write-Host "Python not found! Please install Python and try again." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Run the benchmark
python benchmark_runner.py --executable build/bin/terrain-gen-benchmark.exe --output benchmark_results --runs 3

Write-Host ""
Write-Host "Benchmark complete! Check the benchmark_results folder for results." -ForegroundColor Green
Read-Host "Press Enter to exit"
