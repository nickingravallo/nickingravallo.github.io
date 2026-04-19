#pragma once

#include "game_state.hpp"
#include "core/hand.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <atomic>

namespace solver {

// Forward declarations
class GameTreeNode;
class InfoSet;

/**
 * Types of game tree nodes
 */
enum class NodeType {
    CHANCE,     // Nature deals cards
    PLAYER,     // Player makes a decision
    TERMINAL    // Hand is over
};

/**
 * Information set - groups of game states that look the same to a player.
 * In poker, this is defined by: player's hand + public board + action history
 */
class InfoSet {
public:
    InfoSet(Position player, const std::string& key);
    
    // Get/set strategy (action probabilities)
    const std::vector<double>& getStrategy() const { return strategy_; }
    void setStrategy(const std::vector<double>& strategy);
    
    // Get average strategy (for final result)
    std::vector<double> getAverageStrategy() const;
    
    // Regret matching: update regrets and compute new strategy
    void addRegret(int actionIndex, double regret);
    void updateStrategy();
    
    // For strategy accumulation
    void accumulateStrategy(const std::vector<double>& reachProb);
    
    // Info set key
    const std::string& key() const { return key_; }
    Position player() const { return player_; }
    
    // Number of actions available
    int numActions() const { return static_cast<int>(strategy_.size()); }
    void setNumActions(int n);
    
private:
    Position player_;
    std::string key_;
    
    std::vector<double> regrets_;           // Cumulative regrets
    std::vector<double> strategy_;          // Current strategy
    std::vector<double> strategySum_;       // Sum of strategies for averaging
    double reachProbSum_ = 0;               // Sum of reach probabilities
};

/**
 * A node in the game tree.
 */
class GameTreeNode {
public:
    GameTreeNode(NodeType type, const GameState& state);
    
    NodeType type() const { return type_; }
    const GameState& state() const { return state_; }
    GameState& state() { return state_; }
    
    // For player nodes
    void setInfoSet(std::shared_ptr<InfoSet> infoSet) { infoSet_ = infoSet; }
    std::shared_ptr<InfoSet> getInfoSet() const { return infoSet_; }
    
    // Child nodes (indexed by action)
    void addChild(const Action& action, std::shared_ptr<GameTreeNode> child);
    std::shared_ptr<GameTreeNode> getChild(int actionIndex) const;
    const std::vector<Action>& getActions() const { return actions_; }
    int numChildren() const { return static_cast<int>(children_.size()); }
    
    // For terminal nodes
    double getPayoff(Position player, const core::Hand& oopHand, 
                     const core::Hand& ipHand) const;
    
    // For chance nodes
    void setChanceOutcomes(const std::vector<std::pair<core::Card, double>>& outcomes);
    const std::vector<std::pair<core::Card, double>>& getChanceOutcomes() const { return chanceOutcomes_; }

private:
    NodeType type_;
    GameState state_;
    std::shared_ptr<InfoSet> infoSet_;
    
    std::vector<Action> actions_;
    std::vector<std::shared_ptr<GameTreeNode>> children_;
    
    // For chance nodes (card, probability)
    std::vector<std::pair<core::Card, double>> chanceOutcomes_;
};

/**
 * Manages the game tree and info sets for MCCFR.
 */
class GameTree {
public:
    GameTree();
    
    // Build tree from initial state
    void build(const GameState& initialState, 
               const core::Range& oopRange,
               const core::Range& ipRange);
    
    // Get root node
    std::shared_ptr<GameTreeNode> getRoot() const { return root_; }
    
    // Get info set by key (creates if doesn't exist)
    std::shared_ptr<InfoSet> getOrCreateInfoSet(Position player, 
                                                 const std::string& key,
                                                 int numActions);
    
    // Get all info sets
    const std::unordered_map<std::string, std::shared_ptr<InfoSet>>& getInfoSets() const {
        return infoSets_;
    }
    
    // Clear all info sets
    void clearInfoSets();
    
    // Statistics
    size_t numInfoSets() const { return infoSets_.size(); }
    size_t numNodes() const { return nodeCount_; }

private:
    std::shared_ptr<GameTreeNode> root_;
    std::unordered_map<std::string, std::shared_ptr<InfoSet>> infoSets_;
    std::atomic<size_t> nodeCount_{0};
    
    // Build tree recursively
    std::shared_ptr<GameTreeNode> buildNode(const GameState& state);
    
    // Generate info set key
    std::string generateInfoSetKey(Position player, 
                                   const core::Hand& hand,
                                   const GameState& state) const;
};

} // namespace solver
