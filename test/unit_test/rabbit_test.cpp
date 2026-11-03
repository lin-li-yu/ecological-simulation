#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <random>
#include <vector>

// A subclass that exposes protected methods for testing
class TestableRabbit : public Rabbit {
public:
  using Rabbit::DetermineState;
  using Rabbit::DetermineTarget;
  using Rabbit::EatInteract;
  using Rabbit::GetTraits;
  using Rabbit::Rabbit;
  using Rabbit::SetState;
  void SetLevels(float energy, float thirst, float libido) {
    energy_ = energy;
    thirst_ = thirst;
    libido_ = libido;
  }
  void SetWaitTime(size_t wait_time) { wait_time_ = wait_time; }
  size_t GetWaitTime() { return wait_time_; }
  float GetEnergy() { return energy_; }
  float GetThirst() { return thirst_; }
  float GetCriticalEnergyThreshold() { return energy_crit_threshold_; }
  float GetCriticalThirstThreshold() { return thirst_crit_threshold_; }
};

TEST_CASE("DetermineTarget - Testing hungry state", "[Rabbit][Grass]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();
  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  rabbit.SetState(AnimalState::Hungry);

  SECTION("Testing grass available") {
    grid.SetLandTypeAt({5, 6}, LandType::Ground);
    grid.SetLandTypeAt({5, 7}, LandType::Grass);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Standing on grass") {
    grid.SetLandTypeAt({5, 5}, LandType::Grass);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing no grass available") {
    grid.SetLandTypeAt({5, 5 + rabbit.GetTraits().sense_radius + 1},
                       LandType::Grass);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing thirsty state", "[Rabbit][Water]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();
  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  rabbit.SetState(AnimalState::Thirsty);

  SECTION("Testing pool available") {
    grid.SetLandTypeAt({5, 6}, LandType::Ground);
    grid.SetLandTypeAt({5, 7}, LandType::Water);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Testing no water available") {
    grid.SetLandTypeAt({5, 5 + rabbit.GetTraits().sense_radius + 1},
                       LandType::Water);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing libidinous state", "[Rabbit]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  rabbit.SetState(AnimalState::Libidinous);

  std::vector<std::unique_ptr<Animal>> animals;

  SECTION("Testing mate available") {
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 7}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Standing on the same tile as mate") {
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing no mate available") {
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5 + rabbit.GetTraits().sense_radius + 1}, genes,
        Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);
    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{-1, -1});
    REQUIRE(target != Coords{5, 5});
  }
}

TEST_CASE("DetermineTarget - Testing idle state", "[Rabbit]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  SECTION("Testing zero restlessness") {
    genes.restlessness = 0.0f;
    TestableRabbit rabbit({5, 5}, genes,
                          Animal::InitialiseStatusSpawn(genes, rng));
    rabbit.SetState(AnimalState::Idle);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Testing full restlessness") {
    genes.restlessness = 1.0f;
    TestableRabbit rabbit({5, 5}, genes,
                          Animal::InitialiseStatusSpawn(genes, rng));
    rabbit.SetState(AnimalState::Idle);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target != Coords{5, 5});

    int dx = std::abs(target.first - 5);
    int dy = std::abs(target.second - 5);
    REQUIRE((dx + dy) == 1);
  }
}

TEST_CASE("DetermineTarget - Testing scared state", "[Rabbit][Fox][Bush]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  rabbit.SetState(AnimalState::Scared);

  std::vector<std::unique_ptr<Animal>> animals;

  SECTION("Testing bush available") {
    grid.SetLandTypeAt({5, 7}, LandType::Bush);
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{6, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 7});
  }

  SECTION("Locating on bush") {
    grid.SetLandTypeAt({5, 5}, LandType::Bush);
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{6, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = rabbit.DetermineTarget(grid, rng);
    REQUIRE(target == Coords{5, 5});
  }

  SECTION("Fleeing from fox") {
    grid.SetLandTypeAt({5, 5 + rabbit.GetTraits().sense_radius + 1},
                       LandType::Bush);
    animals.push_back(std::make_unique<Rabbit>(
        Coords{5, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    animals.push_back(std::make_unique<Fox>(
        Coords{6, 6}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    Coords target = rabbit.DetermineTarget(grid, rng);
    INFO("Target: (" << target.first << ", " << target.second << ")");
    REQUIRE(target == Coords{4, 4});
  }
}

TEST_CASE("DetermineState", "[Rabbit][Fox]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalGenes genes = utils::CreateTestingGenes();

  std::vector<std::unique_ptr<Animal>> animals;
  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  animals.push_back(std::make_unique<TestableRabbit>(rabbit));

  SECTION("Testing death state") {
    // Thirst to death
    rabbit.SetLevels(1.0f, 1.0f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Dead);

    // Tired to death
    rabbit.SetLevels(0.0f, 0.0f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Dead);
  }

  SECTION("Testing scared state with critical needs") {
    animals.push_back(std::make_unique<Fox>(
        Coords{6, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    // Thirst critical threshold = 0.9
    rabbit.SetLevels(1.0f, rabbit.GetCriticalThirstThreshold() + 0.001f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Thirsty);

    // Energy critical threshold = 0.1
    rabbit.SetLevels(rabbit.GetCriticalEnergyThreshold() - 0.001f, 0.0f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Hungry);
  }

  SECTION("Testing scared state") {
    animals.push_back(std::make_unique<Fox>(
        Coords{6, 5}, genes, Animal::InitialiseStatusSpawn(genes, rng)));
    grid.UpdateAnimalsCache(animals);

    rabbit.SetLevels(1.0f, 0.6f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Scared);

    rabbit.SetLevels(0.4f, 0.0f, 0.0f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Scared);
  }

  SECTION("Testing thirst > hunger > libido state priority") {
    rabbit.SetLevels(0.4f, 0.6f, 0.6f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Thirsty);

    rabbit.SetLevels(0.4f, 0.0f, 0.6f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Hungry);

    rabbit.SetLevels(1.0f, 0.0f, 0.6f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Libidinous);
  }

  SECTION("Testing idle state") {
    rabbit.SetLevels(1.0f, 0.0f, 0.4f);
    REQUIRE(rabbit.DetermineState(grid) == AnimalState::Idle);
  }
}

TEST_CASE("Interaction", "[Rabbit]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  grid.SetLandTypeAt({5, 5}, LandType::Grass);

  std::vector<std::unique_ptr<Animal>> animals;
  AnimalGenes genes = utils::CreateTestingGenes();
  TestableRabbit rabbit({5, 5}, genes,
                        Animal::InitialiseStatusSpawn(genes, rng));
  animals.push_back(std::make_unique<TestableRabbit>(rabbit));
  rabbit.SetWaitTime(0);
  rabbit.SetState(AnimalState::Hungry);

  SECTION("Interact - Reseting wait time after interaction") {
    rabbit.Interact(grid, animals, 0, rng);
    REQUIRE(rabbit.GetWaitTime() == rabbit.GetGenes().action_delay);
  }

  SECTION("EatInteract") {
    rabbit.SetLevels(0.3f, 0.0f, 0.0f);
    rabbit.EatInteract(grid, 10);
    REQUIRE(rabbit.GetEnergy() == 1.0f);
    REQUIRE(grid.GetLandTypeAt({5, 5}) == LandType::Ground);
  }
}