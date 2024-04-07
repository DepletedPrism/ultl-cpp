#include <catch2/catch_test_macros.hpp>

#include "ultl/ultl.hpp"

TEST_CASE("test `ultl::leftist_heap::push`", "[leftist_heap]") {
  ultl::leftist_heap<int> heap;

  SECTION("push a single int value `1`.") {
    heap.push(1);

    REQUIRE(heap.size() == 1);
  }

  // SECTION("push two integers and get the larger one.") {
  //   heap.push(114);
  //   heap.push(514);

  //   REQUIRE(heap.top() == 514);
  // }
}
