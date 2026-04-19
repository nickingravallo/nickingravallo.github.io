# GTO Poker Solver

A Game Theory Optimal (GTO) poker solver using Monte Carlo Counterfactual Regret Minimization (MCCFR).

## Features

- **OMPEval Hand Evaluator**: Fast poker hand evaluation using lookup tables
- **MCCFR Solver**: Monte Carlo CFR algorithm for computing Nash equilibrium strategies
- **Progressive Solving**: Street-by-street solving with real-time strategy updates
- **Qt GUI**: Modern dark-themed interface with:
  - Interactive card selection for flop, turn, and river
  - Range input with support for standard notation (e.g., "77+, ATs+, KQs")
  - 13x13 strategy grid showing action frequencies by hand type
  - Color-coded actions (green=check/call, red shades=bets, blue=fold)
  - Real-time progress tracking and exploitability display

## Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+
- Qt6 (Widgets, Core, Gui)
- pthread (for threading support)

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . -j$(nproc)

# Run
./gto_solver
```

### Ubuntu/Debian

```bash
sudo apt install build-essential cmake qt6-base-dev
```

### Fedora

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

### macOS

```bash
brew install cmake qt@6
```

## Usage

1. **Set Ranges**: Enter hand ranges for OOP (UTG) and IP (BTN) players using standard notation:
   - `77+` - Pairs 77 and higher
   - `ATs+` - Suited Aces from AT to AK
   - `AJo-AKo` - Offsuit Aces from AJ to AK
   - `AKs@50` - AK suited at 50% frequency

2. **Select Board**: Click cards to select the flop (3 cards required before solving)

3. **Solve**: Click "Solve" to run the MCCFR algorithm

4. **View Strategy**: The strategy grid shows action frequencies for each hand type:
   - Green = Check/Call
   - Orange/Red = Bets (darker = larger sizing)
   - Blue = Fold

5. **Progress**: The solver iterates progressively with exploitability decreasing over time

## Default Ranges

The "Load Default Ranges" button loads standard 6max UTG vs BTN ranges:

- **UTG Open**: 77+, ATs+, KQs, AJo+, KQo (~15% of hands)
- **BTN Call vs UTG**: 66-TT, ATs-AQs, KQs, KJs, QJs, JTs, T9s, 98s, 87s, 76s, AQo (~12% of hands)

## Bet Sizing

### OOP (Out of Position)
- Flop: 25%, 40%, 80%, 120% pot
- Turn: 25%, 40%, 80%, 120% pot
- River: 50%, 80%, 120% pot

### IP (In Position)
- Flop: 50%, 80%, 120% pot
- Turn: 50%, 80%, 120% pot
- River: 80%, 120% pot
- Raise: 2.5x

All-in threshold: 125% pot (if remaining stack < 125% pot, goes all-in)

## Architecture

```
src/
├── ompeval/          # Hand evaluation module
│   ├── hand_evaluator.hpp/cpp
│   └── lookup_tables.hpp/cpp
├── core/             # Core poker types
│   ├── card.hpp/cpp
│   ├── deck.hpp/cpp
│   ├── hand.hpp/cpp
│   └── range.hpp/cpp
├── solver/           # MCCFR solver
│   ├── game_state.hpp/cpp
│   ├── game_tree.hpp/cpp
│   └── mccfr.hpp/cpp
├── gui/              # Qt interface
│   ├── main_window.hpp/cpp
│   ├── card_selector.hpp/cpp
│   ├── range_grid.hpp/cpp
│   ├── action_panel.hpp/cpp
│   ├── strategy_grid.hpp/cpp
│   └── progress_panel.hpp/cpp
└── main.cpp
```

## Technical Details

### OMPEval Algorithm
The hand evaluator uses pre-computed lookup tables for O(1) hand ranking:
- Flush detection via suit masks
- Straight detection via rank bitmasks
- Paired hand evaluation via rank counting

### MCCFR Algorithm
Uses external sampling CFR for efficient convergence:
- Samples opponent actions according to current strategy
- Computes counterfactual values for all actions at traversing player's nodes
- Updates regrets and strategies via regret matching
- Optional CFR+ style discounting for faster convergence

## License

MIT License
