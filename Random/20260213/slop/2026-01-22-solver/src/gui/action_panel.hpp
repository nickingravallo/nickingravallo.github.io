#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include "solver/game_state.hpp"

namespace gui {

/**
 * Panel displaying available betting actions.
 */
class ActionPanel : public QWidget {
    Q_OBJECT

public:
    explicit ActionPanel(QWidget* parent = nullptr);
    
    // Update available actions
    void setActions(const std::vector<solver::Action>& actions);
    
    // Clear all actions
    void clear();
    
    // Enable/disable the panel
    void setActionsEnabled(bool enabled);

signals:
    void actionSelected(int actionIndex);

private slots:
    void onActionClicked();

private:
    void setupUI();
    QString getActionStyle(const solver::Action& action) const;
    QColor getActionColor(solver::ActionType type) const;
    
    QHBoxLayout* layout_;
    QLabel* titleLabel_;
    std::vector<QPushButton*> actionButtons_;
    std::vector<solver::Action> currentActions_;
};

} // namespace gui
