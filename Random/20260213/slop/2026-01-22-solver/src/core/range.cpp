#include "range.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace core {

Range Range::fromString(const std::string& rangeStr) {
    Range range;
    range.addRange(rangeStr);
    return range;
}

void Range::addHandType(const HandType& type, double weight) {
    weight = std::clamp(weight, 0.0, 100.0);
    weights_[type] = weight;
}

void Range::addHandType(const std::string& typeStr, double weight) {
    auto type = HandType::fromString(typeStr);
    if (type) {
        addHandType(*type, weight);
    }
}

void Range::addRange(const std::string& rangeStr, double weight) {
    // Remove whitespace and split by comma
    std::string cleaned;
    for (char c : rangeStr) {
        if (!std::isspace(c)) {
            cleaned += c;
        }
    }
    
    std::vector<std::string> tokens;
    std::stringstream ss(cleaned);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    for (const auto& tok : tokens) {
        // Check for weight suffix (e.g., "AKo@50")
        double tokenWeight = weight;
        std::string handPart = tok;
        
        size_t atPos = tok.find('@');
        if (atPos != std::string::npos) {
            handPart = tok.substr(0, atPos);
            try {
                tokenWeight = std::stod(tok.substr(atPos + 1));
            } catch (...) {
                tokenWeight = weight;
            }
        }
        
        // Check for range notation (e.g., "22-AA", "AKs-ATs")
        size_t dashPos = handPart.find('-');
        if (dashPos != std::string::npos) {
            std::string start = handPart.substr(0, dashPos);
            std::string end = handPart.substr(dashPos + 1);
            
            auto startType = HandType::fromString(start);
            auto endType = HandType::fromString(end);
            
            if (startType && endType) {
                std::vector<HandType> expanded;
                
                // Pair range
                if (startType->isPair() && endType->isPair()) {
                    expanded = expandPairRange(
                        std::min(startType->highRank(), endType->highRank()),
                        std::max(startType->highRank(), endType->highRank())
                    );
                }
                // Suited range
                else if (startType->isSuited() && endType->isSuited()) {
                    expanded = expandSuitedRange(
                        startType->highRank(), startType->lowRank(),
                        endType->highRank(), endType->lowRank()
                    );
                }
                // Offsuit range
                else if (!startType->isSuited() && !endType->isSuited() &&
                         !startType->isPair() && !endType->isPair()) {
                    expanded = expandOffsuitRange(
                        startType->highRank(), startType->lowRank(),
                        endType->highRank(), endType->lowRank()
                    );
                }
                
                for (const auto& type : expanded) {
                    addHandType(type, tokenWeight);
                }
            }
        }
        // Check for + notation (e.g., "77+", "ATs+", "AJo+")
        else if (handPart.back() == '+') {
            std::string base = handPart.substr(0, handPart.length() - 1);
            auto type = HandType::fromString(base);
            
            if (type) {
                if (type->isPair()) {
                    // Expand pair+ (e.g., "77+" = 77-AA)
                    auto expanded = expandPairRange(type->highRank(), Rank::ACE);
                    for (const auto& t : expanded) {
                        addHandType(t, tokenWeight);
                    }
                }
                else {
                    // Expand suited/offsuit+ (e.g., "ATs+" = ATs-AKs)
                    Rank high = type->highRank();
                    Rank low = type->lowRank();
                    
                    for (int r = static_cast<int>(low); r < static_cast<int>(high); ++r) {
                        addHandType(HandType(high, static_cast<Rank>(r), type->isSuited()), tokenWeight);
                    }
                }
            }
        }
        // Single hand type
        else {
            auto type = HandType::fromString(handPart);
            if (type) {
                addHandType(*type, tokenWeight);
            }
        }
    }
}

void Range::removeHandType(const HandType& type) {
    weights_.erase(type);
}

void Range::clear() {
    weights_.clear();
}

double Range::getWeight(const HandType& type) const {
    auto it = weights_.find(type);
    return (it != weights_.end()) ? it->second : 0.0;
}

double Range::getWeight(const Hand& hand) const {
    // Get canonical hand type for this hand
    HandType type(hand.card1().rank(), hand.card2().rank(), hand.isSuited());
    return getWeight(type);
}

void Range::setWeight(const HandType& type, double weight) {
    if (weight <= 0.0) {
        weights_.erase(type);
    } else {
        weights_[type] = std::clamp(weight, 0.0, 100.0);
    }
}

bool Range::contains(const HandType& type) const {
    return getWeight(type) > 0.0;
}

bool Range::contains(const Hand& hand) const {
    return getWeight(hand) > 0.0;
}

std::vector<std::pair<Hand, double>> Range::getWeightedHands() const {
    std::vector<std::pair<Hand, double>> result;
    
    for (const auto& [type, weight] : weights_) {
        if (weight <= 0.0) continue;
        
        auto hands = type.getHands();
        for (const auto& hand : hands) {
            result.emplace_back(hand, weight);
        }
    }
    
    return result;
}

std::vector<std::pair<Hand, double>> Range::getAvailableHands(
        const std::vector<Card>& deadCards) const {
    
    std::set<int> dead;
    for (const auto& card : deadCards) {
        dead.insert(card.value());
    }
    
    std::vector<std::pair<Hand, double>> result;
    
    for (const auto& [type, weight] : weights_) {
        if (weight <= 0.0) continue;
        
        auto hands = type.getHands();
        for (const auto& hand : hands) {
            bool available = !dead.count(hand.card1().value()) &&
                           !dead.count(hand.card2().value());
            if (available) {
                result.emplace_back(hand, weight);
            }
        }
    }
    
    return result;
}

double Range::totalCombos() const {
    double total = 0.0;
    
    for (const auto& [type, weight] : weights_) {
        int combos = type.isPair() ? 6 : (type.isSuited() ? 4 : 12);
        total += combos * (weight / 100.0);
    }
    
    return total;
}

std::string Range::toString() const {
    std::string result;
    
    for (const auto& [type, weight] : weights_) {
        if (!result.empty()) result += ", ";
        result += type.toString();
        if (weight < 100.0) {
            result += "@" + std::to_string(static_cast<int>(weight));
        }
    }
    
    return result;
}

std::array<std::array<double, 13>, 13> Range::getGridWeights() const {
    std::array<std::array<double, 13>, 13> grid = {};
    
    for (const auto& [type, weight] : weights_) {
        auto [row, col] = type.gridPosition();
        grid[row][col] = weight;
    }
    
    return grid;
}

std::vector<HandType> Range::expandPairRange(Rank low, Rank high) {
    std::vector<HandType> types;
    
    int lowIdx = static_cast<int>(low);
    int highIdx = static_cast<int>(high);
    
    for (int r = lowIdx; r <= highIdx; ++r) {
        types.emplace_back(static_cast<Rank>(r), static_cast<Rank>(r), false);
    }
    
    return types;
}

std::vector<HandType> Range::expandSuitedRange(Rank highStart, Rank lowStart,
                                               Rank highEnd, Rank lowEnd) {
    std::vector<HandType> types;
    
    // For suited hands like "ATs-AKs", the high card stays the same
    // and we iterate through the low cards
    if (highStart == highEnd) {
        int lowMin = std::min(static_cast<int>(lowStart), static_cast<int>(lowEnd));
        int lowMax = std::max(static_cast<int>(lowStart), static_cast<int>(lowEnd));
        
        for (int r = lowMin; r <= lowMax; ++r) {
            types.emplace_back(highStart, static_cast<Rank>(r), true);
        }
    }
    
    return types;
}

std::vector<HandType> Range::expandOffsuitRange(Rank highStart, Rank lowStart,
                                                Rank highEnd, Rank lowEnd) {
    std::vector<HandType> types;
    
    if (highStart == highEnd) {
        int lowMin = std::min(static_cast<int>(lowStart), static_cast<int>(lowEnd));
        int lowMax = std::max(static_cast<int>(lowStart), static_cast<int>(lowEnd));
        
        for (int r = lowMin; r <= lowMax; ++r) {
            types.emplace_back(highStart, static_cast<Rank>(r), false);
        }
    }
    
    return types;
}

} // namespace core
