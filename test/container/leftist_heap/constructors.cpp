#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ultl/ultl.hpp"

TEST_CASE("test constructors of `ultl::leftist_heap`", "[leftist_heap]") {

  SECTION("with `std::initializer_list`") {
    ultl::leftist_heap<int, std::greater<int>> heap{1, 1, 4, 5, 14};
    for (auto value : {1, 1, 4, 5, 14}) {
      REQUIRE(heap.top() == value);
      heap.pop();
    }
  }

  SECTION("copy constructor") {
    ultl::leftist_heap<int, std::greater<int>> a{19, 19, 8, 10};
    auto b = a;
    for (auto value : {8, 10, 19, 19}) {
      REQUIRE(b.top() == value);
      b.pop();
    }
  };

  SECTION("with `std::move`") {
    ultl::leftist_heap<int> a, b;
    a.push(114);
    a.push(514);
    b = std::move(a);
    REQUIRE(b.top() == 514);

    ultl::leftist_heap<int> c(std::move(b));
    REQUIRE(c.top() == 514);
  }
}
