# Ecological Simulation

## Overview
This repository contains a C++ library and command-line application for modelling an agent-based ecological simulation. The project simulates the complex interactions between individual animals (predators and prey) within a configurable, two-dimensional gridded environment. 

The environment consists of different terrain types (Ground, Water, Bush, and Grass). Simulated agents, currently represented by Rabbits (prey) and Foxes (predators), navigate this space governed by internal state mechanics including energy, thirst, and libido. The simulation models genetic inheritance and drift, tracks species populations, and uses both Shared Memory (OpenMP) and Distributed Memory (MPI) parallelism to efficiently compute large-scale ecosystems.

## Prerequisites and Requirements
To build and run this simulation, your system must meet the following requirements:
* **Operating System:** Ubuntu 20.04 or 22.04 (A Docker `devcontainer` is provided and highly recommended to ensure compatibility).
* **Compiler:** `g++` with full support for `C++17`.
* **Build System:** CMake version 3.21 or above.
* **Dependencies:** 
    * `Catch2`: Required for building and running the unit tests.
    * `OpenMP`: For shared-memory parallelism.
    * `MPI`: For distributed-memory parallelism.

## Build Instructions
The project uses CMake for its build system. To compile the application and the test suite from the top-level repository directory, execute the following commands:
    1. Generate the build files: `cmake -B build`
    2. Compile the project: `cmake --build build`
This will generate the main application executable (EcoSim) and the test executable (TestSimulation) inside the build/bin/ directory.

## Running the Simulation
The simulation is executed via the command line using the EcoSim executable. It accepts a variety of flags to configure the initial state of the ecosystem.

**Command line arguments:**
```
-h: Displays the help message
-N, -M: Sets the dimensions of the simulation grid
-r: Sets the size of the initial Rabbit population
-f: Sets the size of the initial Fox population
-g, -b: Sets the initial amount of random specialised land tiles for Grass and Bushes, respectively.
-p: Sets the number of contiguous pools of water to create.
-s: Sets the size of the created water pools.
-t: Sets the total number of timesteps the simulation will run for.
-o: Specifies the output file name where the simulation statistics will be stored.
-i: Sets the output interval.
```

### Examples of Usage
    Basic serial simulation: 1./build/bin/EcoSim -N 50 -M 50 -r 100 -f 20 -g 200 -b 50 -p 3 -s 15 -t 2000 -i 50 outputs/basic_run.txt1
    
    Distributed parallel simulation (MPI): 1mpirun -np 4 ./build/bin/EcoSim -N 200 -M 200 -r 1000 -f 100 -g 1500 -b 400 -p 10 -s 40 -t 5000 -i 100 -o outputs/mpi_run.txt1

### Running Unit Tests
To ensure the integrity of the simulation logic, a unit test suite is included. After building the project, run the tests using: `./build/bin/TestSimulation`
