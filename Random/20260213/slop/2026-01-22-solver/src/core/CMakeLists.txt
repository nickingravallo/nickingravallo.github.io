add_library(core STATIC
    card.cpp
    deck.cpp
    hand.cpp
    range.cpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core PUBLIC ompeval)
