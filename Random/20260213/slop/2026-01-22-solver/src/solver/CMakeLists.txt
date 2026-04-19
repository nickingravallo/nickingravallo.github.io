add_library(solver STATIC
    game_state.cpp
    game_tree.cpp
    mccfr.cpp
)

target_include_directories(solver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(solver 
    PUBLIC core ompeval
    PRIVATE Threads::Threads
)
