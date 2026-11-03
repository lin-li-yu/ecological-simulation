#include "distributed_simulation.hpp"
#include <iostream>
#include <algorithm>
#include "rabbit.hpp"

DistributedSimulation::DistributedSimulation(Grid &grid, SimulationConfig config, std::mt19937 rng, int rank, size_t buffer)
    : Simulation(grid, config, rng), rank_(rank), buffer_size_(buffer) {    
    // Max sense radius
    Coords grid_size = grid.GetSize();

    int mid_x = grid_size.first / 2;
    int mid_y = grid_size.second / 2;

    if (rank_ == 0) { // Top-left
        start_x_ = 0;       end_x_ = mid_x;
        start_y_ = 0;       end_y_ = mid_y;
    } else if (rank_ == 1) { // Top-right
        start_x_ = mid_x;   end_x_ = grid_size.first;
        start_y_ = 0;       end_y_ = mid_y;
    } else if (rank_ == 2) { // Bottom-left
        start_x_ = 0;       end_x_ = mid_x;
        start_y_ = mid_y;   end_y_ = grid_size.second;
    } else if (rank_ == 3) { // Bottom-right
        start_x_ = mid_x;   end_x_ = grid_size.first;
        start_y_ = mid_y;   end_y_ = grid_size.second;
    }

    FilterInitialAnimals();

    // Reseed RNG
    std::random_device rd;
    rng_.seed(rd() + rank_);
}

bool DistributedSimulation::IsInBuffer(Coords loc) const {
    int buffer_start_x = start_x_ - buffer_size_;
    int buffer_end_x   = end_x_   + buffer_size_;
    int buffer_start_y = start_y_ - buffer_size_;
    int buffer_end_y   = end_y_   + buffer_size_;

    return (loc.first >= buffer_start_x && loc.first < buffer_end_x &&
            loc.second >= buffer_start_y && loc.second < buffer_end_y);
}

bool DistributedSimulation::IsInLocal(Coords loc) const {
    return (loc.first >= start_x_ && loc.first < end_x_ && 
            loc.second >= start_y_ && loc.second < end_y_);
}

bool DistributedSimulation::IsInTargetBuffer(Coords loc, int target_rank) const {
    Coords grid_size = grid_.GetSize();
    int mid_x = grid_size.first / 2;
    int mid_y = grid_size.second / 2;

    int t_start_x = (target_rank == 1 || target_rank == 3) ? mid_x : 0;
    int t_end_x   = (target_rank == 1 || target_rank == 3) ? grid_size.first : mid_x;
    int t_start_y = (target_rank == 2 || target_rank == 3) ? mid_y : 0;
    int t_end_y   = (target_rank == 2 || target_rank == 3) ? grid_size.second : mid_y;

    // Apply the buffer expansion
    int b_start_x = t_start_x - buffer_size_;
    int b_end_x   = t_end_x   + buffer_size_;
    int b_start_y = t_start_y - buffer_size_;
    int b_end_y   = t_end_y   + buffer_size_;

    return (loc.first >= b_start_x && loc.first < b_end_x &&
            loc.second >= b_start_y && loc.second < b_end_y);
}

void DistributedSimulation::FilterInitialAnimals() {
    std::vector<std::unique_ptr<Animal>> owned_animals;
    std::vector<std::unique_ptr<Animal>> ghost_animals;

    for (auto& animal : animals_) { 
        Coords loc = animal->GetLocation();

        // Keep only the ones in local and buffer zones
        if (IsInLocal(loc)) {
            owned_animals.push_back(std::move(animal));
        } 
        else if (IsInBuffer(loc)) {
            ghost_animals.push_back(std::move(animal));
        }
    }

    animals_ = std::move(owned_animals);
    buffer_animals_ = std::move(ghost_animals);
}

void DistributedSimulation::run() {
    while (current_time_ <= config_.total_time) {
        Tick();
        
        if (current_time_ % 10 == 0) {
            UpdateSimulationState();
            RecordStatistics();
        }
        current_time_++;
    }
}

void DistributedSimulation::Tick() {
    // Grass regrowth
    grid_.ProcessGrassRegrowth(current_time_);

    // State Update
    for (auto &animal : animals_) animal->StateUpdate(grid_);

    // Sync animal states
    SyncAnimalsInBuffer();

    // Update cache
    std::vector<Animal*> all_animals;
    for (auto& a : animals_) all_animals.push_back(a.get());
    for (auto& a : buffer_animals_) all_animals.push_back(a.get());
    grid_.UpdateAnimalsCacheRaw(all_animals);

    // Seek
    for (auto &animal : animals_)
      if (animal->IsReadyToAct())
        animal->SeekTarget(grid_, rng_);

    // Move
    for (size_t i = 0; i < animals_.size(); i++) {
      if (animals_[i]->IsReadyToAct()) {
          animals_[i]->MoveStep(grid_, animals_[i]->GetForbiddenTiles());
      }
    }

    // Transfer animals that move out of the local zone to the new owner
    MigrateAnimals();

    // Update only local animals because buffer animals won't interact in this rank
    buffer_animals_.clear(); 
    grid_.UpdateAnimalsCache(animals_);

    // Interact
    std::vector<std::unique_ptr<Animal>> newborns;
    for (auto &animal : animals_) {
        if (animal->IsReadyToAct()) {
            animal->Interact(grid_, newborns, current_time_, rng_, [this]() { return GenerateAnimalId(); });
        }
    }
    for (auto &baby : newborns) animals_.push_back(std::move(baby));

    CleanupCorpses();

    grid_.UpdateAnimalsCache(animals_);

    SyncConsumedGrass();
}

void DistributedSimulation::SyncConsumedGrass() {
    std::vector<Coords> local_consumed = grid_.ExtractRecentlyConsumedGrass();
    std::vector<CoordData> local_consumed_data;
    for (const auto& coord : local_consumed) {
        local_consumed_data.push_back(CoordData{coord.first, coord.second});
    }

    // Gather counts
    int local_count = local_consumed_data.size();
    std::vector<int> all_counts(4);
    MPI_Allgather(&local_count, 1, MPI_INT, all_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

    // Calculate offsets
    std::vector<int> displacements(4, 0);
    int total_consumed = all_counts[0];
    for (int i = 1; i < 4; i++) {
        displacements[i] = displacements[i - 1] + all_counts[i - 1];
        total_consumed += all_counts[i];
    }

    // Gather the actual coordinates
    std::vector<CoordData> global_flat(total_consumed);
    MPI_Allgatherv(local_consumed_data.data(), local_count, MPI_2INT,
                   global_flat.data(), all_counts.data(), displacements.data(),
                   MPI_2INT, MPI_COMM_WORLD);

    // Update the ghost grass
    std::vector<Coords> global_consumed;
    for (auto& flat : global_flat) {
        global_consumed.push_back({flat.x, flat.y});
    }
    grid_.SyncGhostGrass(global_consumed, current_time_);
}

uint64_t DistributedSimulation::GenerateAnimalId() {
    // ID formation: [rank (32 bits)][local counter (32 bits)]
    uint64_t new_id = (static_cast<uint64_t>(rank_) << 32) | animal_counter_;
    animal_counter_++;
    return new_id;
}

void DistributedSimulation::SyncAnimalsInBuffer() {
    buffer_animals_.clear();

    // Prepare send buffers for each target rank
    std::vector<AnimalData> send_buffers[4];

    // Pack the buffers
    // Loop through animals instead of using grid_.animals_cache_ due to space locality
    for (const auto& animal : animals_) {
        Coords loc = animal->GetLocation();
        int species_id = animal->GetSpeciesId();

        for (int r = 0; r < 4; r++) {
            if (r == rank_) continue;
            if (IsInTargetBuffer(loc, r)) {
                send_buffers[r].push_back(animal->Serialize(species_id));
            }
        }
    }

    // Exchange incoming counts
    int send_counts[4] = {
        (int)send_buffers[0].size(), (int)send_buffers[1].size(), 
        (int)send_buffers[2].size(), (int)send_buffers[3].size()
    };
    int recv_counts[4] = {0};
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    std::vector<AnimalData> recv_buffers[4];
    std::vector<MPI_Request> requests;

    // Post all non-blocking receives first to avoid deadlocks
    for (int r = 0; r < 4; r++) {
        if (r == rank_ || recv_counts[r] == 0) continue;

        recv_buffers[r].resize(recv_counts[r]);
        
        MPI_Request req;
        MPI_Irecv(recv_buffers[r].data(), 
                  recv_counts[r] * sizeof(AnimalData),
                  MPI_BYTE,
                  r, 0, MPI_COMM_WORLD, &req);
        requests.push_back(req);
    }

    // Post all non-blocking sends
    for (int r = 0; r < 4; r++) {
        if (r == rank_ || send_counts[r] == 0) continue;

        MPI_Request req;
        MPI_Isend(send_buffers[r].data(), 
                  send_counts[r] * sizeof(AnimalData), 
                  MPI_BYTE, 
                  r, 0, MPI_COMM_WORLD, &req);
        requests.push_back(req);
    }

    // Deserialize after communication completes
    if (!requests.empty()) {
        std::vector<MPI_Status> statuses(requests.size());
        MPI_Waitall(requests.size(), requests.data(), statuses.data());
    }
    for (int r = 0; r < 4; r++) {
        for (const AnimalData& data : recv_buffers[r]) {
            buffer_animals_.push_back(Animal::Deserialize(data));
        }
    }
}

bool DistributedSimulation::IsOwnerOfAnimal(Animal* animal) const {
    return (animal->GetId() >> 32) == static_cast<uint64_t>(rank_);
}

int DistributedSimulation::GetTargetRank(Coords loc) const {
    Coords grid_size = grid_.GetSize();
    int mid_x = grid_size.first / 2;
    int mid_y = grid_size.second / 2;

    bool is_right_half = (loc.first >= mid_x);
    bool is_bottom_half = (loc.second >= mid_y);

    if (!is_right_half && !is_bottom_half) return 0;
    if (is_right_half  && !is_bottom_half) return 1;
    if (!is_right_half && is_bottom_half)  return 2;
    return 3;
}

void DistributedSimulation::MigrateAnimals() {
    std::vector<AnimalData> send_buffers[4];
    std::vector<std::unique_ptr<Animal>> staying_animals;

    // Sort staying vs migrating animals
    for (auto& animal : animals_) {
        Coords loc = animal->GetLocation();
        
        if (IsInLocal(loc)) {
            staying_animals.push_back(std::move(animal));
        } else {
            int target_rank = GetTargetRank(loc);
            send_buffers[target_rank].push_back(animal->Serialize(animal->GetSpeciesId()));
        }
    }

    animals_ = std::move(staying_animals);

    // Exchange migrating counts
    int send_counts[4] = {
        (int)send_buffers[0].size(), (int)send_buffers[1].size(), 
        (int)send_buffers[2].size(), (int)send_buffers[3].size()
    };
    int recv_counts[4] = {0};

    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    std::vector<AnimalData> recv_buffers[4];
    std::vector<MPI_Request> requests;

    // Post all non-blocking receives first to avoid deadlocks
    for (int r = 0; r < 4; r++) {
        if (r == rank_ || recv_counts[r] == 0) continue;

        recv_buffers[r].resize(recv_counts[r]);
        
        MPI_Request req;
        MPI_Irecv(recv_buffers[r].data(), 
                  recv_counts[r] * sizeof(AnimalData), 
                  MPI_BYTE, 
                  r, 1, MPI_COMM_WORLD, &req); // Tag is 1 to differentiate from Ghost sync
        requests.push_back(req);
    }

    for (int r = 0; r < 4; r++) {
        if (r == rank_ || send_counts[r] == 0) continue;

        MPI_Request req;
        MPI_Isend(send_buffers[r].data(), 
                  send_counts[r] * sizeof(AnimalData), 
                  MPI_BYTE, 
                  r, 1, MPI_COMM_WORLD, &req);
        requests.push_back(req);
    }

    if (!requests.empty()) {
        std::vector<MPI_Status> statuses(requests.size());
        MPI_Waitall(requests.size(), requests.data(), statuses.data());
    }

    for (int r = 0; r < 4; r++) {
        for (const AnimalData& data : recv_buffers[r]) {
            animals_.push_back(Animal::Deserialize(data));
        }
    }
}

void DistributedSimulation::UpdateSimulationState() {
    std::pair<int, int> local_counts = CountAnimals();
    
    int local_vals[2] = {local_counts.first, local_counts.second};
    int global_vals[2] = {0, 0};

    MPI_Reduce(local_vals, global_vals, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank_ == 0) {
        total_rabbits_ = global_vals[0];
        total_foxes_   = global_vals[1];
        total_grass_   = grid_.GetTotalAvailableGrass(); 
    }
}

void DistributedSimulation::RecordStatistics() const {
    // Prepare sum of local genes
    std::vector<float> local_sums = CalculateLocalGeneSums();
    std::vector<float> global_sums(24, 0.0f);

    // Sum up all 24 floats using MPI_FLOAT
    MPI_Reduce(local_sums.data(), global_sums.data(), 24, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank_ == 0) {
        WriteStatisticsToFile(global_sums); 
    }
}