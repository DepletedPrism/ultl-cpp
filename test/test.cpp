#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "ultl/ultl.hpp"

TEST_CASE("A pluses B", "[a_plus_b]") {
  REQUIRE(ultl::a_plus_b(1, 1) == 2);
  REQUIRE(ultl::a_plus_b(114, 514) == 628);
}