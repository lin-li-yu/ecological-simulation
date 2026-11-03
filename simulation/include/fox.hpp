#pragma once
#include "animal.hpp"
#include <random>
#include <vector>

class Fox : public Animal {
public:
  Fox(Coords location, const AnimalGenes &genes, const AnimalStatus &status,
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
      10,      // sense_radius
      0.004f,  // energy_move_penalty
      0.0005f, // energy_rate
      0.001f,  // thirst_rate
      0.4f,    // born_energy
      0.2f,    // born_thirst
      0.0f     // born_libido
  };

  void EatInteract(Grid &grid, int current_time) override;
  Coords DetermineTarget(const Grid &grid, std::mt19937 &rng) const override;
  AnimalState DetermineState(const Grid &grid) const override;
};