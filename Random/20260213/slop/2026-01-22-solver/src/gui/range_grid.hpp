#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <array>
#include "core/range.hpp"

namespace gui {

/**
 * Widget displaying a 13x13 hand range grid.
 * Pairs on diagonal, suited above, offsuit below.
 */
class RangeGrid : public QWidget {
    Q_OBJECT

public:
    explicit RangeGrid(QWidget* parent = nullptr);
    
    // Set range to display
    void setRange(const core::Range& range);
    
    // Get current range
    core::Range getRange() const;
    
    // Set weights for display (e.g., from solver strategy)
    void setWeights(const std::array<std::array<double, 13>, 13>& weights);
    
    // Interactive mode (allow clicking to modify range)
    void setInteractive(bool interactive) { interactive_ = interactive; }
    bool isInteractive() const { return interactive_; }
    
    // Set color scheme for strategy display
    void setActionColors(const std::vector<QColor>& colors);
    
    // Display mode: range selection or strategy display
    enum class DisplayMode {
        RANGE,    // Show range inclusion (green = in range)
        STRATEGY  // Show action distribution (multiple colors)
    };
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; update(); }

signals:
    void rangeChanged(const core::Range& range);
    void cellClicked(int row, int col);

private slots:
    void onCellClicked();

private:
    void setupUI();
    void updateDisplay();
    QString getCellStyle(int row, int col) const;
    QString getHandTypeString(int row, int col) const;
    
    QGridLayout* gridLayout_;
    std::array<std::array<QPushButton*, 13>, 13> cells_;
    std::array<std::array<double, 13>, 13> weights_;
    std::array<std::array<std::vector<double>, 13>, 13> actionWeights_;  // For strategy display
    std::vector<QColor> actionColors_;
    
    bool interactive_ = true;
    DisplayMode displayMode_ = DisplayMode::RANGE;
};

} // namespace gui
