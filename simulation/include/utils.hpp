#include "types.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

struct AnimalGenes;

namespace utils {
float DotProduct(std::pair<int, int> x, std::pair<int, int> y);
float AngleCalc(std::pair<int, int> x, std::pair<int, int> y);
float magnitude(std::pair<int, int> x);
std::string AnimalStateToString(AnimalState state);

void PrintLandTile(LandType landType);
void PrintHelp();
bool ParseArguments(int argc, char **argv, SimulationConfig &sim_config,
                    GridConfig &grid_config, int rank);

std::string FormatRecord(const SimulationStatisticsOutput &stats);
void AppendToFile(const std::string &record, const std::string &file_path);
void InitialiseOutputFile(const std::string &file_path);

// Test utility helpers
AnimalGenes CreateTestingGenes();
GridConfig CreateTestingGridConfig();
AnimalTraits CreateTestingTraits();
AnimalStatus CreateTestingStatus();

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Second = std::chrono::duration<double, std::ratio<1>>;
    Timer() { reset(); }
    void reset();
    double elapsed() const;

private:
    std::chrono::time_point<Clock> curr;
};
}; // namespace utils
