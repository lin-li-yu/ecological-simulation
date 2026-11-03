#pragma once
#include "types.hpp"
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

class Animal;

class Grid {

public:
  Grid(GridConfig config, std::mt19937 &rng);
  static void ConfigValidation(const GridConfig &config);

  // Land type methods
  Coords GetSize() const;
  LandType GetLandTypeAt(Coords location) const;
  Coords GetRandomGroundTile() const;
  Coords GetRandomValidTile(const std::vector<LandType> &forbiddenTypes) const;
  bool IsInBounds(Coords location) const;
  bool IsLegalLocation(Coords location,
                       const std::vector<LandType> &forbiddenTypes) const;
  bool IsNextToWater(Coords location) const;
  void PrintGrid() const;
  void SetLandTypeAt(Coords location, LandType landType);
  void PlaceNTiles(size_t nTiles, LandType landType);
  void PlaceContinuousWater(size_t target_size);

  // Grass regrowth methods
  size_t GetTotalAvailableGrass() const;
  void ConsumeGrassAt(Coords loc, int current_time);
  void ProcessGrassRegrowth(int current_time);
  std::vector<Coords> ExtractRecentlyConsumedGrass(); // For MPI
  void SyncGhostGrass(const std::vector<Coords>& global_consumed, int current_time); // For MPI
  void QuietConsumeGrassAt(Coords loc, int current_time); // For MPI

  // Movement methods
  Coords PlotNextMove(Coords start, Coords end,
                      const std::vector<LandType> &forbiddenTypes,
                      bool moveTowards = true) const;
  std::vector<Coords>
  GetLegalMoves(Coords location,
                const std::vector<LandType> &forbiddenTypes) const;
  Coords SearchGrid(Coords center, int radius,
                    std::function<bool(Coords)> condition) const;

  // Animals cache methods
  template <typename T> std::vector<T *> GetAnimalsAt(Coords location) const;
  size_t GetTotalRabbitsAt(Coords location) const;
  size_t GetTotalFoxesAt(Coords location) const;
  void ClearAnimalsCache();
  void UpdateAnimalsCache(const std::vector<std::unique_ptr<Animal>> &animals);
  void UpdateAnimalsCacheRaw(const std::vector<Animal*> &animals);

protected:
  GridConfig config_;
  std::mt19937 &rng_;
  std::vector<LandType> land_tiles_;
  std::vector<GridAnimalCacheCell> animals_cache_;
  std::vector<int> animals_cache_dirty_indices_;
  std::vector<Coords> move_vector_;
  std::queue<GrassTask> growing_grass_;
  std::vector<Coords> recently_consumed_grass_;

  int CoordToIndex(Coords location) const;
};
