#include "animal_genes.hpp"
#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Grid land tiles mechanics", "[Grid]") {
  std::mt19937 rng(42);
  GridConfig config = utils::CreateTestingGridConfig();
  config.regrowth_delay = 5;

  Grid grid(config, rng);

  SECTION("IsInBounds - Enforcing grid dimensions") {
    // Valid bounds
    REQUIRE(grid.IsInBounds({0, 0}) == true);
    REQUIRE(grid.IsInBounds({9, 9}) == true);
    REQUIRE(grid.IsInBounds({5, 5}) == true);

    // Invalid bounds
    REQUIRE(grid.IsInBounds({-1, 5}) == false);
    REQUIRE(grid.IsInBounds({5, -1}) == false);
    REQUIRE(grid.IsInBounds({10, 5}) == false);
    REQUIRE(grid.IsInBounds({5, 10}) == false);
  }

  SECTION("PlaceNTiles - Placing tiles when map is full") {
    REQUIRE_THROWS_AS(grid.PlaceNTiles(101, LandType::Grass),
                      std::out_of_range);
  }

  SECTION("PlaceNTiles - Placing tiles") {
    grid.PlaceNTiles(25, LandType::Grass);
    grid.PlaceNTiles(25, LandType::Water);
    grid.PlaceNTiles(25, LandType::Bush);
    int grass_count = 0;
    int water_count = 0;
    int bush_count = 0;
    int ground_count = 0;
    for (int i = 0; i < 100; i++) {
      if (grid.GetLandTypeAt({i % 10, i / 10}) == LandType::Grass)
        grass_count++;
      else if (grid.GetLandTypeAt({i % 10, i / 10}) == LandType::Water)
        water_count++;
      else if (grid.GetLandTypeAt({i % 10, i / 10}) == LandType::Bush)
        bush_count++;
      else if (grid.GetLandTypeAt({i % 10, i / 10}) == LandType::Ground)
        ground_count++;
    }
    REQUIRE(grass_count == 25);
    REQUIRE(water_count == 25);
    REQUIRE(bush_count == 25);
    REQUIRE(ground_count == 25);
  }

  SECTION("GetRandomGroundTile - Testing no ground tile available") {
    grid.PlaceNTiles(100, LandType::Water);
    REQUIRE_THROWS_AS(grid.GetRandomGroundTile(), std::runtime_error);
  }

  SECTION("GetRandomGroundTile - Getting ground tile") {
    grid.PlaceNTiles(99, LandType::Water);
    Coords tile = grid.GetRandomGroundTile();
    REQUIRE(grid.GetLandTypeAt(tile) == LandType::Ground);
  }

  SECTION("GetRandomValidTile - Avoiding forbidden types") {
    grid.PlaceNTiles(50, LandType::Water);
    grid.PlaceNTiles(30, LandType::Bush);
    std::vector<LandType> forbidden = {LandType::Water, LandType::Bush};
    Coords tile = grid.GetRandomValidTile(forbidden);
    REQUIRE(std::find(forbidden.begin(), forbidden.end(),
                      grid.GetLandTypeAt(tile)) == forbidden.end());
  }

  SECTION("GetRandomValidTile - Testing no valid tile available") {
    grid.PlaceNTiles(100, LandType::Water);
    std::vector<LandType> forbidden = {LandType::Water};
    REQUIRE_THROWS_AS(grid.GetRandomValidTile(forbidden), std::runtime_error);
  }

  SECTION("IsNextToWater - Neighboring water tiles") {
    grid.SetLandTypeAt({5, 6}, LandType::Water);
    grid.SetLandTypeAt({6, 5}, LandType::Water);

    REQUIRE(grid.IsNextToWater({5, 5}) == true);
    REQUIRE(grid.IsNextToWater({4, 5}) == false);
    REQUIRE(grid.IsNextToWater({5, 4}) == false);
    REQUIRE_THROWS_AS(grid.IsNextToWater({11, 11}), std::out_of_range);
  }

  SECTION("IsNextToWater - Not neighboring water tiles") {
    grid.SetLandTypeAt({4, 4}, LandType::Water);
    grid.SetLandTypeAt({4, 6}, LandType::Water);
    grid.SetLandTypeAt({6, 4}, LandType::Water);
    grid.SetLandTypeAt({6, 6}, LandType::Water);

    REQUIRE(grid.IsNextToWater({5, 5}) == false);
  }
}

TEST_CASE("Grid grass growth", "[Grid]") {
  std::mt19937 rng(42);

  GridConfig config = utils::CreateTestingGridConfig();
  config.num_grass = 99;
  config.regrowth_delay = 5;
  int total_grass = 99;

  Grid grid(config, rng);

  SECTION("ConsumeGrassAt and ProcessGrassRegrowth - Testing grass lifecycle") {
    if (grid.GetLandTypeAt({5, 5}) != LandType::Grass) {
      grid.SetLandTypeAt({5, 5}, LandType::Grass);
      total_grass++;
    }
    REQUIRE(grid.GetLandTypeAt({5, 5}) == LandType::Grass);
    REQUIRE(grid.GetTotalAvailableGrass() == total_grass);

    grid.ConsumeGrassAt({5, 5}, 0);
    REQUIRE(grid.GetLandTypeAt({5, 5}) == LandType::Ground);
    REQUIRE(grid.GetTotalAvailableGrass() == total_grass - 1);

    grid.ProcessGrassRegrowth(4);
    REQUIRE(grid.GetLandTypeAt({5, 5}) == LandType::Ground);
    REQUIRE(grid.GetTotalAvailableGrass() == total_grass - 1);

    grid.ProcessGrassRegrowth(5);
    REQUIRE(grid.GetLandTypeAt({5, 5}) == LandType::Grass);
    REQUIRE(grid.GetTotalAvailableGrass() == total_grass);
  }
}

TEST_CASE("Grid pool generation", "[Grid]") {
  std::mt19937 rng(42);

  GridConfig config = utils::CreateTestingGridConfig();
  config.regrowth_delay = 5;

  Grid grid(config, rng);

  auto CountWaterTiles = [&]() {
    int count = 0;
    for (int x = 0; x < config.size_x; ++x) {
      for (int y = 0; y < config.size_y; ++y) {
        if (grid.GetLandTypeAt({x, y}) == LandType::Water) {
          count++;
        }
      }
    }
    return count;
  };

  SECTION("PlaceContinuousWater - Testing size 0") {
    grid.PlaceContinuousWater(0);
    REQUIRE(CountWaterTiles() == 0);
  }

  SECTION("PlaceContinuousWater - Testing size 1") {
    grid.PlaceContinuousWater(1);
    REQUIRE(CountWaterTiles() == 1);
  }

  SECTION("PlaceContinuousWater - Testing standard size and continuity") {
    int target = 5;
    grid.PlaceContinuousWater(target);
    REQUIRE(CountWaterTiles() == target);

    // Check Continuity
    bool all_continuous = true;
    for (int x = 0; x < config.size_x; ++x) {
      for (int y = 0; y < config.size_y; ++y) {
        Coords current = {x, y};
        if (grid.GetLandTypeAt(current) == LandType::Water) {
          if (!grid.IsNextToWater(current)) {
            all_continuous = false;
          }
        }
      }
    }
    REQUIRE(all_continuous == true);
  }

  SECTION("PlaceContinuousWater - Testing size greater than map size") {
    grid.PlaceContinuousWater(150);
    REQUIRE(CountWaterTiles() == 100);
  }

  SECTION("PlaceContinuousWater - Testing size greater than available tiles") {
    grid.PlaceNTiles(100, LandType::Grass);

    grid.PlaceContinuousWater(5);
    REQUIRE(CountWaterTiles() == 0);
  }
}

TEST_CASE("Grid spatial searching", "[Grid]") {
  std::mt19937 rng(42);
  GridConfig config = utils::CreateTestingGridConfig();
  Grid grid(config, rng);

  std::vector<LandType> forbidden = {LandType::Water};

  SECTION("GetLegalMoves") {
    SECTION("Locating at center of map") {
      auto moves = grid.GetLegalMoves({5, 5}, forbidden);
      REQUIRE(moves.size() == 4);
    }

    SECTION("Locating at corner of map") {
      auto moves = grid.GetLegalMoves({0, 0}, forbidden);
      REQUIRE(moves.size() == 2);

      bool has_right = false, has_down = false;
      for (auto m : moves) {
        if (m.first == 1 && m.second == 0)
          has_right = true;
        if (m.first == 0 && m.second == 1)
          has_down = true;
      }
      REQUIRE(has_right);
      REQUIRE(has_down);
    }

    SECTION("Locating out of bounds") {
      REQUIRE_THROWS_AS(grid.GetLegalMoves({-1, 5}, forbidden),
                        std::out_of_range);
    }

    SECTION("Locating within forbidden tiles") {
      grid.SetLandTypeAt({5, 4}, LandType::Water);
      grid.SetLandTypeAt({5, 6}, LandType::Water);
      grid.SetLandTypeAt({4, 5}, LandType::Water);
      grid.SetLandTypeAt({6, 5}, LandType::Water);

      auto moves = grid.GetLegalMoves({5, 5}, forbidden);
      REQUIRE(moves.empty() == true);
    }
  }

  SECTION("SearchGrid") {
    auto IsWater = [&](Coords c) {
      return grid.GetLandTypeAt(c) == LandType::Water;
    };

    SECTION("Testing negative radius or out of bounds center") {
      REQUIRE_THROWS_AS(grid.SearchGrid({-1, 0}, 2, IsWater),
                        std::out_of_range);
      REQUIRE_THROWS_AS(grid.SearchGrid({5, 5}, -1, IsWater),
                        std::invalid_argument);
    }

    SECTION("Testing no tile meets condition") {
      Coords result = grid.SearchGrid({5, 5}, 3, IsWater);
      REQUIRE(result == Coords{-1, -1});
    }

    SECTION("Testing center matches condition") {
      grid.SetLandTypeAt({5, 5}, LandType::Water);
      Coords result = grid.SearchGrid({5, 5}, 2, IsWater);
      REQUIRE(result == Coords{5, 5});
    }

    SECTION("Identifying the closest tile") {
      grid.SetLandTypeAt({5, 7}, LandType::Water);
      grid.SetLandTypeAt({7, 7}, LandType::Water);

      Coords result = grid.SearchGrid({5, 5}, 5, IsWater);
      REQUIRE(result == Coords{5, 7});
    }
  }

  SECTION("PlotNextMove") {
    SECTION("Testing out of bounds start or end") {
      REQUIRE_THROWS_AS(grid.PlotNextMove({-1, 0}, {5, 5}, forbidden, true),
                        std::out_of_range);
      REQUIRE_NOTHROW(
          grid.PlotNextMove({5, 5}, {10, 10}, forbidden,
                            true)); // Out of bounds end should not throw
    }

    SECTION("Testing start equals end") {
      Coords result = grid.PlotNextMove({5, 5}, {5, 5}, forbidden, true);
      REQUIRE(result == Coords{5, 5});
    }

    SECTION("Testing trapped") {
      grid.SetLandTypeAt({5, 4}, LandType::Water);
      grid.SetLandTypeAt({5, 6}, LandType::Water);
      grid.SetLandTypeAt({4, 5}, LandType::Water);
      grid.SetLandTypeAt({6, 5}, LandType::Water);

      Coords result = grid.PlotNextMove({5, 5}, {9, 9}, forbidden, true);
      REQUIRE(result == Coords{5, 5});
    }

    SECTION("Moving Towards") {
      Coords result = grid.PlotNextMove({5, 5}, {5, 9}, forbidden, true);
      REQUIRE(result == Coords{5, 6});
    }

    SECTION("Moving Away") {
      Coords result = grid.PlotNextMove({5, 5}, {5, 9}, forbidden, false);
      REQUIRE(result == Coords{5, 4});
    }

    SECTION("Testing direct path blocked") {
      grid.SetLandTypeAt({5, 6}, LandType::Water);
      Coords result = grid.PlotNextMove({5, 5}, {5, 9}, forbidden, true);
      bool stepped_around = (result == Coords{4, 5} || result == Coords{6, 5});
      REQUIRE(stepped_around == true);
    }
  }
}

TEST_CASE("Grid animal caching", "[Grid][Animal]") {
  std::mt19937 rng(42);
  GridConfig config = utils::CreateTestingGridConfig();
  Grid grid(config, rng);

  AnimalGenes genes = utils::CreateTestingGenes();
  genes.action_delay = 1.0f;

  std::vector<std::unique_ptr<Animal>> animals;

  SECTION("UpdateAnimalsCache - Caching animals") {
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));

    grid.UpdateAnimalsCache(animals);

    REQUIRE(grid.GetTotalRabbitsAt({5, 5}) == 2);
    REQUIRE(grid.GetTotalFoxesAt({5, 5}) == 1);
    REQUIRE(grid.GetTotalRabbitsAt({0, 0}) == 0);
  }

  SECTION("UpdateAnimalsCache - Ignoring out of bounds animals") {
    animals.push_back(std::make_unique<Fox>(
        Coords{15, 15}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{-1, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));

    REQUIRE_NOTHROW(grid.UpdateAnimalsCache(animals));
  }

  SECTION("GetAnimalsAt - Ignoring dead animals") {
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals[0]->SetState(AnimalState::Dead);

    grid.UpdateAnimalsCache(animals);
    std::vector<Rabbit *> living_rabbits = grid.GetAnimalsAt<Rabbit>({5, 5});

    REQUIRE(living_rabbits.size() == 1);
    REQUIRE(grid.GetTotalRabbitsAt({5, 5}) == 1);
  }

  SECTION("UpdateAnimalsCache - Clearing previous state") {
    // Round 1
    animals.push_back(std::make_unique<Rabbit>(
        Coords{2, 2}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);
    REQUIRE(grid.GetTotalRabbitsAt({2, 2}) == 1);

    // Round 2
    animals.clear();
    animals.push_back(std::make_unique<Rabbit>(
        Coords{3, 3}, genes, Animal::InitialiseStatusSpawn(genes, rng)));

    grid.UpdateAnimalsCache(animals);

    REQUIRE(grid.GetTotalRabbitsAt({2, 2}) == 0);
    REQUIRE(grid.GetTotalRabbitsAt({3, 3}) == 1);
  }
}
