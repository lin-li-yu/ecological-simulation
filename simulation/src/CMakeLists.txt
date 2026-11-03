add_library(SimulationLib STATIC 
    grid.cpp
    animal.cpp
    animal_genes.cpp
    fox.cpp
    rabbit.cpp
    simulation.cpp
    utils.cpp
    distributed_simulation.cpp
)

target_include_directories(SimulationLib PUBLIC "${CMAKE_SOURCE_DIR}/simulation/include")