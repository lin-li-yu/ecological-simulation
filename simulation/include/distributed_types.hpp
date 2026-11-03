#pragma once

#include <cstdint>

struct CoordData {
    int x;
    int y;
};

struct AnimalData {
    uint64_t id;
    int species_id;
    int state;
    int x;
    int y;
    int target_x;
    int target_y;
    
    // Status
    float energy;
    float thirst;
    float libido;
    int wait_time;

    // Genes
    float energy_threshold;
    float thirst_threshold;
    float libido_threshold;
    float libido_rate;
    float action_delay;
    float restlessness;
};