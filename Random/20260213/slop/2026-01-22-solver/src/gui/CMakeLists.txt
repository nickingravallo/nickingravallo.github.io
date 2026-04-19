add_library(gui STATIC
    main_window.cpp
    card_selector.cpp
    range_grid.cpp
    action_panel.cpp
    strategy_grid.cpp
    progress_panel.cpp
)

target_include_directories(gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
target_link_libraries(gui 
    PUBLIC solver core ompeval
    PUBLIC Qt6::Widgets Qt6::Core Qt6::Gui
)
