#pragma once
#include <iostream>
#include <random>

struct AnimalGenes {
  float energy_threshold;
  float thirst_threshold;
  float libido_threshold;
  float libido_rate;
  float action_delay;
  float restlessness;

  static AnimalGenes GenerateRandom(std::mt19937 &rng);
  static AnimalGenes Inherit(const AnimalGenes &parent1,
                             const AnimalGenes &parent2, std::mt19937 &rng);
};
