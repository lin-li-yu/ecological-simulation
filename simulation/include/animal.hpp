#pragma once
#include "animal_genes.hpp"
#include "types.hpp"
#include "distributed_types.hpp"
#include <memory>
#include <random>
#include <vector>
#include <functional>

class Grid;

class Animal {
public:
  Animal(Coords location, const AnimalGenes &genes, const AnimalStatus &status,
         const AnimalTraits &traits, uint64_t id = 0);
  virtual ~Animal() = default;
  static AnimalStatus InitialiseStatusBorn(const AnimalTraits &traits,
                                           const AnimalGenes &genes);
  static AnimalStatus InitialiseStatusSpawn(const AnimalGenes &genes,
                                            std::mt19937 &rng);

  // Update methods
  void StateUpdate(const Grid &grid);
  void SeekTarget(const Grid &grid, std::mt19937 &rng);
  void MoveStep(const Grid &grid, const std::vector<LandType> &forbidden);
  virtual void Interact(Grid &grid,
                        std::vector<std::unique_ptr<Animal>> &animals,
                        int current_time, std::mt19937 &rng,
                        std::function<uint64_t()> id_generator = {}) = 0;

  // Getters and setters
  virtual const AnimalTraits &GetTraits() const = 0;
  virtual const std::vector<LandType> &GetForbiddenTiles() const = 0;
  virtual int GetSpeciesId() const;
  AnimalState GetState() const;
  Coords GetLocation() const;
  AnimalGenes GetGenes() const;
  uint64_t GetId() const;
  bool IsReadyToAct() const;
  void SetLibidoAfterMate();
  void SetEnergyAfterMate();
  void SetState(AnimalState new_state);

  // MPI
  AnimalData Serialize(int species_id) const;
  static std::unique_ptr<Animal> Deserialize(const AnimalData& data);

protected:
  uint64_t id_;
  Coords location_;
  Coords target_coord_;
  int wait_time_;
  float energy_;
  float thirst_;
  float libido_;
  AnimalState state_;
  AnimalGenes genes_;

  virtual void EatInteract(Grid &grid, int current_time) = 0;
  void DrinkInteract(const Grid &grid);
  template <typename T>
  void MateInteract(const Grid &grid,
                    std::vector<std::unique_ptr<Animal>> &animals,
                    std::mt19937 &rng,
                    std::function<uint64_t()> id_generator = {});
  virtual AnimalState DetermineState(const Grid &grid) const = 0;
  virtual Coords DetermineTarget(const Grid &grid, std::mt19937 &rng) const = 0;
  template <typename T> T *DetermineMate(const Grid &grid) const;
  Coords GetRandomStep(const Grid &grid, std::mt19937 &rng,
                       const std::vector<LandType> &forbidden) const;
};
