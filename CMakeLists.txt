cmake_minimum_required(VERSION 3.16)
project(eco_sim
  VERSION 0.0.1
  LANGUAGES CXX
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# You can set this flag to `-g` when developing to enable debugging
# You should probably turn on optimisations when running large simulations for results
set(CMAKE_CXX_FLAGS "-g")

# Find dependencies
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
find_package(Catch2 3 REQUIRED)

add_subdirectory(app)
add_subdirectory(simulation)
add_subdirectory(test)
