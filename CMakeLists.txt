cmake_minimum_required(VERSION 3.5)
project(csdb)
cmake_policy(SET CMP0049 OLD)

enable_language(C CXX)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CMakeDependentOption)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  )

add_subdirectory(cslog EXCLUDE_FROM_ALL)

find_package (Threads REQUIRED)

set(COMMON_SOURCE_FILES
  cslog/src/logging.cpp
  src/algo/highwayhash.cpp
  src/algo/crc64.cpp
  src/algo/crc32.cpp
  src/structure/hashtable.cpp
  src/main.cpp
  )

macro(create_test name files)
  message(STATUS "Creating test '${name}' of ${files}")

  if (NOT (${name} STREQUAL "unit"))
    # Collecting names for globas "unit_tests" binary
    list(APPEND all_test_files
      "${files}")
  endif()

  set(${name}_TEST_FILES
    test/main.cpp
    ${files}
    )

  set(test_files
    "\${${name}_TEST_FILES}"
    )

  add_executable(
    ${name}_tests
    ${COMMON_SOURCE_FILES}
    ${GTEST_SOURCES}
    ${GMOCK_SOURCES}
    ${test_files}
    )

  target_link_libraries(
    ${name}_tests
    ${LIBCXX_LIBRARY}
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${STATIC_LIBRT}
    )
  add_test(${name} ${name}_tests)
  set_target_properties(
            ${name}_tests
            PROPERTIES
            COMPILE_DEFINITIONS _UNIT_TEST_BUILD)
  set_target_properties(
            ${name}_tests
            PROPERTIES
            COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=pedantic" )
endmacro(create_test)

include(ConfigSafeGuards)

find_package(GTest)
if (DEFINED GTEST_INCLUDE_DIRS)
  include_directories(${GTEST_INCLUDE_DIRS})
endif()

find_package(GMock)
if (DEFINED GMOCK_INCLUDE_DIRS)
  include_directories(${GMOCK_INCLUDE_DIRS})
endif()

if (TRAVIS_BUILD)
  add_definitions(-DTRAVIS_BUILD)
endif()

# Enable thread-safe POSIX implementations of C library
add_definitions(-D_POSIX_C_SOURCE)

set(STANDARD "-std=c++2a")

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas -Wno-sign-compare -Woverloaded-virtual -Wwrite-strings -Wno-unused ${STANDARD} -Wextra -pedantic -Werror -pthread")
set(CMAKE_CXX_FLAGS_PROFILED "${CMAKE_CXX_FLAGS} -pg")

if (STATIC)
  add_definitions(-DSTATIC)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -fPIC -fPIE")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  endif()
endif()

if (SANITIZED)
  set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3 ${STANDARD}")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast ${STANDARD}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -fvisibility=hidden")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unknown-warning-option -Wno-error=braced-scalar-init -stdlib=libc++")
  set(LIBCXX_LIBRARY "-lc++abi")
endif()

if ((COVERALLS OR COVERAGE) AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "will generate coverage data")
  include(CodeCoverage)
  include(Coveralls)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0 -Wall -W -fprofile-arcs -ftest-coverage --coverage --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
  set(CMAKE_SHARED_LINKER_FLAGS="${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
endif()

if (STATIC)
  add_library(csdb STATIC src/main.cpp ${SOURCE_FILES})
  SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  SET (STATIC_LIBRT "-lrt")
  set_target_properties(csdb PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(csdb PROPERTIES LINK_SEARCH_END_STATIC 1)
else()
  add_library(csdb SHARED src/main.cpp ${SOURCE_FILES})
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/cslog/include
  )

target_link_libraries(
  csdb
  ${CMAKE_THREAD_LIBS_INIT}
  ${LIBCXX_LIBRARY}
  )

include(CTest)
enable_testing()

create_test(highwayhash test/highwayhash.cpp)
create_test(crc64 test/crc64.cpp)
create_test(crc32 test/crc32.cpp)
create_test(hashtable test/hashtable.cpp)
create_test(bptree test/bptree.cpp)

create_test(unit "${all_test_files}")

if (COVERAGE)
  setup_target_for_coverage(coverage unit_tests CMakeFiles/unit_tests.dir/src coverage)
endif()

if (COVERALLS)
  setup_target_for_coveralls(coveralls unit_tests src)
endif()
