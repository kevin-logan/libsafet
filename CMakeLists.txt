cmake_minimum_required(VERSION 3.12)

project(libsafet_examples)
include_directories("${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
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
