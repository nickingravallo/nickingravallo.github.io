#include "game_tree.hpp"
#include "ompeval/hand_evaluator.hpp"
#include <algorithm>
#include <sstream>
#include <numeric>

namespace solver {

// InfoSet implementation

InfoSet::InfoSet(Position player, const std::string& key)
    : player_(player), key_(key) {}

void InfoSet::setStrategy(const std::vector<double>& strategy) {
    strategy_ = strategy;
}

void InfoSet::setNumActions(int n) {
    regrets_.resize(n, 0.0);
    strategy_.resize(n, 1.0 / n);  // Uniform initial strategy
    strategySum_.resize(n, 0.0);
}

std::vector<double> InfoSet::getAverageStrategy() const {
    if (strategySum_.empty()) return strategy_;
    
    double sum = std::accumulate(strategySum_.begin(), strategySum_.end(), 0.0);
    
    if (sum <= 0) {
        // Return uniform if no accumulated strategy
        std::vector<double> uniform(strategy_.size(), 1.0 / strategy_.size());
        return uniform;
    }
    
    std::vector<double> avgStrategy(strategySum_.size());
    for (size_t i = 0; i < strategySum_.size(); ++i) {
        avgStrategy[i] = strategySum_[i] / sum;
    }
    
    return avgStrategy;
}

void InfoSet::addRegret(int actionIndex, double regret) {
    if (actionIndex >= 0 && actionIndex < static_cast<int>(regrets_.size())) {
        regrets_[actionIndex] += regret;
    }
}

void InfoSet::updateStrategy() {
    // Regret matching: strategy proportional to positive regrets
    std::vector<double> positiveRegrets(regrets_.size());
    double regretSum = 0;
    
    for (size_t i = 0; i < regrets_.size(); ++i) {
        positiveRegrets[i] = std::max(0.0, regrets_[i]);
        regretSum += positiveRegrets[i];
    }
    
    if (regretSum > 0) {
        for (size_t i = 0; i < strategy_.size(); ++i) {
            strategy_[i] = positiveRegrets[i] / regretSum;
        }
    } else {
        // Uniform strategy if all regrets are non-positive
        double uniform = 1.0 / strategy_.size();
        std::fill(strategy_.begin(), strategy_.end(), uniform);
    }
}

void InfoSet::accumulateStrategy(const std::vector<double>& reachProb) {
    if (reachProb.empty()) {
        for (size_t i = 0; i < strategy_.size(); ++i) {
            strategySum_[i] += strategy_[i];
        }
    } else {
        double reach = (player_ == Position::OOP) ? reachProb[0] : reachProb[1];
        for (size_t i = 0; i < strategy_.size(); ++i) {
            strategySum_[i] += reach * strategy_[i];
        }
        reachProbSum_ += reach;
    }
}

// GameTreeNode implementation

GameTreeNode::GameTreeNode(NodeType type, const GameState& state)
    : type_(type), state_(state) {}

void GameTreeNode::addChild(const Action& action, std::shared_ptr<GameTreeNode> child) {
    actions_.push_back(action);
    children_.push_back(child);
}

std::shared_ptr<GameTreeNode> GameTreeNode::getChild(int actionIndex) const {
    if (actionIndex >= 0 && actionIndex < static_cast<int>(children_.size())) {
        return children_[actionIndex];
    }
    return nullptr;
}

double GameTreeNode::getPayoff(Position player, const core::Hand& oopHand,
                                const core::Hand& ipHand) const {
    // Calculate payoff at terminal node
    const auto& evaluator = ompeval::HandEvaluator::instance();
    
    // Build full hand for each player
    std::vector<int> oopCards = {
        oopHand.card1().value(), oopHand.card2().value()
    };
    std::vector<int> ipCards = {
        ipHand.card1().value(), ipHand.card2().value()
    };
    
    for (const auto& card : state_.board()) {
        oopCards.push_back(card.value());
        ipCards.push_back(card.value());
    }
    
    // Evaluate hands
    auto oopEval = evaluator.evaluate(oopCards);
    auto ipEval = evaluator.evaluate(ipCards);
    
    // Calculate net payoff: winner gets pot minus their investment
    // Pot = oopInvested + ipInvested
    // If OOP wins: they get the pot, net = pot - oopInvested = ipInvested
    // If IP wins: OOP loses their investment, net = -oopInvested
    double oopPayoff = 0;
    
    if (oopEval > ipEval) {
        // OOP wins: gets pot minus their investment
        oopPayoff = state_.pot() - state_.oopInvested();
    } else if (ipEval > oopEval) {
        // IP wins: OOP loses their investment
        oopPayoff = -state_.oopInvested();
    }
    // Tie: split pot, each gets their investment back (net = 0)
    
    return (player == Position::OOP) ? oopPayoff : -oopPayoff;
}

void GameTreeNode::setChanceOutcomes(const std::vector<std::pair<core::Card, double>>& outcomes) {
    chanceOutcomes_ = outcomes;
}

// GameTree implementation

GameTree::GameTree() {}

void GameTree::build(const GameState& initialState,
                     const core::Range& oopRange,
                     const core::Range& ipRange) {
    clearInfoSets();
    nodeCount_ = 0;
    root_ = buildNode(initialState);
}

std::shared_ptr<GameTreeNode> GameTree::buildNode(const GameState& state) {
    ++nodeCount_;
    
    if (state.isTerminal()) {
        return std::make_shared<GameTreeNode>(NodeType::TERMINAL, state);
    }
    
    auto node = std::make_shared<GameTreeNode>(NodeType::PLAYER, state);
    
    // Get available actions
    auto actions = state.getAvailableActions();
    
    // Build child nodes for each action
    for (const auto& action : actions) {
        auto childState = state.afterAction(action);
        auto childNode = buildNode(childState);
        node->addChild(action, childNode);
    }
    
    return node;
}

std::shared_ptr<InfoSet> GameTree::getOrCreateInfoSet(Position player,
                                                       const std::string& key,
                                                       int numActions) {
    auto it = infoSets_.find(key);
    if (it != infoSets_.end()) {
        return it->second;
    }
    
    auto infoSet = std::make_shared<InfoSet>(player, key);
    infoSet->setNumActions(numActions);
    infoSets_[key] = infoSet;
    return infoSet;
}

void GameTree::clearInfoSets() {
    infoSets_.clear();
}

std::string GameTree::generateInfoSetKey(Position player,
                                          const core::Hand& hand,
                                          const GameState& state) const {
    std::stringstream ss;
    
    // Player position
    ss << (player == Position::OOP ? "O" : "I") << ":";
    
    // Hand (canonical form)
    ss << hand.canonicalName() << ":";
    
    // Board
    for (const auto& card : state.board()) {
        ss << card.toString();
    }
    ss << ":";
    
    // Action history
    for (const auto& action : state.actionHistory()) {
        ss << static_cast<int>(action.type);
        if (action.type == ActionType::BET || action.type == ActionType::RAISE) {
            ss << static_cast<int>(action.potFraction * 100);
        }
        ss << ",";
    }
    
    return ss.str();
}

} // namespace solver
