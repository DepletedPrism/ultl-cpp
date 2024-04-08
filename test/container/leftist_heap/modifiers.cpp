#include <iostream>
#include <random>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "ultl/ultl.hpp"

TEST_CASE("test modifiers of `ultl::leftist_heap`", "[leftist_heap]") {
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

    heap.pop();
    REQUIRE(heap.top() == 114);
  }

  SECTION("push and pop 1,000,000 elements") {
    constexpr int N = 1'000'000;

    for (int i = 0; i < N; ++i)
      heap.push(i);
    REQUIRE(heap.top() == N - 1);
    REQUIRE(heap.size() == N);

    for (int i = N; i > 0; --i) {
      REQUIRE(heap.top() == i - 1);
      heap.pop();
    }
    REQUIRE(heap.empty());
  }
}

TEST_CASE("test `ultl::leftist_heap::emplace`", "[leftist_heap]") {
  ultl::leftist_heap<std::string> heap;

  heap.emplace(10, 'a');
  heap.emplace("aaaaa");

  REQUIRE(heap.size() == 2);
  REQUIRE(heap.top() == std::string(10, 'a'));
}

TEST_CASE("test `ultl::leftist_heap::merge`", "[leftist_heap]") {
  ultl::leftist_heap<int> odd{1, 3, 5, 7, 9}, even{2, 4, 6, 8, 10};

  auto m1 = odd;
  m1.merge(even);
  REQUIRE(m1.size() == 10);
  for (int i = 10; i > 0; --i) {
    REQUIRE(m1.top() == i);
    m1.pop();
  }

  auto m2 = even;
  m2.merge(std::move(odd));
  REQUIRE(m2.size() == 10);
  for (int i = 10; i > 0; --i) {
    REQUIRE(m2.top() == i);
    m2.pop();
  }
}

TEST_CASE("benchmark `ultl::leftist_heap::merge`", "[leftist_heap]") {
  constexpr int N = 10'000;
  ultl::leftist_heap<uint64_t> a, b;
  std::mt19937 rng(Catch::getCurrentContext().getConfig()->rngSeed());

  for (int i = 0; i < N; ++i) {
    a.push(rng());
    b.push(rng());
  }

  SECTION("merge with copying") {
    BENCHMARK("copy and merge") {
      a.merge(b);
    };
  }

  SECTION("merge with moving") {
    BENCHMARK("move and merge") {
      a.merge(std::move(b));
    };
  }
}
