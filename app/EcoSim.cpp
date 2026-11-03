#include "distributed_simulation.hpp"
#include "grid.hpp"
#include "simulation.hpp"
#include "utils.hpp"
#include <mpi.h>
#include <string>

int main(int argc, char **argv) {
  try {
    // MPI Initialisation
    MPI_Init(&argc, &argv);
    int rank, n_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_rank);

    utils::Timer timer;

    // Use a fixed seed to ensure all ranks generate the same initial grid and
    // animals The seed will later be reseeded in the DistributedSimulation
    // constructor
    std::mt19937 rng(42);

    SimulationConfig sim_config;
    GridConfig grid_config;

    // Parse arguments in every rank
    if (!utils::ParseArguments(argc, argv, sim_config, grid_config, rank)) {
      MPI_Finalize();
      return 0;
    }

    Grid::ConfigValidation(grid_config);
    Simulation::ConfigValidation(sim_config);

    Grid grid(grid_config, rng);

    // MPI rank factory
    std::unique_ptr<Simulation> sim;
    if (n_rank == 1) {
      sim = std::make_unique<Simulation>(grid, sim_config, rng);
    } else if (n_rank == 4) {
      sim = std::make_unique<DistributedSimulation>(grid, sim_config, rng, rank,
                                                    10);
    } else {
      if (rank == 0)
        std::cerr << "Error: Simulation only supports 1 or 4 MPI processes.\n";

      MPI_Finalize();
      return 1;
    }

    sim->run();

    if (rank == 0) {
      std::cout << "Time: " << timer.elapsed() << '\n';
    }

    MPI_Finalize();

  } catch (const std::exception &e) {
    std::cerr << "Unexpected Error: " << e.what() << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 1);
    return 1;
  }
  return 0;
}