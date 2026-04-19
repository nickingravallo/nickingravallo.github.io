cmake_minimum_required(VERSION 3.16)
project(GTOSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Optimization flags for release builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# Find threading library
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add subdirectories for each module
add_subdirectory(src/ompeval)
add_subdirectory(src/core)
add_subdirectory(src/solver)
add_subdirectory(src/gui)

# Main executable
add_executable(gto_solver src/main.cpp)

target_link_libraries(gto_solver
    PRIVATE
    ompeval
    core
    solver
    gui
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Threads::Threads
)

# Install rules
install(TARGETS gto_solver DESTINATION bin)
