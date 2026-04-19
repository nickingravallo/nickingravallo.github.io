#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <array>
#include <vector>
#include <map>
#include "solver/mccfr.hpp"
#include "solver/game_state.hpp"

namespace gui {

/**
 * Custom cell widget that displays action percentages as colored segments
 */
class StrategyCell : public QWidget {
    Q_OBJECT

public:
    explicit StrategyCell(QWidget* parent = nullptr);
    
    void setActionProbabilities(const std::vector<double>& probs);
    void setActionColors(const std::vector<QColor>& colors);
    void setHandType(const QString& handType);
    
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<double> actionProbs_;
    std::vector<QColor> actionColors_;
    QString handType_;
};


/**
 * Strategy display widget showing action frequencies for each hand.
 * Uses color coding:
 * - Green: Check/Call
 * - Red shades: Bets (darker = larger)
 * - Blue: Fold
 */
class StrategyGrid : public QWidget {
    Q_OBJECT

public:
    explicit StrategyGrid(QWidget* parent = nullptr);
    
    // Set strategy data to display
    void setStrategy(const std::vector<solver::NodeStrategy>& strategies,
                     const std::vector<std::string>& actionNames);
    
    // Set strategy with individual hand data (for detailed tooltips)
    void setStrategyWithHands(const std::vector<solver::NodeStrategy>& strategies,
                              const std::map<std::string, std::vector<solver::NodeStrategy>>& handStrategies,
                              const std::vector<std::string>& actionNames);
    
    // Set available actions for color mapping
    void setAvailableActions(const std::vector<solver::Action>& actions);
    
    // Clear display
    void clear();
    
    // Set which action to highlight
    void setHighlightAction(int actionIndex);

signals:
    void cellHovered(const QString& handType, const std::vector<double>& actionProbs);
    void cellClicked(const QString& handType);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();
    void updateDisplay();
    QColor blendColors(const std::vector<double>& weights) const;
    QString getCellTooltip(int row, int col) const;
    QString getHandTypeString(int row, int col) const;
    int mapActionToColorIndex(int actionIndex) const;
    
    QGridLayout* gridLayout_;
    std::array<std::array<StrategyCell*, 13>, 13> cells_;
    std::array<std::array<std::vector<double>, 13>, 13> actionProbs_;
    // Store individual hand strategies for detailed tooltips
    // Map: hand string (e.g., "AsKs") -> action probabilities
    std::array<std::array<std::map<std::string, std::vector<double>>, 13>, 13> handStrategies_;
    std::vector<std::string> actionNames_;
    std::vector<solver::Action> availableActions_;  // For color mapping
    
    // Action colors (configurable)
    // Order: Fold, Check/Call, Bet 25%, Bet 40%, Bet 80%, Bet 120%/Raise
    std::vector<QColor> actionColors_ = {
        QColor("#3498db"),  // Fold - Blue
        QColor("#27ae60"),  // Check/Call - Green
        QColor("#f39c12"),  // Bet 25% - Orange
        QColor("#e74c3c"),  // Bet 40% - Light red
        QColor("#c0392b"),  // Bet 80% - Red
        QColor("#8e44ad")   // Bet 120% / Raise - Purple
    };
    
    int highlightAction_ = -1;
    
    // Legend labels
    QWidget* legendWidget_;
};

/**
 * Legend widget showing action colors
 */
class StrategyLegend : public QWidget {
    Q_OBJECT

public:
    explicit StrategyLegend(QWidget* parent = nullptr);
    
    void setActions(const std::vector<std::string>& actionNames,
                    const std::vector<QColor>& colors);

private:
    QHBoxLayout* layout_;
};

} // namespace gui
