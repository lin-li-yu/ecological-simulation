#include "animal_genes.hpp"
#include <algorithm>
#include <cmath>

AnimalGenes AnimalGenes::GenerateRandom(std::mt19937 &rng) {
  AnimalGenes genes;
  std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
  std::normal_distribution<float> libido_dist(0.08f, 0.01f);
  std::normal_distribution<float> delay_dist(15.0f, 2.0f);

  genes.thirst_threshold = uniform_dist(rng);
  genes.energy_threshold = uniform_dist(rng);
  genes.libido_threshold = uniform_dist(rng);
  genes.restlessness = uniform_dist(rng);
  genes.libido_rate = std::max(0.001f, libido_dist(rng));
  genes.action_delay = std::max(1.0f, delay_dist(rng));

  return genes;
}

auto ApplyGenetics = [](float p1, float p2, std::mt19937 &rng) {
  float mu = (p1 + p2) / 2.0f;
  float sigma = 0.2f * mu;
  std::normal_distribution<float> dist(mu, sigma);
  return dist(rng);
};

AnimalGenes AnimalGenes::Inherit(const AnimalGenes &p1, const AnimalGenes &p2,
                                 std::mt19937 &rng) {
  AnimalGenes baby;

  baby.thirst_threshold = std::clamp(
      ApplyGenetics(p1.thirst_threshold, p2.thirst_threshold, rng), 0.0f, 1.0f);
  baby.energy_threshold = std::clamp(
      ApplyGenetics(p1.energy_threshold, p2.energy_threshold, rng), 0.0f, 1.0f);
  baby.libido_threshold = std::clamp(
      ApplyGenetics(p1.libido_threshold, p2.libido_threshold, rng), 0.0f, 1.0f);
  baby.restlessness = std::clamp(
      ApplyGenetics(p1.restlessness, p2.restlessness, rng), 0.0f, 1.0f);
  baby.libido_rate =
      std::max(0.001f, ApplyGenetics(p1.libido_rate, p2.libido_rate, rng));
  baby.action_delay =
      std::max(1.0f, ApplyGenetics(p1.action_delay, p2.action_delay, rng));

  return baby;
}