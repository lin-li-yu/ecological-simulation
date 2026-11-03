#include "animal.hpp"
#include "fox.hpp"
#include "grid.hpp"
#include "rabbit.hpp"
#include <random>
#include <stdexcept>
#include <functional>

Animal::Animal(Coords location, const AnimalGenes &genes,
               const AnimalStatus &status, const AnimalTraits &traits,
               uint64_t id) {
  // No validation on inputs, as they are validated during creation
  id_ = id;
  location_ = location;
  genes_ = genes;
  energy_ = status.energy;
  thirst_ = status.thirst;
  libido_ = status.libido;
  wait_time_ = static_cast<size_t>(status.wait_time);
  state_ = AnimalState::Idle;
}

AnimalStatus Animal::InitialiseStatusBorn(const AnimalTraits &traits,
                                          const AnimalGenes &genes) {
  AnimalStatus status;
  if (traits.born_energy < 0.0f || traits.born_energy > 1.0f ||
      traits.born_thirst < 0.0f || traits.born_thirst > 1.0f ||
      traits.born_libido < 0.0f || traits.born_libido > 1.0f) {
    throw std::invalid_argument("Born status traits must be between 0 and 1");
  }
  status.energy = traits.born_energy;
  status.thirst = traits.born_thirst;
  status.libido = traits.born_libido;
  status.wait_time = genes.action_delay;
  return status;
}

AnimalStatus Animal::InitialiseStatusSpawn(const AnimalGenes &genes,
                                           std::mt19937 &rng) {
  AnimalStatus status;
  std::uniform_real_distribution<float> thirst_dist(0.0f, 1.0f);
  status.thirst = thirst_dist(rng);

  std::uniform_real_distribution<float> energy_dist(genes.energy_threshold,
                                                    1.0f);
  status.energy = energy_dist(rng);

  std::uniform_real_distribution<float> libido_dist(genes.libido_threshold,
                                                    1.0f);
  status.libido = libido_dist(rng);

  std::uniform_int_distribution<int> wait_dist(
      0, static_cast<int>(genes.action_delay));
  status.wait_time = wait_dist(rng);
  return status;
}

AnimalState Animal::GetState() const { return state_; }

Coords Animal::GetRandomStep(const Grid &grid, std::mt19937 &rng,
                             const std::vector<LandType> &forbidden) const {
  std::vector<Coords> legal_moves = grid.GetLegalMoves(location_, forbidden);

  if (!legal_moves.empty()) {
    std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
    return legal_moves[dist(rng)];
  }
  return location_;
}

void Animal::StateUpdate(const Grid &grid) {
  if (wait_time_ > 0) 
    wait_time_ -= 1;
    
  energy_ -= GetTraits().energy_rate;
  thirst_ += GetTraits().thirst_rate;
  if (thirst_ <= genes_.thirst_threshold &&
      energy_ >= genes_.energy_threshold) {
    libido_ += genes_.libido_rate;
  }

  if (wait_time_ > 0)
    return;
  state_ = DetermineState(grid);
}

void Animal::MoveStep(const Grid &grid,
                      const std::vector<LandType> &forbidden) {
  if (location_ == target_coord_)
    return;
  Coords next_step = grid.PlotNextMove(location_, target_coord_, forbidden);
  if (next_step != location_) {
    location_ = next_step;
    energy_ -= GetTraits().energy_move_penalty;
  }
}

void Animal::DrinkInteract(const Grid &grid) {
  if (grid.IsNextToWater(location_)) {
    thirst_ = 0.0f;
    state_ = AnimalState::Idle;
  }
}

bool Animal::IsReadyToAct() const {
  return wait_time_ == 0 && state_ != AnimalState::Dead;
}

void Animal::SetLibidoAfterMate() { libido_ = 0.0f; }

void Animal::SetEnergyAfterMate() { energy_ -= 0.4f; }

void Animal::SetState(AnimalState new_state) { state_ = new_state; }

Coords Animal::GetLocation() const { return location_; }

void Animal::SeekTarget(const Grid &grid, std::mt19937 &rng) {
  target_coord_ = DetermineTarget(grid, rng);
}

template <typename T> T *Animal::DetermineMate(const Grid &grid) const {
  std::vector<T *> mates = grid.GetAnimalsAt<T>(location_);
  for (T *ptr : mates) {
    if (ptr->GetState() == AnimalState::Libidinous && ptr != this) {
      return ptr;
    }
  }
  return nullptr;
}
template Rabbit *Animal::DetermineMate<Rabbit>(const Grid &) const;
template Fox *Animal::DetermineMate<Fox>(const Grid &) const;

template <typename T>
void Animal::MateInteract(const Grid &grid,
                          std::vector<std::unique_ptr<Animal>> &newborns,
                          std::mt19937 &rng
                          , std::function<uint64_t()> id_generator) {
  T *mate = DetermineMate<T>(grid);
  if (mate != nullptr) {
    AnimalGenes baby_genes = AnimalGenes::Inherit(genes_, mate->genes_, rng);
    AnimalStatus newborn_status = InitialiseStatusBorn(GetTraits(), baby_genes);
  uint64_t newborn_id = id_generator ? id_generator() : 0;
    newborns.push_back(
    std::make_unique<T>(location_, baby_genes, newborn_status, newborn_id));

    SetLibidoAfterMate();
    SetEnergyAfterMate();
    SetState(AnimalState::Idle);
    mate->SetLibidoAfterMate();
    mate->SetEnergyAfterMate();
    mate->SetState(AnimalState::Idle);
  }
}
template void Animal::MateInteract<Rabbit>(
    const Grid &, std::vector<std::unique_ptr<Animal>> &, std::mt19937 &, std::function<uint64_t()>);
template void Animal::MateInteract<Fox>(const Grid &,
                                        std::vector<std::unique_ptr<Animal>> &,
                                        std::mt19937 &, std::function<uint64_t()>);

AnimalGenes Animal::GetGenes() const { return genes_; }

int Animal::GetSpeciesId() const { return -1; } // TODO: 

AnimalData Animal::Serialize(int species_id) const {
  AnimalData data;
  data.id = id_;
  data.species_id = species_id;
  data.state = static_cast<int>(state_);
  data.x = location_.first;
  data.y = location_.second;
  data.target_x = target_coord_.first;
  data.target_y = target_coord_.second;

  data.energy = energy_;
  data.thirst = thirst_;
  data.libido = libido_;
  data.wait_time = wait_time_;

  data.energy_threshold = genes_.energy_threshold;
  data.thirst_threshold = genes_.thirst_threshold;
  data.libido_threshold = genes_.libido_threshold;
  data.libido_rate = genes_.libido_rate;
  data.action_delay = genes_.action_delay;
  data.restlessness = genes_.restlessness;
  return data;
}

std::unique_ptr<Animal> Animal::Deserialize(const AnimalData& data) {
  Coords loc = {data.x, data.y};

  AnimalGenes genes;
  genes.energy_threshold = data.energy_threshold;
  genes.thirst_threshold = data.thirst_threshold;
  genes.libido_threshold = data.libido_threshold;
  genes.libido_rate = data.libido_rate;
  genes.action_delay = data.action_delay;
  genes.restlessness = data.restlessness;

  AnimalStatus status;
  status.energy = data.energy;
  status.thirst = data.thirst;
  status.libido = data.libido;
  status.wait_time = data.wait_time;

  std::unique_ptr<Animal> new_animal;
  if (data.species_id == 0) {
    new_animal = std::make_unique<Rabbit>(loc, genes, status, data.id);
  } else if (data.species_id == 1) {
    new_animal = std::make_unique<Fox>(loc, genes, status, data.id);
  } else {
    throw std::invalid_argument("Unknown species_id in Animal::Deserialize");
  }

  new_animal->target_coord_ = {data.target_x, data.target_y};
  new_animal->SetState(static_cast<AnimalState>(data.state));
  return new_animal;
}

uint64_t Animal::GetId() const {
    return id_;
}