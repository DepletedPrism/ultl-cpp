#include <catch2/catch_test_macros.hpp>

#include "ultl/ultl.hpp"

TEST_CASE("test `ultl::leftist_heap::push`", "[leftist_heap]") {
  ultl::leftist_heap<int> heap;

  SECTION("push a single int value `1`.") {
    heap.push(1);

    REQUIRE(heap.size() == 1);
    REQUIRE(heap.top() == 1);
  }

  SECTION("push two integers and get the larger one.") {
    heap.push(514);
    heap.push(114);

    REQUIRE(heap.top() == 514);
  }

  SECTION("push 100,000 elements") {
    for (int i = 0; i < 100'000; ++i) {
      heap.push(i);
    }

    REQUIRE(heap.top() == 100'000 - 1);
    REQUIRE(heap.size() == 100'000);
  }
}
