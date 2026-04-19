#include "strategy_grid.hpp"
#include <QToolTip>
#include <QPainter>

namespace gui {

// StrategyCell implementation

StrategyCell::StrategyCell(QWidget* parent) : QWidget(parent) {
    setFixedSize(36, 36);
    setMinimumSize(36, 36);
}

void StrategyCell::setActionProbabilities(const std::vector<double>& probs) {
    actionProbs_ = probs;
    update();
}

void StrategyCell::setActionColors(const std::vector<QColor>& colors) {
    actionColors_ = colors;
    update();
}

void StrategyCell::setHandType(const QString& handType) {
    handType_ = handType;
    update();
}

void StrategyCell::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect().adjusted(1, 1, -1, -1);
    
    // Draw background
    painter.setBrush(QColor("#2a2a3a"));
    painter.setPen(QPen(QColor("#1a1a2a"), 1));
    painter.drawRoundedRect(rect, 2, 2);
    
    if (actionProbs_.empty() || actionColors_.empty()) {
        // No strategy - just show hand type text
        painter.setPen(QColor("#888"));
        QFont font = painter.font();
        font.setPointSize(9);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(rect, Qt::AlignCenter, handType_);
        return;
    }
    
    // Draw segments based on action percentages
    // We'll draw horizontally (left to right)
    double x = rect.left();
    double totalWidth = rect.width();
    
    // Draw segments for each action
    for (size_t i = 0; i < actionProbs_.size() && i < actionColors_.size(); ++i) {
        if (actionProbs_[i] <= 0.0) continue;
        
        double segmentWidth = totalWidth * actionProbs_[i];
        if (segmentWidth < 0.5) continue;  // Skip tiny segments
        
        QRect segmentRect(static_cast<int>(x), rect.top(), 
                         static_cast<int>(segmentWidth), rect.height());
        
        painter.setBrush(actionColors_[i]);
        painter.setPen(Qt::NoPen);
        painter.drawRect(segmentRect);
        
        x += segmentWidth;
    }
    
    // Draw hand type text on top (with outline for visibility)
    painter.setPen(QColor("#ffffff"));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);
    
    // Draw text with outline for better visibility
    QPen outlinePen(QColor("#000000"), 2);
    painter.setPen(outlinePen);
    painter.drawText(rect, Qt::AlignCenter, handType_);
    painter.setPen(QColor("#ffffff"));
    painter.drawText(rect, Qt::AlignCenter, handType_);
}


StrategyGrid::StrategyGrid(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void StrategyGrid::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    
    // Title
    QLabel* titleLabel = new QLabel("Strategy", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #ddd;");
    mainLayout->addWidget(titleLabel);
    
    // Grid container
    QWidget* gridContainer = new QWidget(this);
    gridLayout_ = new QGridLayout(gridContainer);
    gridLayout_->setSpacing(1);
    gridLayout_->setContentsMargins(0, 0, 0, 0);
    
    // Column headers
    const char* ranks = "AKQJT98765432";
    for (int c = 0; c < 13; ++c) {
        QLabel* label = new QLabel(QString(ranks[c]), this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #888; font-weight: bold; font-size: 10px;");
        label->setFixedSize(36, 16);
        gridLayout_->addWidget(label, 0, c + 1);
    }
    
    // Row headers and cells
    for (int r = 0; r < 13; ++r) {
        QLabel* rowLabel = new QLabel(QString(ranks[r]), this);
        rowLabel->setAlignment(Qt::AlignCenter);
        rowLabel->setStyleSheet("color: #888; font-weight: bold; font-size: 10px;");
        rowLabel->setFixedSize(16, 36);
        gridLayout_->addWidget(rowLabel, r + 1, 0);
        
        for (int c = 0; c < 13; ++c) {
            StrategyCell* cell = new StrategyCell(this);
            cell->setHandType(getHandTypeString(r, c));
            // Colors will be set in updateDisplay after actions are known
            
            cells_[r][c] = cell;
            gridLayout_->addWidget(cell, r + 1, c + 1);
        }
    }
    
    mainLayout->addWidget(gridContainer);
    
    // Legend
    legendWidget_ = new QWidget(this);
    auto* legendLayout = new QHBoxLayout(legendWidget_);
    legendLayout->setSpacing(10);
    legendLayout->setContentsMargins(0, 5, 0, 0);
    
    // Default legend items
    QStringList defaultActions = {"Check", "Bet 25%", "Bet 40%", "Bet 80%", "Bet 120%", "Fold"};
    for (int i = 0; i < defaultActions.size() && i < static_cast<int>(actionColors_.size()); ++i) {
        QLabel* colorBox = new QLabel(this);
        colorBox->setFixedSize(12, 12);
        colorBox->setStyleSheet(QString("background-color: %1; border-radius: 2px;")
                               .arg(actionColors_[i].name()));
        legendLayout->addWidget(colorBox);
        
        QLabel* label = new QLabel(defaultActions[i], this);
        label->setStyleSheet("color: #888; font-size: 10px;");
        legendLayout->addWidget(label);
    }
    legendLayout->addStretch();
    
    mainLayout->addWidget(legendWidget_);
    mainLayout->addStretch();
}

void StrategyGrid::setStrategy(const std::vector<solver::NodeStrategy>& strategies,
                                const std::vector<std::string>& actionNames) {
    actionNames_ = actionNames;
    
    // Clear previous data
    for (auto& row : actionProbs_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    for (auto& row : handStrategies_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    
    // Map strategies to grid positions
    for (const auto& strat : strategies) {
        auto handType = core::HandType::fromString(strat.handType);
        if (!handType) continue;
        
        auto [row, col] = handType->gridPosition();
        actionProbs_[row][col] = strat.actionProbabilities;
    }
    
    updateDisplay();
}

void StrategyGrid::setStrategyWithHands(const std::vector<solver::NodeStrategy>& strategies,
                                         const std::map<std::string, std::vector<solver::NodeStrategy>>& handStrategies,
                                         const std::vector<std::string>& actionNames) {
    actionNames_ = actionNames;
    
    // Clear previous data
    for (auto& row : actionProbs_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    for (auto& row : handStrategies_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    
    // Map strategies to grid positions
    for (const auto& strat : strategies) {
        auto handType = core::HandType::fromString(strat.handType);
        if (!handType) continue;
        
        auto [row, col] = handType->gridPosition();
        actionProbs_[row][col] = strat.actionProbabilities;
        
        // Store individual hand strategies for this hand type
        std::string handTypeStr = strat.handType;
        auto it = handStrategies.find(handTypeStr);
        if (it != handStrategies.end()) {
            for (const auto& handStrat : it->second) {
                // handStrat.handType now contains the actual hand string (e.g., "AsKs")
                // Store it with the actual hand string as key
                handStrategies_[row][col][handStrat.handType] = handStrat.actionProbabilities;
            }
        }
    }
    
    updateDisplay();
}

void StrategyGrid::clear() {
    for (auto& row : actionProbs_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    for (auto& row : handStrategies_) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
    actionNames_.clear();
    updateDisplay();
}

void StrategyGrid::setAvailableActions(const std::vector<solver::Action>& actions) {
    availableActions_ = actions;
}

void StrategyGrid::setHighlightAction(int actionIndex) {
    highlightAction_ = actionIndex;
    updateDisplay();
}

// Map action index to color index based on action type and size
int StrategyGrid::mapActionToColorIndex(int actionIndex) const {
    if (actionIndex < 0 || actionIndex >= static_cast<int>(availableActions_.size())) {
        return -1;
    }
    
    const auto& action = availableActions_[actionIndex];
    
    switch (action.type) {
        case solver::ActionType::FOLD:
            return 0;  // Blue
        case solver::ActionType::CHECK:
        case solver::ActionType::CALL:
            return 1;  // Green
        case solver::ActionType::BET:
            // Map bet size to color index
            if (action.potFraction <= 0.26) return 2;  // 25% - Orange
            if (action.potFraction <= 0.45) return 3;  // 40% - Light red
            if (action.potFraction <= 0.85) return 4;  // 80% - Red
            return 5;  // 120% - Purple
        case solver::ActionType::RAISE:
        case solver::ActionType::ALL_IN:
            return 5;  // Purple
        default:
            return 1;  // Default to green
    }
}

void StrategyGrid::updateDisplay() {
    for (int r = 0; r < 13; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (!cells_[r][c]) continue;
            
            const auto& probs = actionProbs_[r][c];
            
            // Set colors on cell (in correct order: Fold, Check/Call, Bet25%, Bet40%, Bet80%, Bet120%/Raise)
            cells_[r][c]->setActionColors(actionColors_);
            
            if (probs.empty()) {
                // No strategy - clear the cell
                cells_[r][c]->setActionProbabilities({});
            } else {
                // Reorder probabilities by color index
                // Color order: Fold(0), Check/Call(1), Bet25%(2), Bet40%(3), Bet80%(4), Bet120%/Raise(5)
                std::vector<double> colorOrderedProbs(6, 0.0);
                
                for (size_t i = 0; i < probs.size(); ++i) {
                    int colorIdx = mapActionToColorIndex(static_cast<int>(i));
                    if (colorIdx >= 0 && colorIdx < 6) {
                        colorOrderedProbs[colorIdx] += probs[i];
                    }
                }
                
                cells_[r][c]->setActionProbabilities(colorOrderedProbs);
            }
            
            cells_[r][c]->setToolTip(getCellTooltip(r, c));
        }
    }
}

QColor StrategyGrid::blendColors(const std::vector<double>& weights) const {
    if (weights.empty()) return QColor("#2a2a3a");
    
    double r = 0, g = 0, b = 0;
    double totalWeight = 0;
    
    for (size_t i = 0; i < weights.size() && i < actionColors_.size(); ++i) {
        double w = weights[i];
        if (w <= 0) continue;
        
        r += actionColors_[i].redF() * w;
        g += actionColors_[i].greenF() * w;
        b += actionColors_[i].blueF() * w;
        totalWeight += w;
    }
    
    if (totalWeight <= 0) return QColor("#2a2a3a");
    
    r /= totalWeight;
    g /= totalWeight;
    b /= totalWeight;
    
    // Darken the color to ensure white text is visible
    // Mix with dark background (30% dark, 70% original)
    r = r * 0.7 + 0.15;
    g = g * 0.7 + 0.15;
    b = b * 0.7 + 0.15;
    
    return QColor::fromRgbF(r, g, b);
}

QString StrategyGrid::getCellTooltip(int row, int col) const {
    const auto& probs = actionProbs_[row][col];
    if (probs.empty()) return getHandTypeString(row, col);
    
    QString tooltip = QString("<b>%1</b>").arg(getHandTypeString(row, col));
    tooltip += "<hr>";
    
    // Check if we have individual hand strategies
    const auto& handStratMap = handStrategies_[row][col];
    
    if (!handStratMap.empty()) {
        // Show each individual combo with its percentages
        tooltip += "<b>Individual Combos:</b><br>";
        for (const auto& [handStr, handProbs] : handStratMap) {
            tooltip += QString("<br><b>%1:</b><br>").arg(QString::fromStdString(handStr));
            for (size_t i = 0; i < handProbs.size() && i < actionNames_.size(); ++i) {
                if (handProbs[i] > 0.01) {
                    tooltip += QString("&nbsp;&nbsp;%1: <b>%2%</b><br>")
                        .arg(QString::fromStdString(actionNames_[i]))
                        .arg(static_cast<int>(handProbs[i] * 100));
                }
            }
        }
    } else {
        // Fallback: show aggregated percentages for the hand type
        tooltip += "<b>Aggregated Strategy:</b><br>";
        for (size_t i = 0; i < probs.size() && i < actionNames_.size(); ++i) {
            if (probs[i] > 0.01) {
                tooltip += QString("%1: <b>%2%</b><br>")
                    .arg(QString::fromStdString(actionNames_[i]))
                    .arg(static_cast<int>(probs[i] * 100));
            }
        }
    }
    
    return tooltip;
}

QString StrategyGrid::getHandTypeString(int row, int col) const {
    const char* ranks = "AKQJT98765432";
    
    if (row == col) {
        return QString("%1%1").arg(ranks[row]);
    } else if (row < col) {
        return QString("%1%2s").arg(ranks[row]).arg(ranks[col]);
    } else {
        return QString("%1%2o").arg(ranks[col]).arg(ranks[row]);
    }
}

void StrategyGrid::enterEvent(QEnterEvent*) {
    // Could add hover effects here
}

void StrategyGrid::leaveEvent(QEvent*) {
    // Could remove hover effects here
}

// StrategyLegend implementation

StrategyLegend::StrategyLegend(QWidget* parent) : QWidget(parent) {
    layout_ = new QHBoxLayout(this);
    layout_->setSpacing(8);
    layout_->setContentsMargins(0, 0, 0, 0);
}

void StrategyLegend::setActions(const std::vector<std::string>& actionNames,
                                 const std::vector<QColor>& colors) {
    // Clear existing items
    QLayoutItem* item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Add new legend items
    for (size_t i = 0; i < actionNames.size() && i < colors.size(); ++i) {
        QLabel* colorBox = new QLabel(this);
        colorBox->setFixedSize(12, 12);
        colorBox->setStyleSheet(QString("background-color: %1; border-radius: 2px;")
                               .arg(colors[i].name()));
        layout_->addWidget(colorBox);
        
        QLabel* label = new QLabel(QString::fromStdString(actionNames[i]), this);
        label->setStyleSheet("color: #888; font-size: 10px;");
        layout_->addWidget(label);
    }
    
    layout_->addStretch();
}

} // namespace gui
