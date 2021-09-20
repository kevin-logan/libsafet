cmake_minimum_required(VERSION 3.12)

project(libsafet_examples)
include_directories("${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(
    CPP_SOURCES
    "examples/examples.cpp"
)

if (WIN32)
    add_executable(libsafet_examples WIN32 ${CPP_SOURCES})
else()
    add_executable(libsafet_examples ${CPP_SOURCES})
endif()

set(
    DEPENDENCIES
    ""
)

if(UNIX)
    set(DEPENDENCIES ${DEPENDENCIES} "pthread")
endif(UNIX)

target_link_libraries(libsafet_examples ${DEPENDENCIES})

project(libsafet_tests)

include_directories("${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(
    CPP_SOURCES
    "tests/main.cpp"
    "tests/condition_variable.cpp"
    "tests/cow.cpp"
    "tests/critical_section.cpp"
    "tests/finally.cpp"
    "tests/memory.cpp"
    "tests/optional.cpp"
    "tests/variant.cpp"
)

find_package(Catch2 REQUIRED)

if (WIN32)
    add_executable(libsafet_tests WIN32 ${CPP_SOURCES})
else()
    add_executable(libsafet_tests ${CPP_SOURCES})
endif()

set(
    DEPENDENCIES
    "Catch2::Catch2"
)

if(UNIX)
    set(DEPENDENCIES ${DEPENDENCIES} "pthread")
endif(UNIX)

target_link_libraries(libsafet_tests PRIVATE ${DEPENDENCIES})
