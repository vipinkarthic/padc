@echo off
echo Building benchmark executable...
cd build
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Running benchmarks...
cd ..
python benchmark_runner.py --executable build/bin/terrain-gen-benchmark.exe --output benchmark_results --runs 3

echo.
echo Benchmark complete! Check the benchmark_results folder for results.
pause
