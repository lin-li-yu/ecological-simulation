#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

using namespace Catch::Matchers;

TEST_CASE("A test", "Example test") {
  int a = 5;
  REQUIRE(a < 6);
}