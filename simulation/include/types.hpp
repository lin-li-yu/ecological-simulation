#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class Animal;

enum struct AnimalState { Dead, Thirsty, Hungry, Libidinous, Idle, Scared };

struct AnimalTraits {
  size_t sense_radius;
  float energy_move_penalty;
  float energy_rate;
  float thirst_rate;
  float born_energy;
  float born_thirst;
  float born_libido;
};

struct AnimalStatus {
  float energy;
  float thirst;
  float libido;
  int wait_time;
};

using Coords = std::pair<int, int>;

enum struct LandType { Ground, Water, Bush, Grass };

struct GrassTask {
  Coords location;
  int ready_time;
};

struct GridAnimalCacheCell {
  std::vector<Animal*> animals;
  int rabbit_count = 0;
  int fox_count = 0;

  void clear() {
      animals.clear();
      rabbit_count = 0;
      fox_count = 0;
  }
};

struct GridConfig {
  size_t size_x = 50;
  size_t size_y = 50;
  size_t num_grass = 30;
  size_t num_bush = 10;
  size_t num_pool = 3;
  size_t pool_size = 5;
  size_t regrowth_delay = 1000;
};

struct SimulationConfig {
  size_t num_rabbits = 20;
  size_t num_foxes = 5;
  size_t total_time = 3000;
  std::string output_path = "statistics_output.txt";
  size_t print_interval = 10;
};

struct AnimalStatisticsOutput {
  int total_count;
  float energy_threshold_mean;
  float energy_threshold_dev;
  float thirst_threshold_mean;
  float thirst_threshold_dev;
  float libido_threshold_mean;
  float libido_threshold_dev;
  float restlessness_mean;
  float restlessness_dev;
  float libido_rate_mean;
  float libido_rate_dev;
  float action_delay_mean;
  float action_delay_dev;
};

struct SimulationStatisticsOutput {
  int timestep;
  int total_grass;
  AnimalStatisticsOutput rabbit;
  AnimalStatisticsOutput fox;
};