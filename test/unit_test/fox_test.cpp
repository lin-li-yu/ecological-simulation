#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <random>
#include <vector>

// A subclass that exposes protected methods for testing
class TestableFox : public Fox {
public:
  using Fox::DetermineState;
  using Fox::DetermineTarget;
  using Fox::EatInteract;
  using Fox::Fox;
  using Fox::GetTraits;
  using Fox::SetState;
  void SetLevels(float energy, float thirst, float libido) {
    energy_ = energy;
    thirst_ = thirst;
    libido_ = libido;
  }
  void SetWaitTime(size_t wait_time) { wait_time_ = wait_time; }
  size_t GetWaitTime() { return wait_time_; }
  float GetEnergy() { return energy_; }
};

TEST_CASE("GetForbiddenTiles", "[Fox]") {
  const auto &forbidden = Fox::GetForbiddenTilesStatic();
  REQUIRE(forbidden.size() == 2);
  REQUIRE(forbidden[0] == LandType::Water);
  REQUIRE(forbidden[1] == LandType::Bush);

  std::mt19937 rng(42);
  AnimalGenes genes = utils::CreateTestingGenes();
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  REQUIRE(fox.GetForbiddenTiles() == forbidden);
}

TEST_CASE("DetermineTarget - Testing thirsty state", "[Fox][Water]") {
  std::mt19937 rng(42);

  AnimalGenes genes = utils::CreateTestingGenes();
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  fox.SetState(AnimalState::Thirsty);

  GridConfig config = utils::CreateTestingGridConfig();
  config.size_x = 5 + fox.GetTraits().sense_radius + 10;
  config.size_y = 5 + fox.GetTraits().sense_radius + 10;
  Grid grid(config, rng);

  SECTION("Testing water available") {
    grid.SetLandTypeAt({5, 6}, LandType::Ground);
    grid.SetLandTypeAt({5, 7}, LandType::Water);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Standing on water") {
    grid.SetLandTypeAt({5, 5}, LandType::Water);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing no water available") {
    grid.SetLandTypeAt({5, 5 + fox.GetTraits().sense_radius + 1},
                       LandType::Water);
    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing hungry state", "[Fox][Rabbit]") {
  std::mt19937 rng(42);

  AnimalGenes genes = utils::CreateTestingGenes();
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  fox.SetState(AnimalState::Hungry);

  GridConfig config = utils::CreateTestingGridConfig();
  config.size_x = 5 + fox.GetTraits().sense_radius + 10;
  config.size_y = 5 + fox.GetTraits().sense_radius + 10;
  Grid grid(config, rng);

  std::vector<std::unique_ptr<Animal>> animals;

  SECTION("Testing rabbit available") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 7}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Standing on the same tile as rabbit") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing rabbit hidden by bush") {
    grid.SetLandTypeAt({5, 7}, LandType::Bush);
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 7}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{5, 7});
  }

  SECTION("Testing no rabbit available") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5 + fox.GetTraits().sense_radius + 1}, genes,
        Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing libidinous state", "[Fox]") {
  std::mt19937 rng(42);

  AnimalGenes genes = utils::CreateTestingGenes();
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  fox.SetState(AnimalState::Libidinous);

  GridConfig config = utils::CreateTestingGridConfig();
  config.size_x = 5 + fox.GetTraits().sense_radius + 10;
  config.size_y = 5 + fox.GetTraits().sense_radius + 10;
  Grid grid(config, rng);

  std::vector<std::unique_ptr<Animal>> animals;

  SECTION("Testing mate available") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 7}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Standing on the same tile as mate") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing no mate available") {
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{5, 5 + fox.GetTraits().sense_radius + 1}, genes,
        Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing idle state", "[Fox]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  SECTION("Testing zero restlessness") {
    genes.restlessness = 0.0f;
    TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
    fox.SetState(AnimalState::Idle);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing full restlessness") {
    genes.restlessness = 1.0f;
    TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
    fox.SetState(AnimalState::Idle);

    Coords target = fox.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{5, 5});

    int dx = std::abs(target.first - 5);
    int dy = std::abs(target.second - 5);
    REQUIRE((dx + dy) == 1);
  }
}

TEST_CASE("DetermineState", "[Fox]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  std::vector<std::unique_ptr<Animal>> animals;
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  animals.push_back(std::make_unique<TestableFox>(fox));

  SECTION("Testing death state") {
    // Thirst to death
    fox.SetLevels(1.0f, 1.0f, 0.0f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Dead);

    // Tired to death
    fox.SetLevels(0.0f, 0.0f, 0.0f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Dead);
  }

  SECTION("Testing thirst state") {
    fox.SetLevels(1.0f, 0.6f, 0.0f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Thirsty);
  }

  SECTION("Testing hungry state") {
    fox.SetLevels(0.4f, 0.0f, 0.0f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Hungry);
  }

  SECTION("Testing libidinous state") {
    fox.SetLevels(1.0f, 0.0f, 0.6f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Libidinous);
  }

  SECTION("Testing idle state") {
    fox.SetLevels(1.0f, 0.0f, 0.2f);
    REQUIRE(fox.DetermineState(grid) == AnimalState::Idle);
  }
}

TEST_CASE("Interaction", "[Fox]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  grid.SetLandTypeAt({5, 5}, LandType::Water);

  std::vector<std::unique_ptr<Animal>> animals;
  AnimalGenes genes = utils::CreateTestingGenes();
  TestableFox fox({5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng));
  animals.push_back(std::make_unique<TestableFox>(fox));
  fox.SetWaitTime(0);
  fox.SetState(AnimalState::Thirsty);

  SECTION("Interact - Resetting wait time after interaction") {
    fox.Interact(grid, animals, 0, rng);
    REQUIRE(fox.GetWaitTime() == fox.GetGenes().action_delay);
  }

  SECTION("EatInteract") {
    animals.clear();
    animals.push_back(std::make_unique<TestableFox>(fox));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, utils::CreateTestingGenes(),
        Animal::InitialiseStatusSpawn(utils::CreateTestingGenes(), rng)));
    grid.UpdateAnimalsCache(animals);

    fox.SetLevels(0.3f, 0.0f, 0.0f);
    fox.SetState(AnimalState::Hungry);
    fox.EatInteract(grid, 10);
    grid.UpdateAnimalsCache(animals);

    REQUIRE(fox.GetEnergy() == 1.0f);
    REQUIRE(grid.GetTotalRabbitsAt({5, 5}) == 0);
  }
}
