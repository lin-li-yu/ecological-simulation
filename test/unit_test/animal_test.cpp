#include "animal.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include "utils.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

// A concrete and accessible subclass
class TestableAnimal : public Animal {
public:
  TestableAnimal(Coords location, const AnimalGenes &genes,
                 const AnimalStatus &status, const AnimalTraits &traits)
      : Animal(location, genes, status, traits), traits_(traits) {}

  // Expose protected methods
  using Animal::DetermineMate;
  using Animal::DrinkInteract;
  using Animal::GetRandomStep;
  using Animal::MateInteract;

  // Expose protected states
  float GetEnergy() const { return energy_; }
  float GetThirst() const { return thirst_; }
  float GetLibido() const { return libido_; }
  size_t GetWaitTime() const { return wait_time_; }
  Coords GetTargetCoord() const { return target_coord_; }
  void SetTargetCoord(Coords target) { target_coord_ = target; }
  void SetThirst(float thirst) { thirst_ = thirst; }
  void SetWaitTime(size_t wait) { wait_time_ = wait; }

  // Dummy overrides
  void Interact(Grid &, std::vector<std::unique_ptr<Animal>> &, int,
                std::mt19937 &, std::function<uint64_t()> = {}) override {}
  const AnimalTraits &GetTraits() const override { return traits_; }
  const std::vector<LandType> &GetForbiddenTiles() const override {
    static std::vector<LandType> f;
    return f;
  }

private:
  void EatInteract(Grid &, int) override {}
  AnimalState DetermineState(const Grid &) const override {
    return AnimalState::Idle;
  }
  Coords DetermineTarget(const Grid &, std::mt19937 &) const override {
    return location_;
  }

  AnimalTraits traits_;
};

class TestableRabbit : public Rabbit {
public:
  using Rabbit::Rabbit;

  using Rabbit::DetermineMate;
  using Rabbit::MateInteract;
  using Rabbit::SetState;

  float GetEnergy() const { return energy_; }
  float GetLibido() const { return libido_; }
};

TEST_CASE("Initial status generating", "[Animal]") {
  std::mt19937 rng(42);

  SECTION("InitialiseStatusBorn") {
    AnimalGenes genes = utils::CreateTestingGenes();
    genes.action_delay = 5.0f;

    AnimalTraits traits = utils::CreateTestingTraits();

    AnimalStatus status = Animal::InitialiseStatusBorn(traits, genes);

    REQUIRE(status.energy == 0.6f);
    REQUIRE(status.thirst == 0.2f);
    REQUIRE(status.libido == 0.1f);
    REQUIRE(status.wait_time == 5.0f);
  }

  SECTION("InitialiseStatusBorn - Testing invalid traits") {
    AnimalGenes genes = utils::CreateTestingGenes();
    genes.action_delay = 5.0f;

    AnimalTraits traits = utils::CreateTestingTraits();
    traits.born_energy = -0.1f;
    REQUIRE_THROWS_AS(Animal::InitialiseStatusBorn(traits, genes),
                      std::invalid_argument);
  }

  SECTION("InitialiseStatusSpawn") {
    AnimalGenes genes = utils::CreateTestingGenes();
    genes.energy_threshold = 0.6f;
    genes.libido_threshold = 0.8f;
    genes.action_delay = 10.0f;

    for (int i = 0; i < 100; ++i) {
      AnimalStatus status = Animal::InitialiseStatusSpawn(genes, rng);

      REQUIRE(status.thirst >= 0.0f);
      REQUIRE(status.thirst <= 1.0f);

      REQUIRE(status.energy >= 0.6f);
      REQUIRE(status.energy <= 1.0f);

      REQUIRE(status.libido >= 0.8f);
      REQUIRE(status.libido <= 1.0f);

      REQUIRE(status.wait_time >= 0);
      REQUIRE(status.wait_time <= 10);
    }
  }
}

TEST_CASE("StateUpdate", "[Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  AnimalGenes genes = utils::CreateTestingGenes();
  AnimalTraits traits;
  traits.born_energy = 1.0f;
  traits.born_libido = 0.0f;
  traits.born_thirst = 0.0;
  traits.energy_move_penalty = 0.0005f;
  traits.energy_rate = 0.05f;
  traits.thirst_rate = 0.05f;
  traits.sense_radius = 10;

  SECTION("Decaying animal status and wait time") {
    AnimalStatus status = utils::CreateTestingStatus();
    status.wait_time = 2;

    TestableAnimal animal({0, 0}, genes, status, traits);
    animal.SetState(AnimalState::Hungry);

    animal.StateUpdate(grid);

    REQUIRE(animal.GetWaitTime() == 1);
    REQUIRE(animal.GetEnergy() == Catch::Approx(0.95f));
    REQUIRE(animal.GetThirst() == Catch::Approx(0.05f));
    REQUIRE(animal.GetState() == AnimalState::Hungry);
  }

  SECTION("Testing state transition when wait time reaches 0") {
    AnimalStatus status = utils::CreateTestingStatus();
    status.wait_time = 1;

    TestableAnimal animal({0, 0}, genes, status, traits);
    animal.SetState(AnimalState::Hungry);

    animal.StateUpdate(grid);

    REQUIRE(animal.GetWaitTime() == 0);
    REQUIRE(animal.GetState() == AnimalState::Idle);
  }

  SECTION("Testing libido increasing") {
    genes.libido_rate = 0.1f;
    AnimalStatus status = utils::CreateTestingStatus();

    TestableAnimal animal({0, 0}, genes, status, traits);
    animal.StateUpdate(grid);

    REQUIRE(animal.GetLibido() == Catch::Approx(0.1f));
  }

  SECTION("Testing libido not increasing") {
    genes.libido_threshold = 0.1f;
    AnimalStatus status;
    status.thirst = 0.6f; // thirsty

    TestableAnimal animal({0, 0}, genes, status, traits);
    animal.StateUpdate(grid);

    REQUIRE(animal.GetLibido() == Catch::Approx(0.0f));
  }
}

TEST_CASE("MoveStep", "[Animal][Grid]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  AnimalGenes genes = utils::CreateTestingGenes();
  AnimalStatus status = utils::CreateTestingStatus();
  AnimalTraits traits = utils::CreateTestingTraits();
  traits.energy_move_penalty = 0.05f;

  SECTION("Testing successful move") {
    TestableAnimal animal({5, 5}, genes, status, traits);
    animal.SetTargetCoord({5, 9});

    animal.MoveStep(grid, {});

    REQUIRE(animal.GetLocation() == Coords{5, 6});
    REQUIRE(animal.GetEnergy() == Catch::Approx(0.95f));
  }

  SECTION("Standing on target") {
    TestableAnimal animal({5, 5}, genes, status, traits);
    animal.SetTargetCoord({5, 5});

    animal.MoveStep(grid, {});

    REQUIRE(animal.GetLocation() == Coords{5, 5});
    REQUIRE(animal.GetEnergy() == Catch::Approx(1.0f));
  }

  SECTION("Trapped by forbidden tiles") {
    TestableAnimal animal({5, 5}, genes, status, traits);
    animal.SetTargetCoord({5, 9});

    grid.SetLandTypeAt({5, 4}, LandType::Water);
    grid.SetLandTypeAt({5, 6}, LandType::Water);
    grid.SetLandTypeAt({4, 5}, LandType::Water);
    grid.SetLandTypeAt({6, 5}, LandType::Water);

    std::vector<LandType> forbidden = {LandType::Water};
    animal.MoveStep(grid, forbidden);

    REQUIRE(animal.GetLocation() == Coords{5, 5});
    REQUIRE(animal.GetEnergy() == Catch::Approx(1.0f));
  }
}

TEST_CASE("Animal interaction logic", "[Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);

  AnimalTraits traits = utils::CreateTestingTraits();
  AnimalGenes genes = utils::CreateTestingGenes();
  AnimalStatus status = utils::CreateTestingStatus();
  status.thirst = 0.8f;

  SECTION("DrinkInteract - Standing next to water") {
    TestableAnimal animal({5, 5}, genes, status, traits);
    animal.SetThirst(0.9f);
    animal.SetState(AnimalState::Thirsty);
    grid.SetLandTypeAt({5, 6}, LandType::Water);

    animal.DrinkInteract(grid);

    REQUIRE(animal.GetThirst() == Catch::Approx(0.0f));
    REQUIRE(animal.GetState() == AnimalState::Idle);
  }

  SECTION("DrinkInteract - Standing not next to water") {
    TestableAnimal animal({5, 5}, genes, status, traits);
    animal.SetThirst(0.9f);
    animal.SetState(AnimalState::Thirsty);
    grid.SetLandTypeAt({5, 7}, LandType::Water);

    animal.DrinkInteract(grid);

    REQUIRE(animal.GetThirst() == Catch::Approx(0.9f));
    REQUIRE(animal.GetState() == AnimalState::Thirsty);
  }

  SECTION("IsReadyToAct") {
    TestableAnimal animal({5, 5}, genes, status, traits);

    // Ready
    animal.SetWaitTime(0);
    animal.SetState(AnimalState::Idle);
    REQUIRE(animal.IsReadyToAct() == true);

    // Not Ready: wait time > 0
    animal.SetWaitTime(1);
    animal.SetState(AnimalState::Idle);
    REQUIRE(animal.IsReadyToAct() == false);

    // Not Ready: animal is dead
    animal.SetWaitTime(0);
    animal.SetState(AnimalState::Dead);
    REQUIRE(animal.IsReadyToAct() == false);
  }
}

TEST_CASE("DetermineMate", "[Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalStatus status = utils::CreateTestingStatus();
  AnimalGenes genes = utils::CreateTestingGenes();
  genes.action_delay = 5.0f;
  std::vector<std::unique_ptr<Animal>> grid_cache;

  SECTION("Ignoring itself") {
    TestableRabbit rabbit({5, 5}, genes, status);
    rabbit.SetState(AnimalState::Libidinous);

    grid_cache.push_back(std::make_unique<Rabbit>(Coords{5, 5}, genes, status));
    grid.UpdateAnimalsCache(grid_cache);

    REQUIRE(rabbit.DetermineMate<Rabbit>(grid) == nullptr);
  }

  SECTION("Ignoring non-libidinous animals") {
    TestableRabbit rabbit({5, 5}, genes, status);
    rabbit.SetState(AnimalState::Libidinous);

    auto unready_mate = std::make_unique<Rabbit>(Coords{5, 5}, genes, status);
    unready_mate->SetState(AnimalState::Hungry);

    grid_cache.push_back(std::move(unready_mate));
    grid.UpdateAnimalsCache(grid_cache);

    REQUIRE(rabbit.DetermineMate<Rabbit>(grid) == nullptr);
  }

  SECTION("Finding a valid mate") {
    TestableRabbit rabbit({5, 5}, genes, status);
    rabbit.SetState(AnimalState::Libidinous);

    auto ready_mate = std::make_unique<Rabbit>(Coords{5, 5}, genes, status);
    ready_mate->SetState(AnimalState::Libidinous);

    Rabbit *expected_ptr = ready_mate.get();
    grid_cache.push_back(std::move(ready_mate));
    grid.UpdateAnimalsCache(grid_cache);

    REQUIRE(rabbit.DetermineMate<Rabbit>(grid) == expected_ptr);
  }
}

TEST_CASE("MateInteract", "[Animal]") {
  std::mt19937 rng(42);
  Grid grid(utils::CreateTestingGridConfig(), rng);
  AnimalStatus status = utils::CreateTestingStatus();
  AnimalGenes genes = utils::CreateTestingGenes();
  genes.action_delay = 5.0f;

  std::vector<std::unique_ptr<Animal>> grid_cache;
  std::vector<std::unique_ptr<Animal>> newborns;

  SECTION("Testing successful mating interaction") {
    TestableRabbit rabbit({5, 5}, genes, status);
    rabbit.SetState(AnimalState::Libidinous);

    auto mate = std::make_unique<TestableRabbit>(Coords{5, 5}, genes, status);
    mate->SetState(AnimalState::Libidinous);
    TestableRabbit *mate_ptr = mate.get();

    grid_cache.push_back(std::move(mate));
    grid.UpdateAnimalsCache(grid_cache);

    rabbit.MateInteract<Rabbit>(grid, newborns, rng);

    // Check the newborn
    REQUIRE(newborns.size() == 1);
    REQUIRE(newborns[0]->GetLocation() == Coords{5, 5});

    // Check the primary parent
    REQUIRE(rabbit.GetState() == AnimalState::Idle);
    REQUIRE(rabbit.GetLibido() == Catch::Approx(0.0f));
    REQUIRE(rabbit.GetEnergy() < 1.0f);

    // Check the partner parent
    REQUIRE(mate_ptr->GetState() == AnimalState::Idle);
    REQUIRE(mate_ptr->GetLibido() == Catch::Approx(0.0f));
    REQUIRE(mate_ptr->GetEnergy() < 1.0f);
  }

  SECTION("Testing no available mate") {
    TestableRabbit rabbit({5, 5}, genes, status);
    rabbit.SetState(AnimalState::Libidinous);
    float starting_energy = rabbit.GetEnergy();
    float starting_libido = rabbit.GetLibido();

    grid.UpdateAnimalsCache(grid_cache);
    rabbit.MateInteract<Rabbit>(grid, newborns, rng);

    REQUIRE(newborns.size() == 0);
    REQUIRE(rabbit.GetState() == AnimalState::Libidinous);
    REQUIRE(rabbit.GetLibido() == Catch::Approx(starting_libido));
    REQUIRE(rabbit.GetEnergy() == Catch::Approx(starting_energy));
  }
}