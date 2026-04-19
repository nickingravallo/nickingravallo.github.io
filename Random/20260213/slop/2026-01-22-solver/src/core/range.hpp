#pragma once

#include "hand.hpp"
#include <vector>
#include <map>
#include <string>
#include <set>

namespace core {

/**
 * Represents a poker hand range with optional weights (frequencies).
 * 
 * Supports parsing range notations:
 * - "AA" - Single hand type
 * - "22-AA" - Range of pairs
 * - "AKs-ATs" - Range of suited hands
 * - "AKo-AJo" - Range of offsuit hands
 * - "AJo@50" - Hand type with 50% frequency
 * - "AKs, QQ, JTs-T9s" - Comma-separated combinations
 */
class Range {
public:
    Range() = default;
    
    // Parse a range string
    static Range fromString(const std::string& rangeStr);
    
    // Add hand types with optional weight (0-100)
    void addHandType(const HandType& type, double weight = 100.0);
    void addHandType(const std::string& typeStr, double weight = 100.0);
    
    // Add a range of hand types (e.g., "22-AA", "AKs-ATs")
    void addRange(const std::string& rangeStr, double weight = 100.0);
    
    // Remove hand type
    void removeHandType(const HandType& type);
    
    // Clear all hands
    void clear();
    
    // Get weight for a specific hand type (0-100)
    double getWeight(const HandType& type) const;
    double getWeight(const Hand& hand) const;
    
    // Set weight for a hand type
    void setWeight(const HandType& type, double weight);
    
    // Check if hand type is in range
    bool contains(const HandType& type) const;
    bool contains(const Hand& hand) const;
    
    // Get all hand types in range
    const std::map<HandType, double>& getHandTypes() const { return weights_; }
    
    // Get all specific hands with their weights (expands to actual card combinations)
    std::vector<std::pair<Hand, double>> getWeightedHands() const;
    
    // Get hands that don't conflict with dead cards
    std::vector<std::pair<Hand, double>> getAvailableHands(
        const std::vector<Card>& deadCards) const;
    
    // Calculate total number of combos (accounting for weights)
    double totalCombos() const;
    
    // String representation
    std::string toString() const;
    
    // Get grid representation (13x13 matrix of weights)
    // Index by [row][col] where AA is [0][0]
    std::array<std::array<double, 13>, 13> getGridWeights() const;
    
private:
    std::map<HandType, double> weights_;  // HandType -> weight (0-100)
    
    // Parse helpers
    static bool parseHandTypeWithWeight(const std::string& token, 
                                       HandType& type, double& weight);
    static bool parseRange(const std::string& rangeStr,
                          std::vector<HandType>& types);
    static std::vector<HandType> expandPairRange(Rank low, Rank high);
    static std::vector<HandType> expandSuitedRange(Rank highStart, Rank lowStart,
                                                   Rank highEnd, Rank lowEnd);
    static std::vector<HandType> expandOffsuitRange(Rank highStart, Rank lowStart,
                                                    Rank highEnd, Rank lowEnd);
};

// Default ranges for common situations
namespace DefaultRanges {
    // 6max UTG open range (approximately 15%)
    const std::string UTG_OPEN = 
        "77+, ATs+, KQs, AJo+, KQo";
    
    // 6max BTN call vs UTG open (approximately 12%)
    const std::string BTN_CALL_VS_UTG = 
        "66-TT, ATs-AQs, KQs, KJs, QJs, JTs, T9s, 98s, 87s, 76s, AQo";
    
    // Wider UTG range (approximately 18%)
    const std::string UTG_OPEN_WIDE = 
        "66+, A9s+, KTs+, QTs+, JTs, T9s, ATo+, KJo+";
    
    // BTN 3bet range vs UTG
    const std::string BTN_3BET_VS_UTG = 
        "QQ+, AKs, AKo";
}

} // namespace core
