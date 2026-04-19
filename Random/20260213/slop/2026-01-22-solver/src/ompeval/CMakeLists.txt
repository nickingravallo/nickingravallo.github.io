add_library(ompeval STATIC
    hand_evaluator.cpp
    lookup_tables.cpp
)

target_include_directories(ompeval PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(ompeval PRIVATE Threads::Threads)
