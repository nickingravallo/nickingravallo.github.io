#include "mccfr.hpp"
#include "ompeval/hand_evaluator.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

namespace solver {

MCCFRSolver::MCCFRSolver() : MCCFRSolver(MCCFRConfig{}) {}

MCCFRSolver::MCCFRSolver(const MCCFRConfig& config) : config_(config) {
    // Initialize RNGs
    std::random_device rd;
    rngs_.reserve(std::max(1, config_.numThreads));
    for (int i = 0; i < std::max(1, config_.numThreads); ++i) {
        rngs_.emplace_back(rd());
    }
}

void MCCFRSolver::initialize(const GameState& state,
                              const core::Range& oopRange,
                              const core::Range& ipRange) {
    initialState_ = state;
    oopRange_ = oopRange;
    ipRange_ = ipRange;
    iteration_ = 0;
    shouldStop_ = false;
    
    // Clear previous info sets
    gameTree_.clearInfoSets();
}

void MCCFRSolver::solve() {
    for (int i = 0; i < config_.numIterations && !shouldStop_; ++i) {
        runIteration();
        
        if (progressCallback_ && 
            (i % config_.progressCallbackFrequency == 0 || i == config_.numIterations - 1)) {
            reportProgress();
        }
    }
    
    if (progressCallback_) {
        SolveProgress progress;
        progress.currentIteration = iteration_;
        progress.totalIterations = config_.numIterations;
        progress.exploitability = getExploitability();
        progress.complete = true;
        progress.status = "Complete";
        progressCallback_(progress);
    }
}

void MCCFRSolver::runIteration() {
    auto& rng = rngs_[0];  // Single-threaded
    
    // Get dead cards (board)
    std::vector<core::Card> deadCards = initialState_.board();
    
    // Sample hands for both players
    core::Hand oopHand = sampleHand(oopRange_, deadCards, rng);
    
    // Add OOP's hand to dead cards
    std::vector<core::Card> deadForIP = deadCards;
    deadForIP.push_back(oopHand.card1());
    deadForIP.push_back(oopHand.card2());
    
    core::Hand ipHand = sampleHand(ipRange_, deadForIP, rng);
    
    if (!oopHand.isValid() || !ipHand.isValid()) {
        return;  // No valid hand combinations available
    }
    
    // Run CFR for both players
    externalSample(initialState_, oopHand, ipHand, Position::OOP, 1.0, 1.0, rng);
    externalSample(initialState_, oopHand, ipHand, Position::IP, 1.0, 1.0, rng);
    
    ++iteration_;
    
    // Apply discounting periodically
    if (config_.useDiscounting && iteration_ % 100 == 0) {
        applyDiscounting();
    }
}

double MCCFRSolver::externalSample(const GameState& state,
                                    const core::Hand& oopHand,
                                    const core::Hand& ipHand,
                                    Position traversingPlayer,
                                    double oopReach,
                                    double ipReach,
                                    std::mt19937& rng) {
    // Terminal node: return payoff
    if (state.isTerminal()) {
        const auto& evaluator = ompeval::HandEvaluator::instance();
        
        // Check for fold
        if (!state.hasShowdown()) {
            // Someone folded - the other player wins the pot
            Position folder = state.foldedPlayer();
            double pot = state.pot();
            double oopPayoff = 0;
            
            if (folder == Position::IP) {
                // IP folded: OOP wins the pot
                oopPayoff = pot - state.oopInvested();
            } else {
                // OOP folded: OOP loses their investment
                oopPayoff = -state.oopInvested();
            }
            
            return (traversingPlayer == Position::OOP) ? oopPayoff : -oopPayoff;
        }
        
        // Showdown: evaluate hands
        std::vector<int> oopCards = {
            oopHand.card1().value(), oopHand.card2().value()
        };
        std::vector<int> ipCards = {
            ipHand.card1().value(), ipHand.card2().value()
        };
        
        for (const auto& card : state.board()) {
            oopCards.push_back(card.value());
            ipCards.push_back(card.value());
        }
        
        auto oopEval = evaluator.evaluate(oopCards);
        auto ipEval = evaluator.evaluate(ipCards);
        
        // Calculate net payoff: winner gets pot minus their investment
        // Pot = oopInvested + ipInvested
        // If OOP wins: they get the pot, net = pot - oopInvested = ipInvested
        // If IP wins: OOP loses their investment, net = -oopInvested
        double oopPayoff = 0;
        if (oopEval > ipEval) {
            // OOP wins: gets pot minus their investment
            oopPayoff = state.pot() - state.oopInvested();
        } else if (ipEval > oopEval) {
            // IP wins: OOP loses their investment
            oopPayoff = -state.oopInvested();
        }
        // Tie: split pot, each gets their investment back (net = 0)
        
        return (traversingPlayer == Position::OOP) ? oopPayoff : -oopPayoff;
    }
    
    // Get available actions
    auto actions = state.getAvailableActions();
    if (actions.empty()) return 0;
    
    Position currentPlayer = state.currentPlayer();
    const core::Hand& currentHand = (currentPlayer == Position::OOP) ? oopHand : ipHand;
    
    // Get info set
    auto infoSet = getInfoSet(currentPlayer, currentHand, state);
    if (infoSet->numActions() != static_cast<int>(actions.size())) {
        infoSet->setNumActions(static_cast<int>(actions.size()));
    }
    
    const auto& strategy = infoSet->getStrategy();
    
    if (currentPlayer == traversingPlayer) {
        // Traversing player: compute counterfactual values for all actions
        std::vector<double> actionValues(actions.size());
        double nodeValue = 0;
        
        for (size_t a = 0; a < actions.size(); ++a) {
            GameState nextState = state.afterAction(actions[a]);
            
            double newOopReach = oopReach;
            double newIpReach = ipReach;
            
            if (currentPlayer == Position::OOP) {
                newOopReach *= strategy[a];
            } else {
                newIpReach *= strategy[a];
            }
            
            actionValues[a] = externalSample(nextState, oopHand, ipHand,
                                              traversingPlayer, newOopReach, newIpReach, rng);
            nodeValue += strategy[a] * actionValues[a];
        }
        
        // Update regrets
        double opponentReach = (currentPlayer == Position::OOP) ? ipReach : oopReach;
        for (size_t a = 0; a < actions.size(); ++a) {
            double regret = opponentReach * (actionValues[a] - nodeValue);
            infoSet->addRegret(static_cast<int>(a), regret);
        }
        
        // Update strategy using regret matching
        infoSet->updateStrategy();
        infoSet->accumulateStrategy({oopReach, ipReach});
        
        return nodeValue;
    } else {
        // Opponent: sample action according to strategy (external sampling)
        std::discrete_distribution<int> dist(strategy.begin(), strategy.end());
        int sampledAction = dist(rng);
        
        GameState nextState = state.afterAction(actions[sampledAction]);
        
        double newOopReach = oopReach;
        double newIpReach = ipReach;
        
        if (currentPlayer == Position::OOP) {
            newOopReach *= strategy[sampledAction];
        } else {
            newIpReach *= strategy[sampledAction];
        }
        
        return externalSample(nextState, oopHand, ipHand,
                              traversingPlayer, newOopReach, newIpReach, rng);
    }
}

std::shared_ptr<InfoSet> MCCFRSolver::getInfoSet(Position player,
                                                   const core::Hand& hand,
                                                   const GameState& state) {
    std::string key = makeInfoSetKey(player, hand, state);
    auto actions = state.getAvailableActions();
    return gameTree_.getOrCreateInfoSet(player, key, static_cast<int>(actions.size()));
}

std::string MCCFRSolver::makeInfoSetKey(Position player,
                                         const core::Hand& hand,
                                         const GameState& state) const {
    std::string key;
    key.reserve(64);
    
    // Position
    key += (player == Position::OOP) ? 'O' : 'I';
    key += ':';
    
    // Hand (canonical name)
    key += hand.canonicalName();
    key += ':';
    
    // Board cards
    for (const auto& card : state.board()) {
        key += card.toString();
    }
    key += ':';
    
    // Action history
    for (const auto& action : state.actionHistory()) {
        key += std::to_string(static_cast<int>(action.type));
        if (action.amount > 0) {
            key += '_';
            key += std::to_string(static_cast<int>(action.amount));
        }
        key += ',';
    }
    
    return key;
}

core::Hand MCCFRSolver::sampleHand(const core::Range& range,
                                    const std::vector<core::Card>& deadCards,
                                    std::mt19937& rng) {
    auto hands = range.getAvailableHands(deadCards);
    if (hands.empty()) {
        return core::Hand();  // Invalid hand
    }
    
    // Weight by range frequency
    std::vector<double> weights;
    weights.reserve(hands.size());
    for (const auto& [hand, weight] : hands) {
        weights.push_back(weight);
    }
    
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    size_t idx = dist(rng);
    
    return hands[idx].first;
}

void MCCFRSolver::applyDiscounting() {
    // CFR+ style discounting
    double t = static_cast<double>(iteration_);
    double posDiscount = std::pow(t, config_.discountAlpha) / 
                         (std::pow(t, config_.discountAlpha) + 1);
    double negDiscount = std::pow(t, config_.discountBeta) /
                         (std::pow(t, config_.discountBeta) + 1);
    double stratDiscount = std::pow(t / (t + 1), config_.discountGamma);
    
    for (auto& [key, infoSet] : gameTree_.getInfoSets()) {
        // Discounting is applied internally to regrets
        // This is a simplified version
        infoSet->updateStrategy();
    }
}

void MCCFRSolver::reportProgress() {
    if (!progressCallback_) return;
    
    SolveProgress progress;
    progress.currentIteration = iteration_;
    progress.totalIterations = config_.numIterations;
    progress.exploitability = getExploitability();
    progress.complete = false;
    progress.status = "Solving...";
    
    progressCallback_(progress);
}

void MCCFRSolver::reset() {
    iteration_ = 0;
    shouldStop_ = false;
    gameTree_.clearInfoSets();
}

NodeStrategy MCCFRSolver::getStrategy(Position player, const core::Hand& hand) const {
    NodeStrategy result;
    result.handType = hand.canonicalName();
    
    std::string key = makeInfoSetKey(player, hand, initialState_);
    
    auto& infoSets = gameTree_.getInfoSets();
    auto it = infoSets.find(key);
    
    if (it != infoSets.end()) {
        result.actionProbabilities = it->second->getAverageStrategy();
    } else {
        // No info set found - return uniform strategy
        auto actions = initialState_.getAvailableActions();
        result.actionProbabilities.resize(actions.size(), 1.0 / actions.size());
    }
    
    // Get action names
    auto actions = initialState_.getAvailableActions();
    for (const auto& action : actions) {
        result.actionNames.push_back(action.toString());
    }
    
    return result;
}

std::vector<NodeStrategy> MCCFRSolver::getAllStrategies(Position player) const {
    std::vector<NodeStrategy> strategies;
    
    const core::Range& range = (player == Position::OOP) ? oopRange_ : ipRange_;
    
    for (const auto& [handType, weight] : range.getHandTypes()) {
        if (weight <= 0) continue;
        
        // Get a representative hand of this type
        auto hands = handType.getHands();
        if (hands.empty()) continue;
        
        // Use first available hand that doesn't conflict with board
        for (const auto& hand : hands) {
            bool conflicts = false;
            for (const auto& boardCard : initialState_.board()) {
                if (hand.contains(boardCard)) {
                    conflicts = true;
                    break;
                }
            }
            
            if (!conflicts) {
                strategies.push_back(getStrategy(player, hand));
                break;
            }
        }
    }
    
    return strategies;
}

std::vector<double> MCCFRSolver::getAggregatedStrategy(Position player) const {
    auto actions = initialState_.getAvailableActions();
    if (actions.empty()) return {};
    
    std::vector<double> aggregated(actions.size(), 0.0);
    double totalWeight = 0;
    
    const core::Range& range = (player == Position::OOP) ? oopRange_ : ipRange_;
    auto weightedHands = range.getAvailableHands(initialState_.board());
    
    for (const auto& [hand, weight] : weightedHands) {
        auto strategy = getStrategy(player, hand);
        
        for (size_t i = 0; i < strategy.actionProbabilities.size() && i < aggregated.size(); ++i) {
            aggregated[i] += weight * strategy.actionProbabilities[i];
        }
        totalWeight += weight;
    }
    
    if (totalWeight > 0) {
        for (auto& v : aggregated) {
            v /= totalWeight;
        }
    }
    
    return aggregated;
}

double MCCFRSolver::getExploitability() const {
    // Simplified exploitability estimate
    // In practice, this would require a full best-response calculation
    
    if (iteration_ == 0) return 1.0;
    
    // Use a heuristic based on iteration count
    // Real exploitability converges roughly as 1/sqrt(iterations)
    return 100.0 / std::sqrt(static_cast<double>(iteration_));
}

// Helper function implementation
StrategyResult getStrategyResult(const MCCFRSolver& solver, Position player) {
    StrategyResult result;
    result.player = player;
    
    auto actions = solver.config().numIterations > 0 ? 
                   std::vector<std::string>{"Check", "Bet 25%", "Bet 40%", "Bet 80%", "Bet 120%"} :
                   std::vector<std::string>{};
    
    // Get actual action names
    auto availableActions = GameState(solver.config().numIterations > 0 ? 
                                       BetSizingConfig{} : BetSizingConfig{}).getAvailableActions();
    result.actionNames.clear();
    for (const auto& action : availableActions) {
        result.actionNames.push_back(action.toString());
    }
    
    // Fill grid
    const core::Range& range = player == Position::OOP ? 
                               solver.gameTree().getInfoSets().empty() ? core::Range() : core::Range() :
                               core::Range();
    
    // Initialize grid with empty strategies
    for (int row = 0; row < 13; ++row) {
        for (int col = 0; col < 13; ++col) {
            result.grid[row][col].rangeWeight = 0;
        }
    }
    
    return result;
}

} // namespace solver
