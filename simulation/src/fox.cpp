#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "utils.hpp"

Fox::Fox(Coords start_location, const AnimalGenes &genes,
         const AnimalStatus &status, uint64_t id)
    : Animal(start_location, genes, status, traits_, id){};

const AnimalTraits &Fox::GetTraits() const { return traits_; }

int Fox::GetSpeciesId() const { return 1; }

const std::vector<LandType> &Fox::GetForbiddenTiles() const {
  return GetForbiddenTilesStatic();
}

const std::vector<LandType> &Fox::GetForbiddenTilesStatic() {
  // Allocated on heap
  static const std::vector<LandType> forbidden = {LandType::Water,
                                                  LandType::Bush};
  return forbidden;
}

AnimalState Fox::DetermineState(const Grid &grid) const {
  if (thirst_ >= 1.0f || energy_ <= 0.0f) {
    return AnimalState::Dead;
  } else if (thirst_ >= genes_.thirst_threshold) {
    return AnimalState::Thirsty;
  } else if (energy_ < genes_.energy_threshold) {
    return AnimalState::Hungry;
  } else if (libido_ >= genes_.libido_threshold) {
    return AnimalState::Libidinous;
  } else {
    return AnimalState::Idle;
  }
}

Coords Fox::DetermineTarget(const Grid &grid, std::mt19937 &rng) const {
  Coords target = {-1, -1};
  switch (state_) {
  case AnimalState::Thirsty:
    target = grid.SearchGrid(
        location_, traits_.sense_radius, [&](Coords checked_coord) {
          return (grid.GetLandTypeAt(checked_coord) == LandType::Water);
        });
    break;
  case AnimalState::Hungry:
    target = grid.SearchGrid(
        location_, traits_.sense_radius, [&](Coords checked_coord) {
          return (grid.GetTotalRabbitsAt(checked_coord) > 0 &&
                  grid.GetLandTypeAt(checked_coord) != LandType::Bush);
        });
    break;
  case AnimalState::Libidinous:
    target = grid.SearchGrid(location_, traits_.sense_radius,
                             [&](Coords checked_coord) {
                               if (checked_coord == location_) {
                                 return grid.GetTotalFoxesAt(checked_coord) >
                                        1; // Must not mate with self
                               }
                               return grid.GetTotalFoxesAt(checked_coord) > 0;
                             });
    break;
  case AnimalState::Idle: {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) < genes_.restlessness) { // Restlessness check
      target = GetRandomStep(grid, rng, GetForbiddenTiles());
    } else {
      target = location_;
    }
    break;
  }
  default:
    target = location_;
  }

  if (target == Coords{-1, -1}) {
    return GetRandomStep(grid, rng, GetForbiddenTiles());
  }
  return target;
}

void Fox::Interact(Grid &grid, std::vector<std::unique_ptr<Animal>> &newborns,
                   int current_time, std::mt19937 &rng, std::function<uint64_t()> id_generator) {
  switch (state_) {
  case AnimalState::Thirsty:
    DrinkInteract(grid);
    break;
  case AnimalState::Hungry:
    EatInteract(grid, current_time);
    break;
  case AnimalState::Libidinous:
    MateInteract<Fox>(grid, newborns, rng, id_generator);
    break;
  default:
    break;
  }
  wait_time_ = static_cast<int>(genes_.action_delay);
}

void Fox::EatInteract(Grid &grid, int current_time) {
  std::vector<Rabbit *> rabbits = grid.GetAnimalsAt<Rabbit>(location_);
  for (Rabbit *ptr : rabbits) {
    if (ptr->GetState() != AnimalState::Dead) {
      ptr->SetState(AnimalState::Dead);
      energy_ = 1.0f;
      break;
    }
  }
}