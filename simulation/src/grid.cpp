#include "grid.hpp"
#include "animal.hpp"
#include "fox.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <set>


Grid::Grid(GridConfig config, std::mt19937 &rng) : config_(config), rng_(rng) {
  ConfigValidation(config_);

  // Initialise move vector and animals cache
  move_vector_ = {Coords(-1, 0), Coords(1, 0), Coords(0, -1), Coords(0, 1)};
  animals_cache_.resize(config_.size_x * config_.size_y);

  // Initialise land tiles
  for (unsigned int i = 0; i < config_.size_x * config_.size_y; i++) {
    land_tiles_.push_back(LandType::Ground);
  }
  for (size_t i = 0; i < config_.num_pool; i++) {
    PlaceContinuousWater(config_.pool_size);
  }
  PlaceNTiles(config_.num_grass, LandType::Grass);
  PlaceNTiles(config_.num_bush, LandType::Bush);
};

void Grid::PlaceNTiles(size_t nTiles, LandType tileType) {
  // Check there are enough Ground tiles to overwrite
  if (std::count(land_tiles_.begin(), land_tiles_.end(), LandType::Ground) <
      nTiles)
    throw std::out_of_range(
        "Requesting more tiles than are available on the map");

  for (size_t i = 0; i < nTiles; i++) {
    SetLandTypeAt(GetRandomGroundTile(), tileType);
  }
};

void Grid::PlaceContinuousWater(size_t target_size) {
  if (target_size == 0)
    return;

  // Initialise starting point
  Coords start_tile;
  try {
    start_tile = GetRandomGroundTile();
  } catch (const std::exception &e) {
    return; // No place available.
  }
  SetLandTypeAt(start_tile, LandType::Water);
  size_t current_size = 1;

  // Check valid neighbors
  std::set<Coords> valid_neighbors;
  auto addValidNeighbors = [&](Coords c) {
    for (const auto &offset : move_vector_) {
      Coords neighbor = {c.first + offset.first, c.second + offset.second};
      if (IsInBounds(neighbor) && GetLandTypeAt(neighbor) == LandType::Ground) {
        valid_neighbors.insert(neighbor);
      }
    }
  };
  addValidNeighbors(start_tile);

  // Expand the pond
  // Exit the loop early if run out of valid neighbors
  while (current_size < target_size && !valid_neighbors.empty()) {
    std::uniform_int_distribution<size_t> dist(0, valid_neighbors.size() - 1);

    // Jump to the tile
    auto it = valid_neighbors.begin();
    std::advance(it, dist(rng_));
    Coords next_tile = *it;
    valid_neighbors.erase(it);

    SetLandTypeAt(next_tile, LandType::Water);
    current_size++;
    addValidNeighbors(next_tile);
  }
}

Coords Grid::GetRandomGroundTile() const {
  if (std::count(land_tiles_.begin(), land_tiles_.end(), LandType::Ground) == 0)
    throw std::runtime_error("No ground tile to return!");

  std::uniform_int_distribution<int> x_coord(0, config_.size_x - 1);
  std::uniform_int_distribution<int> y_coord(0, config_.size_y - 1);

  Coords tile;
  do {
    tile.first = x_coord(rng_);
    tile.second = y_coord(rng_);
  } while (GetLandTypeAt(tile) != LandType::Ground);

  return tile;
};

Coords Grid::GetRandomValidTile(const std::vector<LandType> &forbiddenTypes) const {
  int invalid_count = 0;
  for (auto &landType : forbiddenTypes) {
    invalid_count +=
        std::count(land_tiles_.begin(), land_tiles_.end(), landType);
  }
  if (invalid_count >= land_tiles_.size())
    throw std::runtime_error("No valid tile to return!");

  Coords tile = Coords(-1, -1);
  std::uniform_int_distribution<int> x_coord(0, config_.size_x - 1);
  std::uniform_int_distribution<int> y_coord(0, config_.size_y - 1);

  do {
    tile.first = x_coord(rng_);
    tile.second = y_coord(rng_);
  } while (IsLegalLocation(tile, forbiddenTypes) == false);

  return tile;
}

int Grid::CoordToIndex(Coords location) const {
  if (!IsInBounds(location))
    throw std::out_of_range(
        "Location outside of bounds cannot be converted to index");
  return location.first + config_.size_x * location.second;
};

LandType Grid::GetLandTypeAt(Coords location) const {
  return land_tiles_[CoordToIndex(location)];
};

void Grid::SetLandTypeAt(Coords location, LandType landType) {
  land_tiles_[CoordToIndex(location)] = landType;
};

Coords Grid::PlotNextMove(Coords start, Coords end,
                          const std::vector<LandType> &forbiddenTypes,
                          bool moveTowards) const {
  if (!IsInBounds(start))
    throw std::out_of_range("Start or end coordinate is out of bounds");

  if (start == end)
    return start;

  std::vector<Coords> legalMoves = GetLegalMoves(start, forbiddenTypes);
  if (legalMoves.size() == 0)
    return start;

  // Calculate the vector of ideal movement to the target
  Coords idealVector =
      Coords(end.first - start.first, end.second - start.second);

  // Set up return value and value criterion
  Coords bestDirection;
  float minimumAngle = 100.;
  if (!moveTowards)
    minimumAngle = 0.;

  // Iterate over our legal moves and find the closest to the ideal
  for (auto &move : legalMoves) {
    // Calculate the movement vector, and check the angle against the ideal
    Coords candidateDirection(move.first - start.first,
                              move.second - start.second);
    float angle = utils::AngleCalc(idealVector, candidateDirection);
    if (angle < minimumAngle == moveTowards) {
      minimumAngle = angle;
      bestDirection = move;
    }
  }

  return bestDirection;
};

std::vector<Coords>
Grid::GetLegalMoves(Coords location,
                    const std::vector<LandType> &forbiddenTypes) const {
  if (!IsInBounds(location))
    throw std::out_of_range("Location is out of bounds");

  std::vector<Coords> legalMoves;
  if (!IsInBounds(location))
    return legalMoves;

  for (auto &move : move_vector_) {
    Coords locationCandidate =
        Coords(location.first + move.first, location.second + move.second);
    if (IsLegalLocation(locationCandidate, forbiddenTypes))
      legalMoves.push_back(locationCandidate);
  }
  return legalMoves;
};

bool Grid::IsLegalLocation(Coords location,
                           const std::vector<LandType> &forbiddenTypes) const {
  return (IsInBounds(location) &&
          std::find(forbiddenTypes.begin(), forbiddenTypes.end(),
                    GetLandTypeAt(location)) == forbiddenTypes.end());
};

bool Grid::IsInBounds(Coords location) const {
  return (location.first >= 0 && location.first < config_.size_x &&
          location.second >= 0 && location.second < config_.size_y);
};

Coords Grid::SearchGrid(Coords center, int radius, const std::function<bool(Coords)> condition) const {
  if (IsInBounds(center) == false)
    throw std::out_of_range("Center coordinate is out of bounds");

  if (radius < 0)
    throw std::invalid_argument("Radius cannot be negative");

  if (condition(center)) {
      return center;
  }

    Coords closest = {-1, -1};
    int min_dist_sq = std::numeric_limits<int>::max();
    bool found = false;

    // Search spirally outwards from the center
    for (int r = 1; r <= radius; ++r) {
        auto check_tile = [&](int dx, int dy) {
            Coords checked = {center.first + dx, center.second + dy};
            
            if (IsInBounds(checked) && condition(checked)) {
                int dist_sq = (dx * dx) + (dy * dy); 
                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    closest = checked;
                    found = true;
                }
            }
        };

        for (int dx = -r; dx <= r; ++dx) {
            check_tile(dx, -r); // Top
            check_tile(dx,  r); // Bottom
        }

        for (int dy = -r + 1; dy <= r - 1; ++dy) {
            check_tile(-r, dy); // Left
            check_tile( r, dy); // Right
        }

        if (found) {
            return closest;
        }
    }

    return closest; 
}

void Grid::ConsumeGrassAt(Coords loc, int current_time) {
  SetLandTypeAt(loc, LandType::Ground);
  growing_grass_.push(
      GrassTask{loc, current_time + static_cast<int>(config_.regrowth_delay)});
  
  // Track for MPI
  recently_consumed_grass_.push_back(loc);
}

void Grid::QuietConsumeGrassAt(Coords loc, int current_time) {
  SetLandTypeAt(loc, LandType::Ground);
  growing_grass_.push(
      GrassTask{loc, current_time + static_cast<int>(config_.regrowth_delay)});

  // This is used in MPI when pretending this grass is consumed by the rabbits in the owner rank
  // Not pushing it to recently_consumed_grass_ because not broadcasting this change to other ranks
  
}

void Grid::ProcessGrassRegrowth(int current_time) {
    while (!growing_grass_.empty() &&
          growing_grass_.front().ready_time <= current_time) {
      SetLandTypeAt(growing_grass_.front().location, LandType::Grass);
      growing_grass_.pop();
    }
}

std::vector<Coords> Grid::ExtractRecentlyConsumedGrass() {
  std::vector<Coords> consumed = recently_consumed_grass_;
  recently_consumed_grass_.clear();
  return consumed;
}

void Grid::SyncGhostGrass(const std::vector<Coords>& global_consumed, int current_time) {
  for (const auto& loc : global_consumed) {
    if (GetLandTypeAt(loc) == LandType::Grass) {
      SetLandTypeAt(loc, LandType::Ground);
      growing_grass_.push(GrassTask{loc, current_time + static_cast<int>(config_.regrowth_delay)});
    }
  }
}

Coords Grid::GetSize() const { return Coords(config_.size_x, config_.size_y); }

void Grid::ClearAnimalsCache() {
  // Safe to parallelize: Only clear dirty tiles and they will never overlap
  #pragma omp parallel for default(none) shared(animals_cache_, animals_cache_dirty_indices_)
  for (size_t i = 0; i < animals_cache_dirty_indices_.size(); i++) {
      int index = animals_cache_dirty_indices_[i];
      animals_cache_[index].clear();
  }
  animals_cache_dirty_indices_.clear();
}

void Grid::UpdateAnimalsCache(const std::vector<std::unique_ptr<Animal>> &animals) {
  std::vector<Animal*> raw_pointers;
    raw_pointers.reserve(animals.size());
    
    // Extract the raw pointers
    for (const auto& animal : animals) {
        raw_pointers.push_back(animal.get());
    }
  
    UpdateAnimalsCacheRaw(raw_pointers);
}

void Grid::UpdateAnimalsCacheRaw(const std::vector<Animal*> &animals) {
  ClearAnimalsCache();

  #pragma omp parallel for default(none) shared(animals, animals_cache_, animals_cache_dirty_indices_)
  for (size_t i = 0; i < animals.size(); i++) {
    if (animals[i]->GetState() == AnimalState::Dead) continue;
    Coords loc = animals[i]->GetLocation();

    if (IsInBounds(loc)) {
      int index = CoordToIndex(loc);

      bool is_rabbit = (animals[i]->GetSpeciesId() == 0);
      bool is_fox = (animals[i]->GetSpeciesId() == 1);

      // Only this section writes to shared vectors
      #pragma omp critical 
      {
        // Mark as dirty if it was empty
        if (animals_cache_[index].animals.empty()) {
            animals_cache_dirty_indices_.push_back(index);
        }
        animals_cache_[index].animals.push_back(animals[i]);
      }

      if (is_rabbit) {
          #pragma omp atomic
          animals_cache_[index].rabbit_count++;
      } else if (is_fox) {
          #pragma omp atomic
          animals_cache_[index].fox_count++;
      }

    } else {
      // TODO: Logging
    }
  }
}

template <typename T>
std::vector<T *> Grid::GetAnimalsAt(Coords location) const {
  std::vector<T *> found;

  for (Animal *animal : animals_cache_[CoordToIndex(location)].animals) {
    T *target_animal = dynamic_cast<T *>(animal);
    if (target_animal != nullptr &&
        target_animal->GetState() != AnimalState::Dead) {
      found.push_back(target_animal);
    }
  }
  return found;
}

template std::vector<Rabbit *> Grid::GetAnimalsAt<Rabbit>(Coords location) const;
template std::vector<Fox *> Grid::GetAnimalsAt<Fox>(Coords location) const;

size_t Grid::GetTotalRabbitsAt(Coords location) const {
  return animals_cache_[CoordToIndex(location)].rabbit_count;
}

size_t Grid::GetTotalFoxesAt(Coords location) const {
  return animals_cache_[CoordToIndex(location)].fox_count;
}

void Grid::PrintGrid() const {
  for (size_t j = 0; j < config_.size_y; j++) {
    // Print Animals
    std::cout << std::setfill(' ');
    for (size_t i = 0; i < config_.size_x; i++) {
      Coords current_loc(i, j);
      int nRabbits = GetTotalRabbitsAt(current_loc);
      int nFoxes = GetTotalFoxesAt(current_loc);

      // Print Rabbits
      if (nRabbits > 0) {
        std::cout << "\033[97m" << std::setw(2) << nRabbits;
      } else {
        std::cout << "  ";
      }

      // Print Foxes
      if (nFoxes > 0) {
        std::cout << "\033[91m" << std::setw(2) << nFoxes;
      } else {
        std::cout << "  ";
      }
    }
    std::cout << "\033[0m\n";

    // Print terrain
    for (size_t i = 0; i < config_.size_x; i++) {
      utils::PrintLandTile(GetLandTypeAt(Coords(i, j)));
    }
    std::cout << "\n";
  }
  std::cout << std::endl;
}

size_t Grid::GetTotalAvailableGrass() const {
  return std::count(land_tiles_.begin(), land_tiles_.end(), LandType::Grass);
}

bool Grid::IsNextToWater(Coords location) const {
  if (!IsInBounds(location))
    throw std::out_of_range("Location is out of bounds");
  for (const auto &offset : move_vector_) {
    Coords check = {location.first + offset.first,
                    location.second + offset.second};
    if (IsInBounds(check) && GetLandTypeAt(check) == LandType::Water) {
      return true;
    }
  }
  return false;
}

void Grid::ConfigValidation(const GridConfig &config) {
  if (config.size_x == 0 || config.size_y == 0) {
    throw std::invalid_argument("Invalid grid size: width and height must both be greater than 0.");
  }

  // Check for total tiles overflow
  const size_t total_tiles = config.size_x * config.size_y;
  if (config.size_x != 0 && total_tiles / config.size_x != config.size_y) {
    throw std::invalid_argument("Invalid grid size: width * height is too large.");
  }

  if (config.num_grass > total_tiles) {
    throw std::invalid_argument("Invalid grass count: number of grass tiles is larger than the grid.");
  }

  if (config.num_bush > total_tiles) {
    throw std::invalid_argument("Invalid bush count: number of bush tiles is larger than the grid.");
  }

  const size_t water_target_tiles = config.num_pool * config.pool_size;
  if (water_target_tiles >= total_tiles) {
    throw std::invalid_argument("Invalid water setup: number of pools * pool size would fill the entire grid. Leave at least one non-water tile.");
  }

  const size_t max_occupied = total_tiles - 1;
  if (config.num_bush > max_occupied - water_target_tiles ||
      config.num_grass > max_occupied - water_target_tiles - config.num_bush) {
    throw std::invalid_argument("Invalid terrain setup: grass + bush + water area is too large. Leave at least one ground tile for spawning.");
  }

  if (config.regrowth_delay == 0) {
    throw std::invalid_argument("Invalid regrowth delay: value must be greater than 0.");
  }
}
