#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <memory>

#include "solver/mccfr.hpp"
#include "solver/game_state.hpp"
#include "core/range.hpp"

namespace gui {

// Forward declarations
class CardSelector;
class RangeGrid;
class ActionPanel;
class StrategyGrid;
class ProgressPanel;

/**
 * Main window for the GTO Solver application
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFlopCardSelected(int index);
    void onTurnCardSelected();
    void onRiverCardSelected();
    void onActionSelected(int actionIndex);
    void onSolveClicked();
    void onStopClicked();
    void onResetClicked();
    void onLoadDefaultRanges();
    void onPlayerChanged(int index);
    void onStreetChanged(int index);
    void onStackSizeChanged(int value);
    void onUndoClicked();
    void updateDisplay();
    void onSolveProgress(int iteration, int total, double exploitability);

signals:
    void solveProgressUpdated(int iteration, int total, double exploitability);

private:
    void setupUI();
    void setupConnections();
    void createMenus();
    void updateBoardDisplay();
    void updateActionHistory();
    void updateStrategyDisplay();
    void runSolver();
    void narrowRangeAfterAction(solver::Position player, const solver::Action& action);
    void enableUIForSolving(bool enable);
    void handleStreetCompletion();
    
    // UI Components
    QWidget* centralWidget_;
    
    // Left panel: Setup
    QWidget* setupPanel_;
    QLineEdit* oopRangeInput_;
    QLineEdit* ipRangeInput_;
    QPushButton* loadDefaultsBtn_;
    QSpinBox* stackSizeSpinner_;
    QComboBox* playerSelector_;
    
    // Center panel: Board and actions
    QWidget* boardPanel_;
    CardSelector* flopSelector1_;
    CardSelector* flopSelector2_;
    CardSelector* flopSelector3_;
    CardSelector* turnSelector_;
    CardSelector* riverSelector_;
    QLabel* streetLabel_;
    QLabel* potLabel_;
    QLabel* currentPlayerLabel_;
    ActionPanel* actionPanel_;
    QPushButton* undoBtn_;
    
    // Right panel: Strategy display
    StrategyGrid* strategyGrid_;
    QComboBox* viewPlayerSelector_;
    
    // Bottom panel: Progress and log
    ProgressPanel* progressPanel_;
    QTextEdit* logOutput_;
    QPushButton* solveBtn_;
    QPushButton* stopBtn_;
    QPushButton* resetBtn_;
    
    // Solver state
    std::unique_ptr<solver::MCCFRSolver> solver_;
    solver::GameState gameState_;
    core::Range oopRange_;
    core::Range ipRange_;
    bool solving_ = false;
    
    // Timer for solver updates
    QTimer* solverTimer_;
};

} // namespace gui
