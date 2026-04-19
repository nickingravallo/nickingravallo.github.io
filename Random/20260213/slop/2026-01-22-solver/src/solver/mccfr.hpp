#pragma once

#include "game_tree.hpp"
#include "game_state.hpp"
#include "core/range.hpp"
#include "core/hand.hpp"
#include <functional>
#include <atomic>
#include <mutex>
#include <random>
#include <thread>

namespace solver {

/**
 * Configuration for MCCFR solver
 */
struct MCCFRConfig {
    int numIterations = 10000;
    int numThreads = 1;  // Single-threaded by default for reliability
    bool useExternalSampling = true;  // External sampling is more stable
    bool useDiscounting = true;
    double discountAlpha = 1.5;
    double discountBeta = 0.0;
    double discountGamma = 2.0;
    
    // Callback frequency (iterations between progress updates)
    int progressCallbackFrequency = 100;
};

/**
 * Progress information during solving
 */
struct SolveProgress {
    int currentIteration;
    int totalIterations;
    double exploitability;  // Measure of how far from Nash equilibrium
    bool complete;
    std::string status;
};

/**
 * Result of solving a game tree node
 */
struct NodeStrategy {
    std::string handType;  // Canonical hand (e.g., "AKs", "QQ")
    std::vector<double> actionProbabilities;
    std::vector<std::string> actionNames;
};

/**
 * MCCFR (Monte Carlo Counterfactual Regret Minimization) Solver
 * 
 * Uses external sampling variant for efficiency and stability.
 * Single-threaded by default to ensure reliable results.
 */
class MCCFRSolver {
public:
    MCCFRSolver();
    explicit MCCFRSolver(const MCCFRConfig& config);
    
    // Set configuration
    void setConfig(const MCCFRConfig& config) { config_ = config; }
    const MCCFRConfig& config() const { return config_; }
    
    // Initialize solver with game setup
    void initialize(const GameState& state,
                    const core::Range& oopRange,
                    const core::Range& ipRange);
    
    // Run the solver
    void solve();
    
    // Run a single iteration (for progressive solving)
    void runIteration();
    
    // Stop solving
    void stop() { shouldStop_ = true; }
    bool isStopped() const { return shouldStop_; }
    void reset();
    
    // Get current iteration
    int currentIteration() const { return iteration_; }
    
    // Get strategy for a specific hand at current node
    NodeStrategy getStrategy(Position player, const core::Hand& hand) const;
    
    // Get strategy for all hands in range at current node
    std::vector<NodeStrategy> getAllStrategies(Position player) const;
    
    // Get aggregated strategy (weighted by hand frequency in range)
    std::vector<double> getAggregatedStrategy(Position player) const;
    
    // Progress callback
    using ProgressCallback = std::function<void(const SolveProgress&)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }
    
    // Get exploitability estimate (lower is better, 0 = Nash equilibrium)
    double getExploitability() const;
    
    // Access game tree
    const GameTree& gameTree() const { return gameTree_; }

private:
    MCCFRConfig config_;
    GameTree gameTree_;
    GameState initialState_;
    core::Range oopRange_;
    core::Range ipRange_;
    
    std::atomic<int> iteration_{0};
    std::atomic<bool> shouldStop_{false};
    
    ProgressCallback progressCallback_;
    
    // Random number generators (one per thread for thread safety)
    std::vector<std::mt19937> rngs_;
    std::mutex rngMutex_;
    
    // External sampling CFR traversal
    double externalSample(const GameState& state,
                          const core::Hand& oopHand,
                          const core::Hand& ipHand,
                          Position traversingPlayer,
                          double oopReach,
                          double ipReach,
                          std::mt19937& rng);
    
    // Get or create info set for a hand at a state
    std::shared_ptr<InfoSet> getInfoSet(Position player,
                                         const core::Hand& hand,
                                         const GameState& state);
    
    // Generate info set key
    std::string makeInfoSetKey(Position player,
                               const core::Hand& hand,
                               const GameState& state) const;
    
    // Sample a hand from a range (accounting for dead cards)
    core::Hand sampleHand(const core::Range& range,
                          const std::vector<core::Card>& deadCards,
                          std::mt19937& rng);
    
    // Apply discounting to regrets and strategies
    void applyDiscounting();
    
    // Update progress
    void reportProgress();
};

/**
 * Strategy result for display
 */
struct StrategyResult {
    // Grid of strategies: [13][13] for hand matrix
    // Each cell contains action probabilities
    struct CellStrategy {
        std::string handType;
        std::vector<double> actionProbs;
        double rangeWeight;  // How much of range this hand is
    };
    
    std::array<std::array<CellStrategy, 13>, 13> grid;
    std::vector<std::string> actionNames;
    Position player;
};

// Helper to get a full strategy result for display
StrategyResult getStrategyResult(const MCCFRSolver& solver, Position player);

} // namespace solver
