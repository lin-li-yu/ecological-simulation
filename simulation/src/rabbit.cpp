#include "rabbit.hpp"
#include "grid.hpp"

Rabbit::Rabbit(Coords start_location, const AnimalGenes &initial_genes,
               const AnimalStatus &status, uint64_t id)
    : Animal(start_location, initial_genes, status, traits_, id){};

const AnimalTraits &Rabbit::GetTraits() const { return traits_; }

int Rabbit::GetSpeciesId() const { return 0; }

const std::vector<LandType> &Rabbit::GetForbiddenTiles() const {
  return GetForbiddenTilesStatic();
}

const std::vector<LandType> &Rabbit::GetForbiddenTilesStatic() {
  // Allocated on heap
  static const std::vector<LandType> forbidden = {LandType::Water};
  return forbidden;
}

AnimalState Rabbit::DetermineState(const Grid &grid) const {
  if (thirst_ >= 1.0f || energy_ <= 0.0f) {
    return AnimalState::Dead;
  } else if (thirst_ >= 0.9f) { // Critical thirst threshold
    return AnimalState::Thirsty;
  } else if (energy_ <= 0.1f) { // Critical energy threshold
    return AnimalState::Hungry;
  } else if (IsScared(grid)) {
    return AnimalState::Scared;
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

Coords Rabbit::DetermineTarget(const Grid &grid, std::mt19937 &rng) const {
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
          return (grid.GetLandTypeAt(checked_coord) == LandType::Grass);
        });
    break;
  case AnimalState::Libidinous:
    target = grid.SearchGrid(location_, traits_.sense_radius,
                             [&](Coords checked_coord) {
                               if (checked_coord == location_) {
                                 return grid.GetTotalRabbitsAt(checked_coord) >
                                        1; // Must not mate with self
                               }
                               return grid.GetTotalRabbitsAt(checked_coord) > 0;
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
  case AnimalState::Scared: {
    Coords bush_loc = grid.SearchGrid(
        location_, traits_.sense_radius, [&](Coords checked_coord) {
          return (grid.GetLandTypeAt(checked_coord) == LandType::Bush);
        });

    if (bush_loc != Coords{-1, -1}) {
      target = bush_loc;
    } else {
      // No bush is found
      // Calculate the opposite direction from the fox
      Coords fox_loc = grid.SearchGrid(
          location_, traits_.sense_radius, [&](Coords checked_coord) {
            return (grid.GetTotalFoxesAt(checked_coord) > 0);
          });

      if (fox_loc != Coords{-1, -1}) {
        int dir_x = fox_loc.first - location_.first;
        int dir_y = fox_loc.second - location_.second;
        target = {location_.first - dir_x, location_.second - dir_y};
      } else {
        target = location_;
      }
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

void Rabbit::Interact(Grid &grid,
                      std::vector<std::unique_ptr<Animal>> &newborns,
                      int current_time, std::mt19937 &rng, std::function<uint64_t()> id_generator) {
  switch (state_) {
  case AnimalState::Thirsty:
    DrinkInteract(grid);
    break;
  case AnimalState::Hungry:
    EatInteract(grid, current_time);
    break;
  case AnimalState::Libidinous:
    MateInteract<Rabbit>(grid, newborns, rng, id_generator);
    break;
  default:
    break;
  }
  // Reset wait time
  wait_time_ = static_cast<int>(genes_.action_delay);
}

void Rabbit::EatInteract(Grid &grid, int current_time) {
  if (grid.GetLandTypeAt(location_) == LandType::Grass) {
    energy_ = 1.0f;
    grid.ConsumeGrassAt(location_, current_time);
  }
}

bool Rabbit::IsScared(const Grid &grid) const {
  Coords fox_loc = grid.SearchGrid(
      location_, traits_.sense_radius, [&](Coords checked_coord) {
        return (grid.GetTotalFoxesAt(checked_coord) > 0);
      });
  if (fox_loc != Coords{-1, -1}) {
    return true;
  }
  return false;
}
