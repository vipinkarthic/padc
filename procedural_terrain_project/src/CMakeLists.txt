cmake_minimum_required(VERSION 3.16)

# Common (CPU-only) sources usable by terrain_gen and terrain_all_in_one
set(SOURCES_COMMON
  perlin.cpp
  mesh_builder.cpp
  camera.cpp
)

# Viewer-only sources (require GLFW/GLAD/OpenGL)
set(SOURCES_VIEWER
  shader.cpp
)

# CLI terrain generator (no GLFW/GL)
add_executable(terrain_gen terrain_gen.cpp ${SOURCES_COMMON})
target_include_directories(terrain_gen PRIVATE ${glm_SOURCE_DIR} ${stb_SOURCE_DIR})
# Do not link glfw/glad for the CLI generator

# Viewer target (GLFW + glad + GLM)
add_executable(terrain_viewer terrain_viewer.cpp ${SOURCES_COMMON} ${SOURCES_VIEWER})
target_include_directories(terrain_viewer PRIVATE ${glad_SOURCE_DIR}/include ${glm_SOURCE_DIR} ${stb_SOURCE_DIR})
target_link_libraries(terrain_viewer PRIVATE glfw glad ${CMAKE_THREAD_LIBS_INIT})

# All-in-one target (generate then view) — link viewer libs
add_executable(terrain_all_in_one terrain_all_in_one.cpp ${SOURCES_COMMON} ${SOURCES_VIEWER})
target_include_directories(terrain_all_in_one PRIVATE ${glad_SOURCE_DIR}/include ${glm_SOURCE_DIR} ${stb_SOURCE_DIR})
target_link_libraries(terrain_all_in_one PRIVATE glfw glad ${CMAKE_THREAD_LIBS_INIT})

# OpenMP: if found, add compile/link flags
if(OpenMP_CXX_FOUND)
  target_compile_options(terrain_gen PRIVATE ${OpenMP_CXX_FLAGS})
  target_link_libraries(terrain_gen PRIVATE OpenMP::OpenMP_CXX)

  target_compile_options(terrain_viewer PRIVATE ${OpenMP_CXX_FLAGS})
  target_link_libraries(terrain_viewer PRIVATE OpenMP::OpenMP_CXX)

  target_compile_options(terrain_all_in_one PRIVATE ${OpenMP_CXX_FLAGS})
  target_link_libraries(terrain_all_in_one PRIVATE OpenMP::OpenMP_CXX)
endif()

# Set output directories
set_target_properties(terrain_gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(terrain_viewer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(terrain_all_in_one PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
