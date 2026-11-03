#include "utils.hpp"
#include "animal_genes.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

float utils::AngleCalc(std::pair<int, int> x, std::pair<int, int> y) {
  return acos(DotProduct(x, y) / (magnitude(x) * magnitude(y)));
};

float utils::DotProduct(std::pair<int, int> x, std::pair<int, int> y) {
  return (float)(x.first * y.first + x.second * y.second);
};

float utils::magnitude(std::pair<int, int> x) {
  return sqrt((x.first * x.first) + (x.second * x.second));
};

void utils::PrintLandTile(LandType landType) {
  switch (landType) {
  case LandType::Ground:
    std::cout << "\033[33m"
              << "...."
              << "\033[0m";
    break;
  case LandType::Water:
    std::cout << "\033[34m"
              << "~~~~"
              << "\033[0m";
    break;
  case LandType::Bush:
    std::cout << "\033[32m"
              << "vvvv"
              << "\033[0m";
    break;
  case LandType::Grass:
    std::cout << "\033[92m"
              << "^^^^"
              << "\033[0m";
    break;
  default:
    std::cout << "?";
    break;
  }
};

void utils::PrintHelp() {
  std::cout << "EcoSim - Ecological Predator-Prey Simulation\n\n"
            << "Usage: EcoSim [options]\n\n"
            << "Options:\n"
            << "  -h          Show this help message and exit.\n"
            << "  -N <int>    Width of the grid (size_x).\n"
            << "  -M <int>    Height of the grid (size_y).\n"
            << "  -r <int>    Initial rabbit population.\n"
            << "  -f <int>    Initial fox population.\n"
            << "  -g <int>    Initial amount of grass tiles.\n"
            << "  -b <int>    Initial amount of bush tiles.\n"
            << "  -p <int>    Number of water pools.\n"
            << "  -s <int>    Size of the water pools.\n"
            << "  -d <int>    Delay for grass regrowth.\n"
            << "  -t <int>    Total timesteps to run the simulation.\n"
            << "  -o <str>    Output file name to store results.\n"
            << "  -i <int>    Output interval (timesteps between prints).\n";
}

bool utils::ParseArguments(int argc, char **argv, SimulationConfig &sim_config,
                           GridConfig &grid_config, int rank) {
  if (argc <= 1) {
    if (rank == 0) {
     std::cerr << "Error: No arguments provided\n\n";
    }
    return false;
  }

  
  for (int i = 1; i < argc; ++i) {
    // argv[0] is the program name
    std::string arg = argv[i];

    if (arg == "-h") {
      if (rank == 0) {
        PrintHelp();
      }
      return false;
    } else if (i + 1 < argc) {
      try {
        if (arg == "-N")
          grid_config.size_x = std::stoull(argv[++i]);
        else if (arg == "-M")
          grid_config.size_y = std::stoull(argv[++i]);
        else if (arg == "-r")
          sim_config.num_rabbits = std::stoull(argv[++i]);
        else if (arg == "-f")
          sim_config.num_foxes = std::stoull(argv[++i]);
        else if (arg == "-g")
          grid_config.num_grass = std::stoull(argv[++i]);
        else if (arg == "-b")
          grid_config.num_bush = std::stoull(argv[++i]);
        else if (arg == "-p")
          grid_config.num_pool = std::stoull(argv[++i]);
        else if (arg == "-s")
          grid_config.pool_size = std::stoull(argv[++i]);
        else if (arg == "-d")
          grid_config.regrowth_delay = std::stoull(argv[++i]);
        else if (arg == "-t")
          sim_config.total_time = std::stoull(argv[++i]);
        else if (arg == "-o")
          sim_config.output_path = argv[++i];
        else if (arg == "-i")
          sim_config.print_interval = std::stoull(argv[++i]);
        else {
          if (rank == 0)
            std::cerr << "Error: Unknown argument'" << arg << "'\n\n";
          return false;
        }
      } catch (const std::exception &e) {
        if (rank == 0)
          std::cerr << "Error: Invalid value for argument'" << arg << "'\n";
        return false;
      }
    } else {
      if (rank == 0)
        std::cerr << "Error: Missing value for argument'" << arg << "'\n\n";
      return false;
    }
  }
  return true;
}

std::string utils::AnimalStateToString(AnimalState state) {
  switch (state) {
  case AnimalState::Dead:
    return "Dead";
  case AnimalState::Thirsty:
    return "Thirsty";
  case AnimalState::Hungry:
    return "Hungry";
  case AnimalState::Libidinous:
    return "Libidinous";
  case AnimalState::Idle:
    return "Idle";
  case AnimalState::Scared:
    return "Scared";
  default:
    return "Unknown";
  }
}

std::string utils::FormatRecord(const SimulationStatisticsOutput &stats) {
  std::stringstream ss;

  // Fixed precision
  ss << std::fixed << std::setprecision(4);

  // Rabbits statistics
  ss << stats.timestep << ", " << stats.total_grass << ", "
     << stats.rabbit.total_count << ", ";
  ss << "(" << stats.rabbit.energy_threshold_mean << ", "
     << stats.rabbit.energy_threshold_dev << "), ";
  ss << "(" << stats.rabbit.thirst_threshold_mean << ", "
     << stats.rabbit.thirst_threshold_dev << "), ";
  ss << "(" << stats.rabbit.libido_threshold_mean << ", "
     << stats.rabbit.libido_threshold_dev << "), ";
  ss << "(" << stats.rabbit.restlessness_mean << ", "
     << stats.rabbit.restlessness_dev << "), ";
  ss << "(" << stats.rabbit.libido_rate_mean << ", "
     << stats.rabbit.libido_rate_dev << "), ";
  ss << "(" << stats.rabbit.action_delay_mean << ", "
     << stats.rabbit.action_delay_dev << "), ";
  // Foxes statistics
  ss << stats.fox.total_count << ", ";
  ss << "(" << stats.fox.energy_threshold_mean << ", "
     << stats.fox.energy_threshold_dev << "), ";
  ss << "(" << stats.fox.thirst_threshold_mean << ", "
     << stats.fox.thirst_threshold_dev << "), ";
  ss << "(" << stats.fox.libido_threshold_mean << ", "
     << stats.fox.libido_threshold_dev << "), ";
  ss << "(" << stats.fox.restlessness_mean << ", " << stats.fox.restlessness_dev
     << "), ";
  ss << "(" << stats.fox.libido_rate_mean << ", " << stats.fox.libido_rate_dev
     << "), ";
  ss << "(" << stats.fox.action_delay_mean << ", " << stats.fox.action_delay_dev
     << ")";

  ss << "\n";
  return ss.str();
}

void utils::InitialiseOutputFile(const std::string &file_path) {
  // Clean the file if it already exists
  std::ofstream file(file_path, std::ios::trunc);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file '" << file_path
              << "' for writing.\n";
    return;
  }

  file << "Timestep, Total Grass Available, "
       << "Rabbit Count, Rabbit Energy Threshold, Rabbit Thirst Threshold, "
          "Rabbit Libido Threshold, "
       << "Rabbit Restlessness, Rabbit Libido Rate, Rabbit Action Delay, "
       << "Fox Count, Fox Energy Threshold, Fox Thirst Threshold, Fox Libido "
          "Threshold, "
       << "Fox Restlessness, Fox Libido Rate, Fox Action Delay"
       << "\n";

  file.close();
}

void utils::AppendToFile(const std::string &record,
                         const std::string &file_path) {
  std::ofstream file(file_path, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file '" << file_path
              << "' for writing.\n";
    return;
  }
  file << record;
  file.close();
}

// Helper functions for testing
AnimalGenes utils::CreateTestingGenes() {
  AnimalGenes genes;
  genes.energy_threshold = 0.5f;
  genes.thirst_threshold = 0.5f;
  genes.libido_threshold = 0.5f;
  genes.libido_rate = 0.01f;
  genes.action_delay = 15.0f;
  genes.restlessness = 0.0f;
  return genes;
}

GridConfig utils::CreateTestingGridConfig() {
  GridConfig config;
  config.size_x = 10;
  config.size_y = 10;
  config.num_grass = 0;
  config.num_bush = 0;
  config.num_pool = 0;
  config.pool_size = 0;
  return config;
}

AnimalTraits utils::CreateTestingTraits() {
  AnimalTraits traits;
  traits.born_energy = 0.6f;
  traits.born_libido = 0.1f;
  traits.born_thirst = 0.2f;
  traits.energy_move_penalty = 0.0005f;
  traits.energy_rate = 0.5f;
  traits.thirst_rate = 0.001f;
  traits.sense_radius = 10;
  return traits;
}

AnimalStatus utils::CreateTestingStatus() {
  AnimalStatus status;
  status.energy = 1.0f;
  status.thirst = 0.0f;
  status.libido = 0.0f;
  status.wait_time = 0;
  return status;
}

void utils::Timer::reset() {
    curr = Clock::now();
}

double utils::Timer::elapsed() const {
    return std::chrono::duration_cast<Second>(Clock::now() - curr).count();
}