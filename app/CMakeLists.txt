add_executable(EcoSim EcoSim.cpp)
target_link_libraries(EcoSim PUBLIC 
    SimulationLib 
    OpenMP::OpenMP_CXX 
    MPI::MPI_CXX
)