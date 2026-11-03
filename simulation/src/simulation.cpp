#include "simulation.hpp"
#include "animal_genes.hpp"
#include "fox.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

Simulation::Simulation(Grid &grid, SimulationConfig config, std::mt19937 rng)
    : grid_(grid), config_(config), rng_(rng) {
  ConfigValidation(config_);
  InitialiseAnimals();
  grid_.UpdateAnimalsCache(animals_);
  utils::InitialiseOutputFile(config_.output_path);
};

Simulation::~Simulation() {
  // TODO: Destructor cleanup
}

void Simulation::InitialiseAnimals() {
  for (int i = 0; i < config_.num_rabbits; i++) {
    Coords spawn_loc =
        grid_.GetRandomValidTile(Rabbit::GetForbiddenTilesStatic());
    AnimalGenes genes = AnimalGenes::GenerateRandom(rng_);
    AnimalStatus status = Animal::InitialiseStatusSpawn(genes, rng_);
    animals_.push_back(std::make_unique<Rabbit>(spawn_loc, genes, status, GenerateAnimalId()));
  }

  for (int i = 0; i < config_.num_foxes; i++) {
    Coords spawn_loc = grid_.GetRandomValidTile(Fox::GetForbiddenTilesStatic());
    AnimalGenes genes = AnimalGenes::GenerateRandom(rng_);
    AnimalStatus status = Animal::InitialiseStatusSpawn(genes, rng_);
    animals_.push_back(std::make_unique<Fox>(spawn_loc, genes, status, GenerateAnimalId()));
  }
}

void Simulation::run() {
  while (current_time_ <= config_.total_time) {
    Tick();

    // Print grid in terminal
    if (current_time_ % config_.print_interval == 0) {
      UpdateSimulationState();
      PrintSimulationState();
    }

    // Record statistics to output file
    if (current_time_ % 10 == 0) {
      UpdateSimulationState();
      RecordStatistics();
    }

    current_time_++;
  }
}

void Simulation::PrintSimulationState() const {
  grid_.PrintGrid();
  std::cout << "--- Timestep: " << current_time_ << " ---" << std::endl;
  std::cout << "Total Rabbits: " << total_rabbits_ << "  ";
  std::cout << "Total Foxes: " << total_foxes_ << "  ";
  std::cout << "Total Grass: " << total_grass_ << std::endl << std::endl;
  std::cout << "----------------" << std::endl << std::endl;
}

// TODO: Add OpenMP parallelization
void Simulation::Tick() {
  // Grass regrowth
  grid_.ProcessGrassRegrowth(current_time_);

  // Safe to parallelize: StateUpdate is self-mutator
  #pragma omp parallel for default(none) shared(animals_, grid_)
  for (size_t i = 0; i < animals_.size(); i++) {
      animals_[i]->StateUpdate(grid_);
  }

  // Unsafe to parallelize: SeekTarget accepts rng_
  for (auto &animal : animals_)
    if (animal->IsReadyToAct())
      animal->SeekTarget(grid_, rng_);

  // Safe to parallelize: MoveStep is self-mutator; PlotNextMove is pure reader
  #pragma omp parallel for default(none) shared(animals_, grid_)
  for (size_t i = 0; i < animals_.size(); i++) {
      if (animals_[i]->IsReadyToAct()) {
          animals_[i]->MoveStep(grid_, animals_[i]->GetForbiddenTiles());
      }
  }

  // Update animals locations in cache
  grid_.UpdateAnimalsCache(animals_);

  // Unsafe to parallelize: Interact accepts rng_ and has too many global mutations
  std::vector<std::unique_ptr<Animal>> newborns;
  for (size_t i = 0; i < animals_.size(); i++) {
    if (animals_[i]->IsReadyToAct()) {
      animals_[i]->Interact(grid_, newborns, current_time_, rng_, [this]() { return GenerateAnimalId(); });
    }
  }

  for (auto &baby : newborns)
    animals_.push_back(std::move(baby));

  // Update animals vector and cache
  CleanupCorpses();
  grid_.UpdateAnimalsCache(animals_);
}

void Simulation::CleanupCorpses() {
  animals_.erase(std::remove_if(animals_.begin(), animals_.end(),
                                [](const std::unique_ptr<Animal> &animal) {
                                  return animal->GetState() ==
                                         AnimalState::Dead;
                                }),
                 animals_.end());
}

int Simulation::GetCurrentTime() const { return current_time_; }

void Simulation::UpdateSimulationState() {
  // Calculate the simulation state for print per interval
  std::pair<int, int> counts = CountAnimals();
  total_rabbits_ = counts.first;
  total_foxes_ = counts.second;
  total_grass_ = grid_.GetTotalAvailableGrass();
}

std::pair<int, int> Simulation::CountAnimals() const {
  int rabbit_count = 0;
  int fox_count = 0;

  #pragma omp parallel for default(none) shared(animals_) reduction(+:rabbit_count, fox_count)
  for (int i = 0; i < animals_.size(); i++) {
    if (animals_[i]->GetState() == AnimalState::Dead)
      continue;

    if (animals_[i]->GetSpeciesId() == 0) {
      rabbit_count++;
    } else if (animals_[i]->GetSpeciesId() == 1) {
      fox_count++;
    }
  }
  return {rabbit_count, fox_count};
}

void Simulation::RecordStatistics() const {
    std::vector<float> sums = CalculateLocalGeneSums();
    WriteStatisticsToFile(sums);
}

uint64_t Simulation::GenerateAnimalId() {
    return animal_counter_++; 
}

void Simulation::WriteStatisticsToFile(const std::vector<float>& global_sums) const {
    SimulationStatisticsOutput stats;
    stats.timestep = current_time_;
    stats.total_grass = total_grass_;

    auto calc_stats = [](float &mean, float &dev, float sum, float sq_sum, int count) {
        if (count > 0) {
            mean = sum / count;
            float var = (sq_sum / count) - (mean * mean);
            dev = std::sqrt(std::max(0.0f, var));
        } else {
            mean = dev = 0.0f;
        }
    };

    // Rabbits: index 0 to 11
    stats.rabbit.total_count = total_rabbits_;
    calc_stats(stats.rabbit.energy_threshold_mean, stats.rabbit.energy_threshold_dev, 
               global_sums[0], global_sums[1], total_rabbits_);
    calc_stats(stats.rabbit.thirst_threshold_mean, stats.rabbit.thirst_threshold_dev, 
               global_sums[2], global_sums[3], total_rabbits_);
    calc_stats(stats.rabbit.libido_threshold_mean, stats.rabbit.libido_threshold_dev,
           global_sums[4], global_sums[5], total_rabbits_);
    calc_stats(stats.rabbit.restlessness_mean, stats.rabbit.restlessness_dev,
           global_sums[6], global_sums[7], total_rabbits_);
    calc_stats(stats.rabbit.libido_rate_mean, stats.rabbit.libido_rate_dev,
           global_sums[8], global_sums[9], total_rabbits_);
    calc_stats(stats.rabbit.action_delay_mean, stats.rabbit.action_delay_dev,
           global_sums[10], global_sums[11], total_rabbits_);

    // Foxes: index 12 to 23
    stats.fox.total_count = total_foxes_;
    calc_stats(stats.fox.energy_threshold_mean, stats.fox.energy_threshold_dev, 
               global_sums[12], global_sums[13], total_foxes_);
    calc_stats(stats.fox.thirst_threshold_mean, stats.fox.thirst_threshold_dev,
           global_sums[14], global_sums[15], total_foxes_);
    calc_stats(stats.fox.libido_threshold_mean, stats.fox.libido_threshold_dev,
           global_sums[16], global_sums[17], total_foxes_);
    calc_stats(stats.fox.restlessness_mean, stats.fox.restlessness_dev,
           global_sums[18], global_sums[19], total_foxes_);
    calc_stats(stats.fox.libido_rate_mean, stats.fox.libido_rate_dev,
           global_sums[20], global_sums[21], total_foxes_);
    calc_stats(stats.fox.action_delay_mean, stats.fox.action_delay_dev,
           global_sums[22], global_sums[23], total_foxes_);

    std::string formatted_record = utils::FormatRecord(stats);
    utils::AppendToFile(formatted_record, config_.output_path);
}

std::vector<float> Simulation::CalculateLocalGeneSums() const {
    std::vector<float> sums(24, 0.0f);

    #pragma omp parallel for default(none) shared(animals_, sums) 
  for (size_t i = 0; i < animals_.size(); i++) {
        const auto& animal = animals_[i];
        if (animal->GetState() == AnimalState::Dead) continue;

        if (animal->GetSpeciesId() == 0) { // Rabbit
            AnimalGenes g = animal->GetGenes();
            #pragma omp atomic
            sums[0] += g.energy_threshold;
            #pragma omp atomic
            sums[1] += (g.energy_threshold * g.energy_threshold);
            #pragma omp atomic
            sums[2] += g.thirst_threshold;
            #pragma omp atomic
            sums[3] += (g.thirst_threshold * g.thirst_threshold);
      #pragma omp atomic
      sums[4] += g.libido_threshold;
      #pragma omp atomic
      sums[5] += (g.libido_threshold * g.libido_threshold);
      #pragma omp atomic
      sums[6] += g.restlessness;
      #pragma omp atomic
      sums[7] += (g.restlessness * g.restlessness);
      #pragma omp atomic
      sums[8] += g.libido_rate;
      #pragma omp atomic
      sums[9] += (g.libido_rate * g.libido_rate);
      #pragma omp atomic
      sums[10] += g.action_delay;
      #pragma omp atomic
      sums[11] += (g.action_delay * g.action_delay);
    } else if (animal->GetSpeciesId() == 1) { // Fox
            AnimalGenes g = animal->GetGenes();
            #pragma omp atomic
            sums[12] += g.energy_threshold;
            #pragma omp atomic
            sums[13] += (g.energy_threshold * g.energy_threshold);
      #pragma omp atomic
      sums[14] += g.thirst_threshold;
      #pragma omp atomic
      sums[15] += (g.thirst_threshold * g.thirst_threshold);
      #pragma omp atomic
      sums[16] += g.libido_threshold;
      #pragma omp atomic
      sums[17] += (g.libido_threshold * g.libido_threshold);
      #pragma omp atomic
      sums[18] += g.restlessness;
      #pragma omp atomic
      sums[19] += (g.restlessness * g.restlessness);
      #pragma omp atomic
      sums[20] += g.libido_rate;
      #pragma omp atomic
      sums[21] += (g.libido_rate * g.libido_rate);
      #pragma omp atomic
      sums[22] += g.action_delay;
      #pragma omp atomic
      sums[23] += (g.action_delay * g.action_delay);
        }
    }
    return sums;
}

void Simulation::ConfigValidation(const SimulationConfig &sim_config) {

  // current_time_ is int, so total_time/print_interval must fit in int domain
  const size_t int_max = static_cast<size_t>(std::numeric_limits<int>::max());

  if (sim_config.total_time == 0) {
    throw std::invalid_argument("Invalid simulation time: total_time must be greater than 0.");
  }

  // Integer overflow check
  if (sim_config.total_time > int_max) {
    throw std::invalid_argument("Invalid simulation time: total_time is too large.");
  }

  if (sim_config.print_interval == 0) {
    throw std::invalid_argument("Invalid print interval: print_interval must be greater than 0.");
  }
  if (sim_config.print_interval > int_max) {
    throw std::invalid_argument("Invalid print interval: value is too large.");
  }
  if (sim_config.print_interval > sim_config.total_time) {
    throw std::invalid_argument("Invalid print interval: print_interval cannot be greater than total_time.");
  }

  if (sim_config.num_rabbits > int_max) {
    throw std::invalid_argument("Invalid rabbit count: num_rabbits is too large.");
  }
  if (sim_config.num_foxes > int_max) {
    throw std::invalid_argument("Invalid fox count: num_foxes is too large.");
  }
}
