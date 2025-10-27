## Dependencies
### Required Dependencies
- **CMake** 3.15 or higher
- **C++ Compiler** with C++17 support:
    - Linux: GCC 7+ or Clang 5+
    - Windows: MinGW-w64
- **OpenMP**

## Installation
### Linux
```bash
sudo apt install -y build-essential cmake
```
### Windows
1. Install MSYS2 : Download from: https://www.msys2.org/
2. Open MSYS2 MinGW shell and install packages
```bash
    pacman -Syu
    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-openmp make
```

## Building the Project
### Linux Build

```bash
cd terrain_gen
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```
### Windows Build

```bash
cd terrain_gen
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
```

## Running the Generator
### Linux
```bash
cd build
cd bin
cp -a ../../assets/. .
./terrain-gen
```
### Windows
```bash
cd build
cd bin
Copy-Item -Path ..\..\assets\* -Destination . -Recurse
.\terrain-gen.exe
```

## Output Files
The generator creates an `out/` directory with the following outputs:
### Heightmaps
- `height_before_erosion.ppm` - Initial heightmap
- `height_after_erosion.ppm` - Heightmap after hydraulic erosion
- `height_after_rivers.ppm` - Heightmap after river carving
- `height.ppm` - Final heightmap
- `erosion_eroded.ppm` - Erosion intensity map
- `erosion_deposited.ppm` - Deposition intensity map
### Biome Maps
- `biome_before_erosion.ppm` - Initial biome classification
- `biome_after_erosion.ppm` - Biomes after erosion
- `biome_after_rivers.ppm` - Biomes after river generation
- `biome.ppm` - Final biome map
### River Map
- `river_map.ppm` - River network visualization
### Viewing PPM Files
+ **Linux:**  `gimp out/height.pp`
+ **Windows:** Use Paint.net

## Configuration Reference
### config.json

```json
{
  "seed": 14355,
  "width": 256,
  "height": 256,
  "worldType": "Voronoi",
  "numPlates": 36,
  "fbmBlend": 0.42,
  "fbmFrequency": 0.0035,
  "fbmOctaves": 5,
  "oceanHeightThreshold": 0.35,
  "lakeHeightThreshold": 0.45,
  "coastDistanceTiles": 3,
  "smoothingIterations": 1
}
```

### Key Parameters to Adjust
- Increase `width` and `height`
- Adjust `fbmBlend` (higher = smoother)
- Increase `fbmOctaves` for more detail
- Adjust `oceanHeightThreshold` (lower = more ocean)