#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "simulation.hpp"
#include "utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <random>

class TestableSimulation : public Simulation {
public:
  using Simulation::CleanupCorpses;
  using Simulation::CountAnimals;
  using Simulation::InitialiseAnimals;
  using Simulation::Simulation;
  using Simulation::Tick;

  void TestSetTime(int t) { current_time_ = t; }
  void TestAddAnimal(std::unique_ptr<Animal> animal) {
    animals_.push_back(std::move(animal));
  }
  void TestClearAnimals() { animals_.clear(); }
  size_t TestGetTotalAnimalCount() const { return animals_.size(); }
  const std::vector<std::unique_ptr<Animal>> &TestGetAnimals() const {
    return animals_;
  }
};

class TestableGrid : public Grid {
public:
  using Grid::Grid;
  void TestSetConfigNumGrass(size_t n) { config_.num_grass = n; }
};

TEST_CASE("InitialiseAnimals", "[Simulation][Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  SECTION("Spawning 0 animals") {
    SimulationConfig config;
    config.num_rabbits = 0;
    config.num_foxes = 0;

    // Called InitialiseAnimals() in constructor
    TestableSimulation sim(grid, config, rng);

    REQUIRE(sim.TestGetAnimals().empty() == true);
  }

  SECTION("Spawning requested quantities of animals") {
    SimulationConfig config;
    config.num_rabbits = 15;
    config.num_foxes = 5;

    TestableSimulation sim(grid, config, rng);

    const auto &animals = sim.TestGetAnimals();
    REQUIRE(sim.TestGetAnimals().size() == 20);

    int rabbit_count = 0;
    int fox_count = 0;
    for (const auto &animal : animals) {
      if (dynamic_cast<Rabbit *>(animal.get()) != nullptr)
        rabbit_count++;
      if (dynamic_cast<Fox *>(animal.get()) != nullptr)
        fox_count++;
    }

    REQUIRE(rabbit_count == 15);
    REQUIRE(fox_count == 5);
  }

  SECTION("Testing forbidden tile compliance") {
    SimulationConfig config;
    config.num_rabbits = 10;
    config.num_foxes = 10;

    // Half water, half bush, one ground
    for (int x = 0; x < 10; x++) {
      for (int y = 0; y < 10; y++) {
        if (x < 5)
          grid.SetLandTypeAt({x, y}, LandType::Water);
        else
          grid.SetLandTypeAt({x, y}, LandType::Bush);
      }
    }
    grid.SetLandTypeAt({9, 9}, LandType::Ground);

    TestableSimulation sim(grid, config, rng);

    for (const auto &animal : sim.TestGetAnimals()) {
      LandType terrain_below = grid.GetLandTypeAt(animal->GetLocation());

      if (dynamic_cast<Rabbit *>(animal.get()) != nullptr) {
        REQUIRE(terrain_below != LandType::Water);
      } else if (dynamic_cast<Fox *>(animal.get()) != nullptr) {
        REQUIRE(terrain_below != LandType::Water);
        REQUIRE(terrain_below != LandType::Bush);
        REQUIRE(animal->GetLocation() == Coords{9, 9});
      }
    }
  }

  SECTION("Testing grid animals cache update") {
    SimulationConfig config;
    config.num_rabbits = 3;
    config.num_foxes = 2;

    TestableSimulation sim(grid, config, rng);

    int grid_rabbits = 0;
    int grid_foxes = 0;
    for (int x = 0; x < 10; x++) {
      for (int y = 0; y < 10; y++) {
        grid_rabbits += grid.GetTotalRabbitsAt({x, y});
        grid_foxes += grid.GetTotalFoxesAt({x, y});
      }
    }

    REQUIRE(grid_rabbits == 3);
    REQUIRE(grid_foxes == 2);
  }
}

TEST_CASE("Simulation animal tracking and memory management",
          "[Simulation][Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  SimulationConfig config;
  config.num_rabbits = 0;
  config.num_foxes = 0;
  TestableSimulation sim(grid, config, rng);

  AnimalGenes genes = utils::CreateTestingGenes();
  AnimalStatus status = utils::CreateTestingStatus();

  auto spawn_rabbit = [&](AnimalState state) {
    auto r = std::make_unique<Rabbit>(Coords{5, 5}, genes, status);
    r->SetState(state);
    return r;
  };

  auto spawn_fox = [&](AnimalState state) {
    auto f = std::make_unique<Fox>(Coords{5, 5}, genes, status);
    f->SetState(state);
    return f;
  };

  SECTION("CleanupCorpses - Ignoring living animals") {
    sim.TestClearAnimals();
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Idle));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Hungry));
    sim.TestAddAnimal(spawn_fox(AnimalState::Thirsty));

    sim.CleanupCorpses();

    REQUIRE(sim.TestGetTotalAnimalCount() == 3);
  }

  SECTION("CleanupCorpses - Removing dead animals") {
    sim.TestClearAnimals();
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Idle));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Dead));
    sim.TestAddAnimal(spawn_fox(AnimalState::Dead));
    sim.TestAddAnimal(spawn_fox(AnimalState::Hungry));

    sim.CleanupCorpses();

    REQUIRE(sim.TestGetTotalAnimalCount() == 2);
  }

  SECTION("CleanupCorpses - Cleaning empty animal vector") {
    sim.TestClearAnimals();

    REQUIRE_NOTHROW(sim.CleanupCorpses());
    REQUIRE(sim.TestGetTotalAnimalCount() == 0);
  }

  SECTION("CountAnimals - Counting animals by species") {
    sim.TestClearAnimals();

    sim.TestAddAnimal(spawn_rabbit(AnimalState::Idle));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Hungry));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Scared));

    sim.TestAddAnimal(spawn_fox(AnimalState::Libidinous));
    sim.TestAddAnimal(spawn_fox(AnimalState::Idle));

    std::pair<int, int> counts = sim.CountAnimals();

    REQUIRE(counts.first == 3);
    REQUIRE(counts.second == 2);
  }

  SECTION("CountAnimals - Ignoring dead animals") {
    sim.TestClearAnimals();

    sim.TestAddAnimal(spawn_rabbit(AnimalState::Idle));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Dead));
    sim.TestAddAnimal(spawn_rabbit(AnimalState::Dead));

    sim.TestAddAnimal(spawn_fox(AnimalState::Idle));
    sim.TestAddAnimal(spawn_fox(AnimalState::Hungry));
    sim.TestAddAnimal(spawn_fox(AnimalState::Dead));

    std::pair<int, int> counts = sim.CountAnimals();

    REQUIRE(counts.first == 1);
    REQUIRE(counts.second == 2);
  }
}

TEST_CASE("Tick", "[Simulation][Animal][Grid]") {
  std::mt19937 rng(42);
  GridConfig g_config = utils::CreateTestingGridConfig();
  g_config.regrowth_delay = 5;
  TestableGrid grid(g_config, rng);

  SimulationConfig s_config;
  s_config.num_rabbits = 0;
  s_config.num_foxes = 0;
  TestableSimulation sim(grid, s_config, rng);

  AnimalGenes genes = utils::CreateTestingGenes();
  genes.action_delay = 0;

  SECTION("Testing full integration cycle (death, birth, and action in one "
          "timestamp)") {
    // Grass regrowth setup
    grid.SetLandTypeAt({0, 0}, LandType::Grass);
    grid.TestSetConfigNumGrass(1);
    grid.ConsumeGrassAt({0, 0}, 0);
    REQUIRE(grid.GetLandTypeAt({0, 0}) == LandType::Ground);

    // Dying rabbit setup
    AnimalStatus starving = utils::CreateTestingStatus();
    starving.energy = 0.0f;

    auto dying_rabbit = std::make_unique<Rabbit>(Coords{1, 1}, genes, starving);
    dying_rabbit->SetState(AnimalState::Hungry);
    sim.TestAddAnimal(std::move(dying_rabbit));

    // Two libidinous foxes setup
    AnimalStatus horny = utils::CreateTestingStatus();
    horny.libido = 1.0f;
    horny.wait_time = 0;

    auto fox1 = std::make_unique<Fox>(Coords{5, 5}, genes, horny);
    fox1->SetState(AnimalState::Libidinous);
    sim.TestAddAnimal(std::move(fox1));

    auto fox2 = std::make_unique<Fox>(Coords{5, 5}, genes, horny);
    fox2->SetState(AnimalState::Libidinous);
    sim.TestAddAnimal(std::move(fox2));

    // Verify setup
    grid.UpdateAnimalsCache(sim.TestGetAnimals());
    REQUIRE(sim.TestGetTotalAnimalCount() == 3);

    // Execution
    sim.TestSetTime(5);
    sim.Tick();

    // Grass regrowth verification
    REQUIRE(grid.GetLandTypeAt({0, 0}) == LandType::Grass);

    // Dying rabbit and mating foxes verification
    // Total = 2 parent foxes + 1 baby fox - 1 dead rabbit
    INFO("Foxes at (5, 5): " << grid.GetTotalFoxesAt({5, 5}));
    INFO("Rabbits at (1, 1): " << grid.GetTotalRabbitsAt({1, 1}));
    REQUIRE(sim.TestGetTotalAnimalCount() == 3);

    // Grid cache update verification
    REQUIRE(grid.GetTotalRabbitsAt({1, 1}) == 0);
    REQUIRE(grid.GetTotalFoxesAt({5, 5}) == 3);
  }
}