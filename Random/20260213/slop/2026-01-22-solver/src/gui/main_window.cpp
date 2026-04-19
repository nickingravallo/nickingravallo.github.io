#include "main_window.hpp"
#include "card_selector.hpp"
#include "range_grid.hpp"
#include "action_panel.hpp"
#include "strategy_grid.hpp"
#include "progress_panel.hpp"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QThread>
#include <QApplication>
#include <map>

namespace gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("GTO Poker Solver");
    setMinimumSize(1400, 900);
    
    // Dark theme
    setStyleSheet(
        "QMainWindow { background-color: #1e1e2e; }"
        "QWidget { background-color: #1e1e2e; color: #cdd6f4; }"
        "QGroupBox { "
        "  border: 1px solid #3a3a4a; "
        "  border-radius: 6px; "
        "  margin-top: 12px; "
        "  padding-top: 10px; "
        "  font-weight: bold; "
        "}"
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "  color: #89b4fa; "
        "}"
        "QLineEdit { "
        "  background-color: #2a2a3a; "
        "  border: 1px solid #3a3a4a; "
        "  border-radius: 4px; "
        "  padding: 6px; "
        "  color: #cdd6f4; "
        "}"
        "QSpinBox { "
        "  background-color: #2a2a3a; "
        "  border: 1px solid #3a3a4a; "
        "  border-radius: 4px; "
        "  padding: 4px; "
        "}"
        "QComboBox { "
        "  background-color: #2a2a3a; "
        "  border: 1px solid #3a3a4a; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "}"
        "QPushButton { "
        "  background-color: #3a3a4a; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 8px 16px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #4a4a5a; }"
        "QPushButton:pressed { background-color: #2a2a3a; }"
        "QLabel { color: #cdd6f4; }"
    );
    
    setupUI();
    setupConnections();
    createMenus();
    
    // Initialize game state with defaults
    solver::BetSizingConfig config;
    gameState_ = solver::GameState(config);
    
    // Initialize solver
    solver_ = std::make_unique<solver::MCCFRSolver>();
    
    // Solver update timer
    solverTimer_ = new QTimer(this);
    connect(solverTimer_, &QTimer::timeout, this, &MainWindow::runSolver);
    
    // Register metatype for signal/slot
    qRegisterMetaType<solver::SolveProgress>("solver::SolveProgress");
    
    updateDisplay();
}

MainWindow::~MainWindow() {
    if (solving_) {
        solver_->stop();
    }
}

void MainWindow::setupUI() {
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);
    
    auto* mainLayout = new QHBoxLayout(centralWidget_);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // === Left Panel: Setup ===
    QGroupBox* setupGroup = new QGroupBox("Setup", this);
    auto* setupLayout = new QVBoxLayout(setupGroup);
    setupLayout->setSpacing(10);
    
    // Stack size
    auto* stackLayout = new QHBoxLayout();
    stackLayout->addWidget(new QLabel("Stack Size (BB):", this));
    stackSizeSpinner_ = new QSpinBox(this);
    stackSizeSpinner_->setRange(10, 500);
    stackSizeSpinner_->setValue(100);
    stackLayout->addWidget(stackSizeSpinner_);
    stackLayout->addStretch();
    setupLayout->addLayout(stackLayout);
    
    // OOP Range
    setupLayout->addWidget(new QLabel("OOP Range (UTG):", this));
    oopRangeInput_ = new QLineEdit(this);
    oopRangeInput_->setPlaceholderText("e.g., 77+, ATs+, KQs, AJo+, KQo");
    setupLayout->addWidget(oopRangeInput_);
    
    // IP Range  
    setupLayout->addWidget(new QLabel("IP Range (BTN):", this));
    ipRangeInput_ = new QLineEdit(this);
    ipRangeInput_->setPlaceholderText("e.g., 66-TT, ATs-AQs, KQs, JTs, AQo");
    setupLayout->addWidget(ipRangeInput_);
    
    // Load defaults button
    loadDefaultsBtn_ = new QPushButton("Load Default Ranges", this);
    loadDefaultsBtn_->setStyleSheet(
        "QPushButton { background-color: #4a90d9; color: white; }"
        "QPushButton:hover { background-color: #5aa0e9; }"
    );
    setupLayout->addWidget(loadDefaultsBtn_);
    
    setupLayout->addSpacing(10);
    
    // Player selector for viewing
    auto* viewLayout = new QHBoxLayout();
    viewLayout->addWidget(new QLabel("View Strategy:", this));
    viewPlayerSelector_ = new QComboBox(this);
    viewPlayerSelector_->addItem("OOP (UTG)");
    viewPlayerSelector_->addItem("IP (BTN)");
    viewLayout->addWidget(viewPlayerSelector_);
    viewLayout->addStretch();
    setupLayout->addLayout(viewLayout);
    
    setupLayout->addStretch();
    
    // Solver controls
    QGroupBox* solverGroup = new QGroupBox("Solver", this);
    auto* solverLayout = new QVBoxLayout(solverGroup);
    
    auto* btnLayout = new QHBoxLayout();
    solveBtn_ = new QPushButton("Solve", this);
    solveBtn_->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; min-width: 80px; }"
        "QPushButton:hover { background-color: #2ecc71; }"
    );
    btnLayout->addWidget(solveBtn_);
    
    stopBtn_ = new QPushButton("Stop", this);
    stopBtn_->setEnabled(false);
    stopBtn_->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; }"
        "QPushButton:hover { background-color: #ec7063; }"
        "QPushButton:disabled { background-color: #5a5a6a; color: #888; }"
    );
    btnLayout->addWidget(stopBtn_);
    
    resetBtn_ = new QPushButton("Reset", this);
    btnLayout->addWidget(resetBtn_);
    
    solverLayout->addLayout(btnLayout);
    
    // Progress panel
    progressPanel_ = new ProgressPanel(this);
    solverLayout->addWidget(progressPanel_);
    
    setupLayout->addWidget(solverGroup);
    
    mainLayout->addWidget(setupGroup);
    
    // === Center Panel: Board and Actions ===
    QGroupBox* boardGroup = new QGroupBox("Board & Actions", this);
    auto* boardLayout = new QVBoxLayout(boardGroup);
    
    // Street and pot info
    auto* infoLayout = new QHBoxLayout();
    streetLabel_ = new QLabel("Street: Flop", this);
    streetLabel_->setStyleSheet("font-weight: bold; font-size: 14px;");
    infoLayout->addWidget(streetLabel_);
    infoLayout->addStretch();
    potLabel_ = new QLabel("Pot: 7.0 BB", this);
    potLabel_->setStyleSheet("font-weight: bold; font-size: 14px; color: #f39c12;");
    infoLayout->addWidget(potLabel_);
    boardLayout->addLayout(infoLayout);
    
    // Current player
    currentPlayerLabel_ = new QLabel("To Act: OOP", this);
    currentPlayerLabel_->setStyleSheet("font-size: 13px; color: #89b4fa;");
    boardLayout->addWidget(currentPlayerLabel_);
    
    boardLayout->addSpacing(10);
    
    // Board card selectors
    QLabel* boardLabel = new QLabel("Board Cards:", this);
    boardLabel->setStyleSheet("font-weight: bold;");
    boardLayout->addWidget(boardLabel);
    
    auto* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(5);
    
    // Flop cards
    QLabel* flopLabel = new QLabel("Flop:", this);
    cardsLayout->addWidget(flopLabel);
    
    flopSelector1_ = new CardSelector(this);
    flopSelector1_->setCompact(true);
    cardsLayout->addWidget(flopSelector1_);
    
    flopSelector2_ = new CardSelector(this);
    flopSelector2_->setCompact(true);
    cardsLayout->addWidget(flopSelector2_);
    
    flopSelector3_ = new CardSelector(this);
    flopSelector3_->setCompact(true);
    cardsLayout->addWidget(flopSelector3_);
    
    cardsLayout->addSpacing(15);
    
    // Turn
    QLabel* turnLabel = new QLabel("Turn:", this);
    cardsLayout->addWidget(turnLabel);
    turnSelector_ = new CardSelector(this);
    turnSelector_->setCompact(true);
    cardsLayout->addWidget(turnSelector_);
    
    cardsLayout->addSpacing(15);
    
    // River
    QLabel* riverLabel = new QLabel("River:", this);
    cardsLayout->addWidget(riverLabel);
    riverSelector_ = new CardSelector(this);
    riverSelector_->setCompact(true);
    cardsLayout->addWidget(riverSelector_);
    
    cardsLayout->addStretch();
    boardLayout->addLayout(cardsLayout);
    
    boardLayout->addSpacing(15);
    
    // Action panel
    actionPanel_ = new ActionPanel(this);
    boardLayout->addWidget(actionPanel_);
    
    // Undo button
    auto* undoLayout = new QHBoxLayout();
    undoBtn_ = new QPushButton("Undo", this);
    undoBtn_->setEnabled(false);
    undoLayout->addWidget(undoBtn_);
    undoLayout->addStretch();
    boardLayout->addLayout(undoLayout);
    
    boardLayout->addStretch();
    
    mainLayout->addWidget(boardGroup);
    
    // === Right Panel: Strategy Grid ===
    QGroupBox* strategyGroup = new QGroupBox("Strategy", this);
    auto* strategyLayout = new QVBoxLayout(strategyGroup);
    
    strategyGrid_ = new StrategyGrid(this);
    strategyLayout->addWidget(strategyGrid_);
    
    mainLayout->addWidget(strategyGroup);
}

void MainWindow::setupConnections() {
    // Load defaults
    connect(loadDefaultsBtn_, &QPushButton::clicked, this, &MainWindow::onLoadDefaultRanges);
    
    // Card selections
    connect(flopSelector1_, &CardSelector::cardSelected, this, [this](const core::Card&) {
        onFlopCardSelected(0);
    });
    connect(flopSelector2_, &CardSelector::cardSelected, this, [this](const core::Card&) {
        onFlopCardSelected(1);
    });
    connect(flopSelector3_, &CardSelector::cardSelected, this, [this](const core::Card&) {
        onFlopCardSelected(2);
    });
    connect(turnSelector_, &CardSelector::cardSelected, this, &MainWindow::onTurnCardSelected);
    connect(riverSelector_, &CardSelector::cardSelected, this, &MainWindow::onRiverCardSelected);
    
    // Actions
    connect(actionPanel_, &ActionPanel::actionSelected, this, &MainWindow::onActionSelected);
    connect(undoBtn_, &QPushButton::clicked, this, &MainWindow::onUndoClicked);
    
    // Solver controls
    connect(solveBtn_, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    
    // View selector
    connect(viewPlayerSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPlayerChanged);
    
    // Stack size
    connect(stackSizeSpinner_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onStackSizeChanged);
    
    // Progress signal
    connect(this, &MainWindow::solveProgressUpdated, this, &MainWindow::onSolveProgress);
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About GTO Solver",
            "GTO Poker Solver v1.0\n\n"
            "A Monte Carlo CFR-based solver for\n"
            "heads-up poker game theory analysis.");
    });
}

void MainWindow::onLoadDefaultRanges() {
    oopRangeInput_->setText(QString::fromStdString(core::DefaultRanges::UTG_OPEN));
    ipRangeInput_->setText(QString::fromStdString(core::DefaultRanges::BTN_CALL_VS_UTG));
    
    oopRange_ = core::Range::fromString(core::DefaultRanges::UTG_OPEN);
    ipRange_ = core::Range::fromString(core::DefaultRanges::BTN_CALL_VS_UTG);
    
    progressPanel_->log("Loaded default ranges: UTG vs BTN");
    updateDisplay();
}

void MainWindow::onFlopCardSelected(int index) {
    // Update dead cards for other selectors
    std::vector<core::Card> dead;
    if (flopSelector1_->selectedCard()) dead.push_back(*flopSelector1_->selectedCard());
    if (flopSelector2_->selectedCard()) dead.push_back(*flopSelector2_->selectedCard());
    if (flopSelector3_->selectedCard()) dead.push_back(*flopSelector3_->selectedCard());
    
    flopSelector1_->setDisabledCards(dead);
    flopSelector2_->setDisabledCards(dead);
    flopSelector3_->setDisabledCards(dead);
    turnSelector_->setDisabledCards(dead);
    riverSelector_->setDisabledCards(dead);
    
    // Check if all flop cards are selected
    if (flopSelector1_->selectedCard() && 
        flopSelector2_->selectedCard() && 
        flopSelector3_->selectedCard()) {
        
        gameState_.setFlop(*flopSelector1_->selectedCard(),
                           *flopSelector2_->selectedCard(),
                           *flopSelector3_->selectedCard());
        
        progressPanel_->log(QString("Flop: %1 %2 %3")
            .arg(QString::fromStdString(flopSelector1_->selectedCard()->toString()))
            .arg(QString::fromStdString(flopSelector2_->selectedCard()->toString()))
            .arg(QString::fromStdString(flopSelector3_->selectedCard()->toString())));
    }
    
    updateDisplay();
}

void MainWindow::onTurnCardSelected() {
    if (turnSelector_->selectedCard()) {
        gameState_.setTurn(*turnSelector_->selectedCard());
        
        // Update dead cards
        std::vector<core::Card> dead;
        if (flopSelector1_->selectedCard()) dead.push_back(*flopSelector1_->selectedCard());
        if (flopSelector2_->selectedCard()) dead.push_back(*flopSelector2_->selectedCard());
        if (flopSelector3_->selectedCard()) dead.push_back(*flopSelector3_->selectedCard());
        dead.push_back(*turnSelector_->selectedCard());
        
        riverSelector_->setDisabledCards(dead);
        
        progressPanel_->log(QString("Turn: %1")
            .arg(QString::fromStdString(turnSelector_->selectedCard()->toString())));
    }
    
    updateDisplay();
}

void MainWindow::onRiverCardSelected() {
    if (riverSelector_->selectedCard()) {
        gameState_.setRiver(*riverSelector_->selectedCard());
        
        progressPanel_->log(QString("River: %1")
            .arg(QString::fromStdString(riverSelector_->selectedCard()->toString())));
    }
    
    updateDisplay();
}

void MainWindow::onActionSelected(int actionIndex) {
    // Don't allow actions while solving
    if (solving_) {
        progressPanel_->log("Please wait for solver to complete before selecting an action.");
        return;
    }
    
    auto actions = gameState_.getAvailableActions();
    if (actionIndex >= 0 && actionIndex < static_cast<int>(actions.size())) {
        solver::Action selectedAction = actions[actionIndex];
        solver::Position actingPlayer = gameState_.currentPlayer();
        
        // Narrow range based on current solver results BEFORE applying action
        // (The solver has strategies for the current state)
        if (solver_ && solver_->currentIteration() > 0) {
            narrowRangeAfterAction(actingPlayer, selectedAction);
        }
        
        // Apply the action
        gameState_.applyAction(selectedAction);
        
        progressPanel_->log(QString("%1: %2")
            .arg(actingPlayer == solver::Position::OOP ? "OOP" : "IP")
            .arg(QString::fromStdString(selectedAction.toString())));
        
        // Update display to show new state
        updateDisplay();
        
        // Block UI while solving
        enableUIForSolving(true);
        
        // Automatically solve progressively after each action
        // Solve whenever we're at a decision point (not terminal)
        if (gameState_.isTerminal()) {
            progressPanel_->log("Hand complete. No further solving needed.");
            enableUIForSolving(false);
        } else {
            // Switch view to current player (the one who needs to act next)
            int playerIndex = (gameState_.currentPlayer() == solver::Position::OOP) ? 0 : 1;
            viewPlayerSelector_->setCurrentIndex(playerIndex);
            
            progressPanel_->log(QString("Auto-solving for %1's decision...")
                .arg(gameState_.currentPlayer() == solver::Position::OOP ? "OOP" : "IP"));
            
            // Start solving for the current decision point
            // Parse ranges if needed
            if (oopRange_.totalCombos() == 0 || ipRange_.totalCombos() == 0) {
                oopRange_ = core::Range::fromString(oopRangeInput_->text().toStdString());
                ipRange_ = core::Range::fromString(ipRangeInput_->text().toStdString());
            }
            
            // Initialize and start solver
            solver::MCCFRConfig config;
            config.numIterations = 3000;  // Fewer iterations for progressive solving
            config.progressCallbackFrequency = 50;
            
            solver_ = std::make_unique<solver::MCCFRSolver>(config);
            solver_->initialize(gameState_, oopRange_, ipRange_);
            
            solver_->setProgressCallback([this](const solver::SolveProgress& progress) {
                emit solveProgressUpdated(progress.currentIteration, 
                                          progress.totalIterations, 
                                          progress.exploitability);
            });
            
            solving_ = true;
            solveBtn_->setEnabled(false);
            stopBtn_->setEnabled(true);
            progressPanel_->setStatus("Solving...");
            
            solverTimer_->start(10);
            runSolver();
        }
    } else {
        updateDisplay();
    }
}

void MainWindow::onSolveClicked() {
    // Parse ranges
    oopRange_ = core::Range::fromString(oopRangeInput_->text().toStdString());
    ipRange_ = core::Range::fromString(ipRangeInput_->text().toStdString());
    
    if (oopRange_.totalCombos() == 0 || ipRange_.totalCombos() == 0) {
        QMessageBox::warning(this, "Error", "Please enter valid ranges for both players.");
        return;
    }
    
    if (gameState_.board().size() < 3) {
        QMessageBox::warning(this, "Error", "Please select all flop cards before solving.");
        return;
    }
    
    solving_ = true;
    solveBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    
    // Block UI while solving
    enableUIForSolving(true);
    
    progressPanel_->log(QString("Starting solver with %1 OOP combos, %2 IP combos...")
        .arg(oopRange_.totalCombos(), 0, 'f', 1)
        .arg(ipRange_.totalCombos(), 0, 'f', 1));
    progressPanel_->setStatus("Solving...");
    
    // Initialize solver
    solver::MCCFRConfig config;
    config.numIterations = 5000;
    config.progressCallbackFrequency = 50;
    
    solver_ = std::make_unique<solver::MCCFRSolver>(config);
    solver_->initialize(gameState_, oopRange_, ipRange_);
    
    progressPanel_->log("Solver initialized. Starting iterations...");
    
    // Set up progress callback
    solver_->setProgressCallback([this](const solver::SolveProgress& progress) {
        emit solveProgressUpdated(progress.currentIteration, 
                                  progress.totalIterations, 
                                  progress.exploitability);
    });
    
    // Run solver in iterations using timer (10ms = 100Hz update rate)
    solverTimer_->start(10);
    runSolver();
}

void MainWindow::runSolver() {
    if (!solving_ || !solver_ || solver_->isStopped()) {
        solverTimer_->stop();
        solving_ = false;
        solveBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        progressPanel_->setStatus("Complete");
        progressPanel_->log("Solving complete.");
        updateStrategyDisplay();
        
        // Re-enable UI
        enableUIForSolving(false);
        return;
    }
    
    // Run a batch of iterations
    int batchSize = 10;
    for (int i = 0; i < batchSize && !solver_->isStopped(); ++i) {
        solver_->runIteration();
    }
    
    // Update progress
    int currentIter = solver_->currentIteration();
    int totalIter = solver_->config().numIterations;
    double exploitability = solver_->getExploitability();
    
    progressPanel_->setProgress(currentIter, totalIter);
    progressPanel_->setExploitability(exploitability);
    
    // Periodically update strategy display
    if (currentIter % 500 == 0 || currentIter == totalIter) {
        updateStrategyDisplay();
    }
    
    // Check if complete
    if (currentIter >= totalIter) {
        solverTimer_->stop();
        solving_ = false;
        solveBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        progressPanel_->setStatus("Complete");
        progressPanel_->log(QString("Solving complete after %1 iterations.").arg(currentIter));
        updateStrategyDisplay();
        
        // Re-enable UI now that solving is complete
        enableUIForSolving(false);
        
        // Check if street is complete and advance if needed
        handleStreetCompletion();
    }
}

void MainWindow::onStopClicked() {
    if (solver_) {
        solver_->stop();
    }
    solving_ = false;
    solverTimer_->stop();
    solveBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    progressPanel_->setStatus("Stopped");
    progressPanel_->log("Solving stopped by user.");
    
    // Re-enable UI
    enableUIForSolving(false);
}

void MainWindow::onResetClicked() {
    // Reset game state
    solver::BetSizingConfig config;
    config.stackSize = stackSizeSpinner_->value();
    gameState_ = solver::GameState(config);
    
    // Clear card selections
    flopSelector1_->clearCard();
    flopSelector2_->clearCard();
    flopSelector3_->clearCard();
    turnSelector_->clearCard();
    riverSelector_->clearCard();
    
    flopSelector1_->enableAllCards();
    flopSelector2_->enableAllCards();
    flopSelector3_->enableAllCards();
    turnSelector_->enableAllCards();
    riverSelector_->enableAllCards();
    
    // Reset solver
    if (solver_) {
        solver_->reset();
    }
    
    // Reset UI
    progressPanel_->reset();
    progressPanel_->log("Reset complete.");
    strategyGrid_->clear();
    
    updateDisplay();
}

void MainWindow::onPlayerChanged(int index) {
    updateStrategyDisplay();
}

void MainWindow::onStreetChanged(int index) {
    updateDisplay();
}

void MainWindow::onStackSizeChanged(int value) {
    gameState_.setStackSize(value);
    updateDisplay();
}

void MainWindow::onUndoClicked() {
    gameState_.undo();
    updateDisplay();
}

void MainWindow::updateDisplay() {
    // Update street label
    streetLabel_->setText(QString("Street: %1")
        .arg(solver::streetToString(gameState_.currentStreet())));
    
    // Update pot
    potLabel_->setText(QString("Pot: %1 BB").arg(gameState_.pot(), 0, 'f', 1));
    
    // Update current player
    currentPlayerLabel_->setText(QString("To Act: %1")
        .arg(solver::positionToString(gameState_.currentPlayer())));
    
    // Update available actions
    auto actions = gameState_.getAvailableActions();
    actionPanel_->setActions(actions);
    
    // Enable/disable undo
    undoBtn_->setEnabled(gameState_.canUndo());
}

void MainWindow::onSolveProgress(int iteration, int total, double exploitability) {
    progressPanel_->setProgress(iteration, total);
    progressPanel_->setExploitability(exploitability);
    
    // Periodically update strategy display
    if (iteration % 500 == 0 || iteration == total) {
        updateStrategyDisplay();
    }
}

void MainWindow::updateBoardDisplay() {
    // Board display is handled by card selectors
}

void MainWindow::updateActionHistory() {
    // Action history is shown in the log
}

void MainWindow::updateStrategyDisplay() {
    if (!solver_ || solver_->currentIteration() == 0) {
        return;
    }
    
    solver::Position viewPlayer = (viewPlayerSelector_->currentIndex() == 0) ? 
                                   solver::Position::OOP : solver::Position::IP;
    
    auto strategies = solver_->getAllStrategies(viewPlayer);
    
    // Get action names and pass actions to grid for color mapping
    auto actions = gameState_.getAvailableActions();
    strategyGrid_->setAvailableActions(actions);
    
    std::vector<std::string> actionNames;
    for (const auto& action : actions) {
        actionNames.push_back(action.toString());
    }
    
    // Collect individual hand strategies for detailed tooltips
    std::map<std::string, std::vector<solver::NodeStrategy>> handStrategies;
    const core::Range& range = (viewPlayer == solver::Position::OOP) ? oopRange_ : ipRange_;
    
    // Get dead cards (board)
    std::vector<core::Card> deadCards = gameState_.board();
    
    // For each hand type, get strategies for all individual hands
    for (const auto& [handType, weight] : range.getHandTypes()) {
        if (weight <= 0) continue;
        
        std::string handTypeStr = handType.toString();
        std::vector<solver::NodeStrategy> handsForType;
        
        // Get all hands of this type
        auto hands = handType.getHands();
        for (const auto& hand : hands) {
            // Check if hand conflicts with board
            bool conflicts = false;
            for (const auto& boardCard : deadCards) {
                if (hand.contains(boardCard)) {
                    conflicts = true;
                    break;
                }
            }
            
            if (!conflicts) {
                // Get strategy for this specific hand
                auto handStrat = solver_->getStrategy(viewPlayer, hand);
                // Store the actual hand string (e.g., "AsKs") instead of canonical name
                handStrat.handType = hand.toString();
                handsForType.push_back(handStrat);
            }
        }
        
        if (!handsForType.empty()) {
            handStrategies[handTypeStr] = handsForType;
        }
    }
    
    // Use the enhanced method that includes per-hand data
    strategyGrid_->setStrategyWithHands(strategies, handStrategies, actionNames);
}

void MainWindow::narrowRangeAfterAction(solver::Position player, const solver::Action& action) {
    if (!solver_ || solver_->currentIteration() == 0) {
        return;  // Can't narrow without solved strategy
    }
    
    core::Range& range = (player == solver::Position::OOP) ? oopRange_ : ipRange_;
    core::Range narrowedRange;
    
    // Get strategy for this player
    auto strategies = solver_->getAllStrategies(player);
    
    // Find the action index that matches the selected action
    auto availableActions = gameState_.getAvailableActions();
    int actionIndex = -1;
    for (size_t i = 0; i < availableActions.size(); ++i) {
        if (availableActions[i].type == action.type) {
            // For bets, also check pot fraction
            if (action.type == solver::ActionType::BET) {
                if (std::abs(availableActions[i].potFraction - action.potFraction) < 0.01) {
                    actionIndex = static_cast<int>(i);
                    break;
                }
            } else {
                actionIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    if (actionIndex < 0) {
        return;  // Couldn't find matching action
    }
    
    // Threshold for keeping hands (hands that take this action >5% of the time)
    const double threshold = 0.05;
    
    // Go through all hand types in the range
    for (const auto& [handType, weight] : range.getHandTypes()) {
        if (weight <= 0) continue;
        
        // Find strategy for this hand type
        auto handTypeStr = handType.toString();
        double actionProb = 0.0;
        
        for (const auto& strat : strategies) {
            if (strat.handType == handTypeStr && 
                actionIndex < static_cast<int>(strat.actionProbabilities.size())) {
                actionProb = strat.actionProbabilities[actionIndex];
                break;
            }
        }
        
        // Keep hand if it takes this action with sufficient frequency
        if (actionProb >= threshold) {
            // Scale weight by action probability
            narrowedRange.addHandType(handType, weight * actionProb);
        }
    }
    
    // Update the range
    if (player == solver::Position::OOP) {
        oopRange_ = narrowedRange;
        progressPanel_->log(QString("Narrowed OOP range: %1 combos remaining")
            .arg(oopRange_.totalCombos(), 0, 'f', 1));
    } else {
        ipRange_ = narrowedRange;
        progressPanel_->log(QString("Narrowed IP range: %1 combos remaining")
            .arg(ipRange_.totalCombos(), 0, 'f', 1));
    }
}

void MainWindow::enableUIForSolving(bool enable) {
    // Block action panel when solving
    actionPanel_->setActionsEnabled(!enable);
    
    if (enable) {
        // Disable all card selectors while solving
        flopSelector1_->setEnabled(false);
        flopSelector2_->setEnabled(false);
        flopSelector3_->setEnabled(false);
        turnSelector_->setEnabled(false);
        riverSelector_->setEnabled(false);
    } else {
        // Re-enable based on current state
        int boardSize = static_cast<int>(gameState_.board().size());
        solver::Street currentStreet = gameState_.currentStreet();
        
        // Flop selectors: enabled if flop not complete
        bool canSelectFlop = boardSize < 3;
        flopSelector1_->setEnabled(canSelectFlop);
        flopSelector2_->setEnabled(canSelectFlop);
        flopSelector3_->setEnabled(canSelectFlop);
        
        // Turn selector: enabled if flop complete (3 cards) and we're at turn street
        // But only if turn card not yet selected
        bool canSelectTurn = boardSize == 3 && currentStreet >= solver::Street::TURN;
        turnSelector_->setEnabled(canSelectTurn);
        
        // River selector: enabled if turn complete (4 cards) and we're at river street
        // But only if river card not yet selected
        bool canSelectRiver = boardSize == 4 && currentStreet >= solver::Street::RIVER;
        riverSelector_->setEnabled(canSelectRiver);
    }
}

void MainWindow::handleStreetCompletion() {
    // Check if we need to advance to the next street
    solver::Street currentStreet = gameState_.currentStreet();
    int boardSize = static_cast<int>(gameState_.board().size());
    
    // If flop actions are complete, enable turn selector
    if (currentStreet == solver::Street::FLOP && boardSize == 3) {
        // Check if action sequence is complete
        const auto& history = gameState_.actionHistory();
        bool streetComplete = false;
        
        if (gameState_.isTerminal()) {
            // Hand is over, don't advance
        } else if (!history.empty()) {
            // Check if last action completed the street
            auto lastAction = history.back();
            if (lastAction.type == solver::ActionType::CALL) {
                streetComplete = true;
            } else if (lastAction.type == solver::ActionType::CHECK && history.size() >= 2) {
                // Both checked - check if previous was also check
                if (history.size() >= 2 && history[history.size() - 2].type == solver::ActionType::CHECK) {
                    streetComplete = true;
                }
            }
        }
        
        if (streetComplete) {
            progressPanel_->log("Flop complete. Ready for turn card selection.");
            // Enable turn selector (UI blocking is already handled by enableUIForSolving)
            // The selector will be enabled when solving completes
        }
    }
    
    // Similar for turn -> river
    if (currentStreet == solver::Street::TURN && boardSize == 4) {
        const auto& history = gameState_.actionHistory();
        bool streetComplete = false;
        
        if (!gameState_.isTerminal() && !history.empty()) {
            auto lastAction = history.back();
            if (lastAction.type == solver::ActionType::CALL) {
                streetComplete = true;
            } else if (lastAction.type == solver::ActionType::CHECK && history.size() >= 2) {
                if (history.size() >= 2 && history[history.size() - 2].type == solver::ActionType::CHECK) {
                    streetComplete = true;
                }
            }
        }
        
        if (streetComplete) {
            progressPanel_->log("Turn complete. Ready for river card selection.");
        }
    }
    
    // Re-enable UI with updated state (this will enable turn/river selectors if appropriate)
    enableUIForSolving(false);
}

} // namespace gui
