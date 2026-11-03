#pragma once
#include "animal.hpp"
#include "grid.hpp"
#include <memory>
#include <random>
#include <vector>

class Simulation {
public:
  Simulation(Grid &grid, SimulationConfig config, std::mt19937 rng);
  ~Simulation();
  static void ConfigValidation(const SimulationConfig &sim_config);

  virtual void run();
  int GetCurrentTime() const;

protected:
  SimulationConfig config_;
  Grid &grid_;
  std::mt19937 rng_;
  std::vector<std::unique_ptr<Animal>> animals_;
  int total_rabbits_;
  int total_foxes_;
  int total_grass_;
  int current_time_ = 0;
  uint32_t animal_counter_ = 0;

  virtual void Tick();
  void CleanupCorpses();
  void InitialiseAnimals();
  virtual void UpdateSimulationState();
  void PrintSimulationState() const;
  std::pair<int, int> CountAnimals() const;
  virtual void RecordStatistics() const;
  virtual uint64_t GenerateAnimalId();
  std::vector<float> CalculateLocalGeneSums() const;
  void WriteStatisticsToFile(const std::vector<float>& global_sums) const;
};
