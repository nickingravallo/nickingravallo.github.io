#pragma once

#include "core/card.hpp"
#include "core/hand.hpp"
#include "core/range.hpp"
#include <vector>
#include <string>
#include <memory>

namespace solver {

// Game street
enum class Street {
    PREFLOP = 0,
    FLOP,
    TURN,
    RIVER
};

// Player positions
enum class Position {
    OOP = 0,  // Out of Position (acts first postflop)
    IP = 1    // In Position (acts last postflop)
};

// Action types
enum class ActionType {
    FOLD,
    CHECK,
    CALL,
    BET,
    RAISE,
    ALL_IN
};

// Bet sizes as percentage of pot
struct BetSize {
    double percentage;
    std::string label;
    
    BetSize(double pct, const std::string& lbl = "") 
        : percentage(pct), label(lbl.empty() ? std::to_string(static_cast<int>(pct)) + "%" : lbl) {}
};

// Action taken by a player
struct Action {
    ActionType type;
    double amount;      // In big blinds
    double potFraction; // As fraction of pot (for display)
    
    std::string toString() const;
    
    static Action fold() { return {ActionType::FOLD, 0, 0}; }
    static Action check() { return {ActionType::CHECK, 0, 0}; }
    static Action call(double amount) { return {ActionType::CALL, amount, 0}; }
    static Action bet(double amount, double potFrac) { return {ActionType::BET, amount, potFrac}; }
    static Action raise(double amount, double potFrac) { return {ActionType::RAISE, amount, potFrac}; }
    static Action allIn(double amount) { return {ActionType::ALL_IN, amount, 0}; }
};

// Available bet sizing presets
struct BetSizingConfig {
    // OOP bet sizes (as % of pot)
    std::vector<double> oopFlopBets = {25, 40, 80, 120};
    std::vector<double> oopTurnBets = {25, 40, 80, 120};
    std::vector<double> oopRiverBets = {50, 80, 120};
    
    // IP bet/raise sizes (as % of pot)
    std::vector<double> ipFlopBets = {50, 80, 120};
    std::vector<double> ipTurnBets = {50, 80, 120};
    std::vector<double> ipRiverBets = {80, 120};
    
    // Raise multiplier
    double raiseMultiplier = 2.5;
    
    // All-in threshold (% of pot where we just go all-in)
    double allInThreshold = 125;
    
    // Default stack size in BB
    double stackSize = 100;
    
    // Single raised pot opening (3bb open, call from BTN)
    double initialPot = 7.0;  // 3bb + 3bb + 0.5sb + 0.5bb blinds
};

/**
 * Represents the current state of a poker hand.
 */
class GameState {
public:
    GameState();
    GameState(const BetSizingConfig& config);
    
    // Setup
    void setStackSize(double bb);
    void setInitialPot(double bb);
    void setOOPRange(const core::Range& range);
    void setIPRange(const core::Range& range);
    void setOOPHand(const core::Hand& hand);
    void setIPHand(const core::Hand& hand);
    void setBoard(const std::vector<core::Card>& board);
    
    // Board management
    void setFlop(const core::Card& c1, const core::Card& c2, const core::Card& c3);
    void setTurn(const core::Card& card);
    void setRiver(const core::Card& card);
    
    // Accessors
    Street currentStreet() const { return street_; }
    Position currentPlayer() const { return currentPlayer_; }
    double pot() const { return pot_; }
    double oopStack() const { return oopStack_; }
    double ipStack() const { return ipStack_; }
    double oopInvested() const { return oopInvested_; }
    double ipInvested() const { return ipInvested_; }
    const std::vector<core::Card>& board() const { return board_; }
    const core::Range& oopRange() const { return oopRange_; }
    const core::Range& ipRange() const { return ipRange_; }
    const std::vector<Action>& actionHistory() const { return history_; }
    const BetSizingConfig& config() const { return config_; }
    
    // Action management
    std::vector<Action> getAvailableActions() const;
    void applyAction(const Action& action);
    bool canUndo() const { return !history_.empty(); }
    void undo();
    
    // State queries
    bool isTerminal() const;
    bool isAllIn() const;
    bool hasShowdown() const;
    double getToCall() const;  // Amount to call for current player
    Position foldedPlayer() const { return foldedPlayer_; }  // Who folded (if anyone)
    
    // Create a copy with a specific action applied
    GameState afterAction(const Action& action) const;
    
    // Reset to start of current street
    void resetStreet();
    
    // Get string representation
    std::string toString() const;

private:
    BetSizingConfig config_;
    
    Street street_ = Street::FLOP;
    Position currentPlayer_ = Position::OOP;
    
    double pot_ = 0;
    double oopStack_ = 0;
    double ipStack_ = 0;
    double oopInvested_ = 0;  // Invested this street
    double ipInvested_ = 0;
    
    std::vector<core::Card> board_;
    core::Range oopRange_;
    core::Range ipRange_;
    core::Hand oopHand_;
    core::Hand ipHand_;
    
    std::vector<Action> history_;
    
    bool folded_ = false;
    Position foldedPlayer_;
    
    void advanceStreet();
    double effectiveStack() const;
};

// Convert enums to strings
const char* streetToString(Street street);
const char* positionToString(Position pos);
const char* actionTypeToString(ActionType type);

} // namespace solver
