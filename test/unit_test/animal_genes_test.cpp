#include "animal_genes.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Animal Genes Random Generation", "[AnimalGenes]") {
  std::mt19937 rng(42);

  SECTION("Boundary conditions check") {
    for (int i = 0; i < 50; ++i) {
      AnimalGenes genes = AnimalGenes::GenerateRandom(rng);

      REQUIRE(genes.thirst_threshold >= 0.0f);
      REQUIRE(genes.thirst_threshold <= 1.0f);

      REQUIRE(genes.energy_threshold >= 0.0f);
      REQUIRE(genes.energy_threshold <= 1.0f);

      REQUIRE(genes.libido_threshold >= 0.0f);
      REQUIRE(genes.libido_threshold <= 1.0f);

      REQUIRE(genes.restlessness >= 0.0f);
      REQUIRE(genes.restlessness <= 1.0f);

      REQUIRE(genes.libido_rate >= 0.001f);
      REQUIRE(genes.action_delay >= 1.0f);
    }
  }
}

TEST_CASE("Animal Genes Inheritance", "[AnimalGenes]") {
  std::mt19937 rng(42);

  SECTION("Boundary conditions check") {
    AnimalGenes p1;
    p1.thirst_threshold = 0.8f;
    p1.energy_threshold = 0.3f;
    p1.libido_threshold = 0.5f;
    p1.restlessness = 0.3f;
    p1.libido_rate = 0.1f;
    p1.action_delay = 10.0f;

    AnimalGenes p2;
    p2.thirst_threshold = 0.8f;
    p2.energy_threshold = 0.4f;
    p2.libido_threshold = 0.6f;
    p2.restlessness = 0.4f;
    p2.libido_rate = 0.15f;
    p2.action_delay = 20.0f;

    for (int i = 0; i < 50; ++i) {
      AnimalGenes baby = AnimalGenes::Inherit(p1, p2, rng);

      REQUIRE(baby.thirst_threshold >= 0.0f);
      REQUIRE(baby.thirst_threshold <= 1.0f);

      REQUIRE(baby.energy_threshold >= 0.0f);
      REQUIRE(baby.energy_threshold <= 1.0f);

      REQUIRE(baby.libido_threshold >= 0.0f);
      REQUIRE(baby.libido_threshold <= 1.0f);

      REQUIRE(baby.restlessness >= 0.0f);
      REQUIRE(baby.restlessness <= 1.0f);

      REQUIRE(baby.libido_rate >= 0.001f);
      REQUIRE(baby.action_delay >= 1.0f);
    }
  }

  SECTION("Boundary conditions check with extreme parents") {
    AnimalGenes extreme_parent;
    extreme_parent.thirst_threshold = 1.0f;
    extreme_parent.energy_threshold = 0.0f;
    extreme_parent.restlessness = 0.0f;
    extreme_parent.libido_rate = 0.001f;
    extreme_parent.action_delay = 1.0f;

    for (int i = 0; i < 50; ++i) {
      AnimalGenes baby =
          AnimalGenes::Inherit(extreme_parent, extreme_parent, rng);

      REQUIRE(baby.thirst_threshold <= 1.0f);
      REQUIRE(baby.energy_threshold >= 0.0f);
      REQUIRE(baby.restlessness >= 0.0f);
      REQUIRE(baby.libido_rate >= 0.001f);
      REQUIRE(baby.action_delay >= 1.0f);
    }
  }
}