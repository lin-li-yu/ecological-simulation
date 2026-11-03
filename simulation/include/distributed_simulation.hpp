#pragma once
#include "simulation.hpp"

#ifndef OMPI_SKIP_MPICXX
#define OMPI_SKIP_MPICXX 1
#endif

#ifndef MPICH_SKIP_MPICXX
#define MPICH_SKIP_MPICXX 1
#endif

#if __has_include(<mpi.h>)
#include <mpi.h>
#elif __has_include(<mpi/mpi.h>)
#include <mpi/mpi.h>
#elif __has_include(<openmpi/mpi.h>)
#include <openmpi/mpi.h>
#else
#error "MPI header not found."
#endif

class DistributedSimulation : public Simulation {
public:
    DistributedSimulation(Grid &grid, SimulationConfig config, std::mt19937 rng, int rank, size_t buffer = 10);
    void run() override;

protected:
    int rank_;
    size_t buffer_size_;
    size_t start_x_, end_x_, start_y_, end_y_;
    std::vector<std::unique_ptr<Animal>> buffer_animals_;
    
    void Tick() override;
    uint64_t GenerateAnimalId() override;

private:
    // TODO: specific MPI phases
    bool IsInBuffer(Coords loc) const;
    bool IsInLocal(Coords loc) const;
    bool IsInTransfer(Coords loc) const;
    bool IsInTargetBuffer(Coords loc, int target_rank) const;
    void FilterInitialAnimals();
    void SyncAnimalsInBuffer(); 
    void SyncConsumedGrass();
    bool IsOwnerOfAnimal(Animal* animal) const;
    void ResolveInteractions();
    void MigrateAnimals();
    int GetTargetRank(Coords loc) const;
    void UpdateSimulationState() override;
    void RecordStatistics() const override;
};