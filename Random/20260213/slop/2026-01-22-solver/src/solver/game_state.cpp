#include "game_state.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace solver {

std::string Action::toString() const {
    switch (type) {
        case ActionType::FOLD: return "Fold";
        case ActionType::CHECK: return "Check";
        case ActionType::CALL: return "Call " + std::to_string(static_cast<int>(amount)) + "bb";
        case ActionType::BET: 
            return "Bet " + std::to_string(static_cast<int>(potFraction * 100)) + "% (" + 
                   std::to_string(static_cast<int>(amount)) + "bb)";
        case ActionType::RAISE:
            return "Raise " + std::to_string(static_cast<int>(amount)) + "bb";
        case ActionType::ALL_IN:
            return "All-in " + std::to_string(static_cast<int>(amount)) + "bb";
        default: return "Unknown";
    }
}

GameState::GameState() : GameState(BetSizingConfig{}) {}

GameState::GameState(const BetSizingConfig& config) : config_(config) {
    pot_ = config_.initialPot;
    oopStack_ = config_.stackSize - (config_.initialPot / 2);
    ipStack_ = config_.stackSize - (config_.initialPot / 2);
}

void GameState::setStackSize(double bb) {
    config_.stackSize = bb;
    oopStack_ = bb - (config_.initialPot / 2);
    ipStack_ = bb - (config_.initialPot / 2);
}

void GameState::setInitialPot(double bb) {
    config_.initialPot = bb;
    pot_ = bb;
}

void GameState::setOOPRange(const core::Range& range) {
    oopRange_ = range;
}

void GameState::setIPRange(const core::Range& range) {
    ipRange_ = range;
}

void GameState::setOOPHand(const core::Hand& hand) {
    oopHand_ = hand;
}

void GameState::setIPHand(const core::Hand& hand) {
    ipHand_ = hand;
}

void GameState::setBoard(const std::vector<core::Card>& board) {
    board_ = board;
    if (board.size() >= 3) street_ = Street::FLOP;
    if (board.size() >= 4) street_ = Street::TURN;
    if (board.size() >= 5) street_ = Street::RIVER;
}

void GameState::setFlop(const core::Card& c1, const core::Card& c2, const core::Card& c3) {
    board_.clear();
    board_.push_back(c1);
    board_.push_back(c2);
    board_.push_back(c3);
    street_ = Street::FLOP;
    currentPlayer_ = Position::OOP;
    oopInvested_ = 0;
    ipInvested_ = 0;
}

void GameState::setTurn(const core::Card& card) {
    if (board_.size() == 3) {
        board_.push_back(card);
        street_ = Street::TURN;
        currentPlayer_ = Position::OOP;
        oopInvested_ = 0;
        ipInvested_ = 0;
    }
}

void GameState::setRiver(const core::Card& card) {
    if (board_.size() == 4) {
        board_.push_back(card);
        street_ = Street::RIVER;
        currentPlayer_ = Position::OOP;
        oopInvested_ = 0;
        ipInvested_ = 0;
    }
}

double GameState::effectiveStack() const {
    return std::min(oopStack_, ipStack_);
}

double GameState::getToCall() const {
    if (currentPlayer_ == Position::OOP) {
        return ipInvested_ - oopInvested_;
    } else {
        return oopInvested_ - ipInvested_;
    }
}

std::vector<Action> GameState::getAvailableActions() const {
    std::vector<Action> actions;
    
    if (isTerminal()) return actions;
    
    double toCall = getToCall();
    double currentStack = (currentPlayer_ == Position::OOP) ? oopStack_ : ipStack_;
    double opponentInvested = (currentPlayer_ == Position::OOP) ? ipInvested_ : oopInvested_;
    double myInvested = (currentPlayer_ == Position::OOP) ? oopInvested_ : ipInvested_;
    
    // Fold is available if there's something to call
    if (toCall > 0) {
        actions.push_back(Action::fold());
    }
    
    // Check is available if nothing to call
    if (toCall == 0) {
        actions.push_back(Action::check());
    }
    
    // Call is available if there's something to call and we have chips
    if (toCall > 0 && currentStack >= toCall) {
        actions.push_back(Action::call(toCall));
    } else if (toCall > 0 && currentStack > 0) {
        // All-in call
        actions.push_back(Action::allIn(currentStack));
    }
    
    // Get bet sizes based on position and street
    std::vector<double> betSizes;
    if (currentPlayer_ == Position::OOP) {
        switch (street_) {
            case Street::FLOP: betSizes = config_.oopFlopBets; break;
            case Street::TURN: betSizes = config_.oopTurnBets; break;
            case Street::RIVER: betSizes = config_.oopRiverBets; break;
            default: break;
        }
    } else {
        switch (street_) {
            case Street::FLOP: betSizes = config_.ipFlopBets; break;
            case Street::TURN: betSizes = config_.ipTurnBets; break;
            case Street::RIVER: betSizes = config_.ipRiverBets; break;
            default: break;
        }
    }
    
    // If no bet has been made, these are bet sizes
    if (opponentInvested == 0) {
        for (double pct : betSizes) {
            double betAmount = pot_ * (pct / 100.0);
            
            if (betAmount >= currentStack) {
                // All-in
                if (currentStack > 0) {
                    actions.push_back(Action::allIn(currentStack));
                }
                break;  // No point adding larger sizes
            }
            
            // Check all-in threshold
            double potAfterBet = pot_ + betAmount;
            double remainingStack = currentStack - betAmount;
            if (remainingStack <= potAfterBet * (config_.allInThreshold / 100.0)) {
                actions.push_back(Action::allIn(currentStack));
                break;
            }
            
            actions.push_back(Action::bet(betAmount, pct / 100.0));
        }
    }
    // Raises
    else if (toCall > 0) {
        // Standard raise (2.5x or configured)
        double raiseAmount = opponentInvested * config_.raiseMultiplier;
        
        if (raiseAmount <= currentStack) {
            // Check all-in threshold
            double totalBet = myInvested + raiseAmount;
            double potAfterRaise = pot_ + totalBet - myInvested + (totalBet - opponentInvested);
            double remainingStack = currentStack - raiseAmount;
            
            if (remainingStack <= potAfterRaise * (config_.allInThreshold / 100.0)) {
                actions.push_back(Action::allIn(currentStack));
            } else {
                actions.push_back(Action::raise(raiseAmount, config_.raiseMultiplier));
            }
        } else if (currentStack > toCall) {
            // All-in raise
            actions.push_back(Action::allIn(currentStack));
        }
    }
    
    return actions;
}

void GameState::applyAction(const Action& action) {
    history_.push_back(action);
    
    double& myStack = (currentPlayer_ == Position::OOP) ? oopStack_ : ipStack_;
    double& myInvested = (currentPlayer_ == Position::OOP) ? oopInvested_ : ipInvested_;
    double& oppInvested = (currentPlayer_ == Position::OOP) ? ipInvested_ : oopInvested_;
    
    switch (action.type) {
        case ActionType::FOLD:
            folded_ = true;
            foldedPlayer_ = currentPlayer_;
            break;
            
        case ActionType::CHECK:
            // If IP checks after OOP checks, advance street
            if (currentPlayer_ == Position::IP && oopInvested_ == 0 && ipInvested_ == 0) {
                advanceStreet();
                return;
            }
            break;
            
        case ActionType::CALL:
            myStack -= action.amount;
            myInvested += action.amount;
            pot_ += action.amount;
            // After a call, advance street
            advanceStreet();
            return;
            
        case ActionType::BET:
        case ActionType::RAISE:
        case ActionType::ALL_IN:
            myStack -= action.amount;
            myInvested += action.amount;
            pot_ += action.amount;
            break;
    }
    
    // Switch players
    currentPlayer_ = (currentPlayer_ == Position::OOP) ? Position::IP : Position::OOP;
}

void GameState::undo() {
    if (history_.empty()) return;
    
    // This is a simplified undo - for full support, we'd need to store more state
    // For now, this is a placeholder
    history_.pop_back();
}

bool GameState::isTerminal() const {
    // Folded
    if (folded_) return true;
    
    // All-in and called
    if (isAllIn() && oopInvested_ == ipInvested_) return true;
    
    // River action complete (both checked or bet called)
    if (street_ == Street::RIVER && oopInvested_ == ipInvested_ && 
        !history_.empty() && 
        (history_.back().type == ActionType::CALL || 
         (history_.back().type == ActionType::CHECK && history_.size() >= 2))) {
        return true;
    }
    
    return false;
}

bool GameState::isAllIn() const {
    return oopStack_ == 0 || ipStack_ == 0;
}

bool GameState::hasShowdown() const {
    return isTerminal() && !folded_;
}

GameState GameState::afterAction(const Action& action) const {
    GameState next = *this;
    next.applyAction(action);
    return next;
}

void GameState::resetStreet() {
    // Reset investments for current street
    oopInvested_ = 0;
    ipInvested_ = 0;
    currentPlayer_ = Position::OOP;
    
    // Clear actions from current street
    while (!history_.empty()) {
        history_.pop_back();
    }
}

void GameState::advanceStreet() {
    // Add investments to pot (they should already be there from actions)
    oopInvested_ = 0;
    ipInvested_ = 0;
    currentPlayer_ = Position::OOP;
    
    // Don't actually change the street - that's done when cards are dealt
}

std::string GameState::toString() const {
    std::stringstream ss;
    ss << "Street: " << streetToString(street_) << "\n";
    ss << "Pot: " << pot_ << "bb\n";
    ss << "OOP Stack: " << oopStack_ << "bb (invested: " << oopInvested_ << ")\n";
    ss << "IP Stack: " << ipStack_ << "bb (invested: " << ipInvested_ << ")\n";
    ss << "To act: " << positionToString(currentPlayer_) << "\n";
    ss << "Board: ";
    for (const auto& card : board_) {
        ss << card.toString() << " ";
    }
    ss << "\n";
    return ss.str();
}

const char* streetToString(Street street) {
    switch (street) {
        case Street::PREFLOP: return "Preflop";
        case Street::FLOP: return "Flop";
        case Street::TURN: return "Turn";
        case Street::RIVER: return "River";
        default: return "Unknown";
    }
}

const char* positionToString(Position pos) {
    switch (pos) {
        case Position::OOP: return "OOP";
        case Position::IP: return "IP";
        default: return "Unknown";
    }
}

const char* actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::FOLD: return "Fold";
        case ActionType::CHECK: return "Check";
        case ActionType::CALL: return "Call";
        case ActionType::BET: return "Bet";
        case ActionType::RAISE: return "Raise";
        case ActionType::ALL_IN: return "All-in";
        default: return "Unknown";
    }
}

} // namespace solver
