cmake_minimum_required(VERSION 3.12)

project(ultl-cpp VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standrd to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
aux_source_directory(src SOURCES)

# Add additional warings
add_compile_options(-Wall -Wextra -Wshadow -Wconversion -O2)

option(ENABLE_TESTING "enable testing" ON)

if (ENABLE_TESTING)
  # Testing
  enable_testing()
  message(STATUS "Testing is enabled.")

  # Use Catch2 as test framework
  add_subdirectory(third_party/Catch2)

  file(GLOB_RECURSE TESTS "test/*.cpp")

  # Add fsanitize
  add_compile_options(-fsanitize=address -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)

  add_executable(tests
    ${TESTS}
    ${SOURCES}
  )
  target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
  )
  add_test(NAME tests COMMAND tests)
else ()
  message(STATUS "Testing is disabled.")
endif ()
