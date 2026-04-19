#include "range_grid.hpp"
#include <QFont>

namespace gui {

RangeGrid::RangeGrid(QWidget* parent) : QWidget(parent) {
    // Initialize weights to zero
    for (auto& row : weights_) {
        row.fill(0.0);
    }
    
    // Default action colors
    actionColors_ = {
        QColor("#27ae60"),  // Check/Call - Green
        QColor("#e67e22"),  // Bet small - Light orange
        QColor("#e74c3c"),  // Bet medium - Red
        QColor("#c0392b"),  // Bet large - Dark red
        QColor("#8e44ad"),  // Raise - Purple
        QColor("#3498db")   // Fold - Blue
    };
    
    setupUI();
}

void RangeGrid::setupUI() {
    gridLayout_ = new QGridLayout(this);
    gridLayout_->setSpacing(1);
    gridLayout_->setContentsMargins(2, 2, 2, 2);
    
    // Column headers (ranks A-2)
    const char* ranks = "AKQJT98765432";
    for (int c = 0; c < 13; ++c) {
        QLabel* label = new QLabel(QString(ranks[c]), this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #888; font-weight: bold; font-size: 10px;");
        label->setFixedSize(32, 16);
        gridLayout_->addWidget(label, 0, c + 1);
    }
    
    // Row headers and cells
    for (int r = 0; r < 13; ++r) {
        QLabel* rowLabel = new QLabel(QString(ranks[r]), this);
        rowLabel->setAlignment(Qt::AlignCenter);
        rowLabel->setStyleSheet("color: #888; font-weight: bold; font-size: 10px;");
        rowLabel->setFixedSize(16, 32);
        gridLayout_->addWidget(rowLabel, r + 1, 0);
        
        for (int c = 0; c < 13; ++c) {
            QPushButton* btn = new QPushButton(this);
            btn->setFixedSize(32, 32);
            btn->setProperty("row", r);
            btn->setProperty("col", c);
            btn->setText(getHandTypeString(r, c));
            btn->setStyleSheet(getCellStyle(r, c));
            
            QFont font = btn->font();
            font.setPointSize(7);
            font.setBold(true);
            btn->setFont(font);
            
            connect(btn, &QPushButton::clicked, this, &RangeGrid::onCellClicked);
            
            cells_[r][c] = btn;
            gridLayout_->addWidget(btn, r + 1, c + 1);
        }
    }
}

void RangeGrid::setRange(const core::Range& range) {
    weights_ = range.getGridWeights();
    updateDisplay();
}

core::Range RangeGrid::getRange() const {
    core::Range range;
    
    const char* ranks = "AKQJT98765432";
    
    for (int r = 0; r < 13; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (weights_[r][c] > 0) {
                std::string handStr;
                handStr += ranks[r];
                handStr += ranks[c];
                
                if (r == c) {
                    // Pair
                } else if (r > c) {
                    // Offsuit (below diagonal)
                    handStr += 'o';
                } else {
                    // Suited (above diagonal)
                    handStr += 's';
                }
                
                range.addHandType(handStr, weights_[r][c]);
            }
        }
    }
    
    return range;
}

void RangeGrid::setWeights(const std::array<std::array<double, 13>, 13>& weights) {
    weights_ = weights;
    updateDisplay();
}

void RangeGrid::setActionColors(const std::vector<QColor>& colors) {
    actionColors_ = colors;
    updateDisplay();
}

void RangeGrid::onCellClicked() {
    if (!interactive_) return;
    
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int row = btn->property("row").toInt();
    int col = btn->property("col").toInt();
    
    // Toggle weight
    if (weights_[row][col] > 0) {
        weights_[row][col] = 0;
    } else {
        weights_[row][col] = 100;
    }
    
    updateDisplay();
    emit cellClicked(row, col);
    emit rangeChanged(getRange());
}

void RangeGrid::updateDisplay() {
    for (int r = 0; r < 13; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (cells_[r][c]) {
                cells_[r][c]->setStyleSheet(getCellStyle(r, c));
            }
        }
    }
}

QString RangeGrid::getCellStyle(int row, int col) const {
    double weight = weights_[row][col];
    
    QString bgColor, textColor, borderColor;
    
    if (displayMode_ == DisplayMode::RANGE) {
        // Simple range display
        if (weight >= 100) {
            bgColor = "#27ae60";  // Full inclusion - green
            textColor = "#fff";
        } else if (weight > 0) {
            // Partial inclusion - lighter green
            int greenValue = static_cast<int>(0x60 + (0xa0 - 0x60) * (1 - weight / 100));
            bgColor = QString("#27%1%2").arg(greenValue, 2, 16, QChar('0')).arg(0x60, 2, 16, QChar('0'));
            textColor = "#fff";
        } else {
            bgColor = "#2a2a3a";  // Not in range
            textColor = "#666";
        }
        borderColor = "#1a1a2a";
    } else {
        // Strategy display mode
        if (!actionWeights_[row][col].empty()) {
            // Blend colors based on action weights
            // For now, just show dominant action color
            const auto& actions = actionWeights_[row][col];
            int maxIdx = 0;
            double maxVal = 0;
            for (size_t i = 0; i < actions.size(); ++i) {
                if (actions[i] > maxVal) {
                    maxVal = actions[i];
                    maxIdx = static_cast<int>(i);
                }
            }
            
            if (maxIdx < static_cast<int>(actionColors_.size())) {
                bgColor = actionColors_[maxIdx].name();
            } else {
                bgColor = "#2a2a3a";
            }
            textColor = "#fff";
        } else if (weight > 0) {
            bgColor = "#4a4a5a";
            textColor = "#aaa";
        } else {
            bgColor = "#2a2a3a";
            textColor = "#444";
        }
        borderColor = "#1a1a2a";
    }
    
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "}"
        "QPushButton:hover {"
        "  border: 1px solid #5a5a6a;"
        "}"
    ).arg(bgColor, textColor, borderColor);
}

QString RangeGrid::getHandTypeString(int row, int col) const {
    const char* ranks = "AKQJT98765432";
    QString str;
    
    if (row == col) {
        // Pair
        str = QString("%1%1").arg(ranks[row]);
    } else if (row < col) {
        // Suited (above diagonal)
        str = QString("%1%2s").arg(ranks[row]).arg(ranks[col]);
    } else {
        // Offsuit (below diagonal)
        str = QString("%1%2o").arg(ranks[col]).arg(ranks[row]);
    }
    
    return str;
}

} // namespace gui
