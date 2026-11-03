file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(TestSimulation ${TEST_SOURCES})
target_link_libraries(TestSimulation PUBLIC SimulationLib Catch2 Catch2::Catch2WithMain)