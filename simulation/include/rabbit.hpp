#pragma once
#include "animal.hpp"
#include <random>
#include <vector>
#include <functional>

class Rabbit : public Animal {
public:
  Rabbit(Coords location, const AnimalGenes &genes, const AnimalStatus &status,
         uint64_t id = 0);
  void Interact(Grid &grid, std::vector<std::unique_ptr<Animal>> &animals,
                int current_time, std::mt19937 &rng,
                std::function<uint64_t()> id_generator = {}) override;

  const AnimalTraits &GetTraits() const override;
  const std::vector<LandType> &GetForbiddenTiles() const override;
  int GetSpeciesId() const override;
  static const std::vector<LandType> &GetForbiddenTilesStatic();

protected: // Exposed for testing
  static constexpr AnimalTraits traits_ = {
      3,       // sense_radius
      0.002f,  // energy_move_penalty
      0.0005f, // energy_rate
      0.001f,  // thirst_rate
      0.6f,    // born_energy
      0.2f,    // born_thirst
      0.2f     // born_libido
  };
  static constexpr float energy_crit_threshold_ = 0.1f;
  static constexpr float thirst_crit_threshold_ = 0.9f;

  void EatInteract(Grid &grid, int current_time) override;
  bool IsScared(const Grid &grid) const;
  Coords DetermineTarget(const Grid &grid, std::mt19937 &rng) const override;
  AnimalState DetermineState(const Grid &grid) const override;
};