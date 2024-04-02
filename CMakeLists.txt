cmake_minimum_required(VERSION 3.12)

project(ultl-cpp VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standrd to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
aux_source_directory(src SOURCES)

option(ENABLE_TESTING "enable testing" ON)

if (ENABLE_TESTING)
  # Testing
  enable_testing()
  message(STATUS "Testing is enabled.")

  # Use Catch2 as test framework
  add_subdirectory(third_party/Catch2)
  
  add_executable(tests
    test/test.cpp
    ${SOURCES}
  )
  target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
  )
  add_test(NAME tests COMMAND tests)
else ()
  message(STATUS "Testing is disabled.")
endif ()
