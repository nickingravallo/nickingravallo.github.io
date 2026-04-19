#include "progress_panel.hpp"
#include <QDateTime>
#include <QScrollBar>

namespace gui {

ProgressPanel::ProgressPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ProgressPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    
    // Title
    QLabel* title = new QLabel("Solver Progress", this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; color: #ddd;");
    layout->addWidget(title);
    
    // Progress bar
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #3a3a4a;"
        "  border-radius: 4px;"
        "  background-color: #2a2a3a;"
        "  height: 20px;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #4a90d9;"
        "  border-radius: 3px;"
        "}"
    );
    layout->addWidget(progressBar_);
    
    // Stats row
    auto* statsLayout = new QHBoxLayout();
    
    iterationLabel_ = new QLabel("Iteration: 0 / 0", this);
    iterationLabel_->setStyleSheet("color: #888; font-size: 11px;");
    statsLayout->addWidget(iterationLabel_);
    
    statsLayout->addStretch();
    
    exploitabilityLabel_ = new QLabel("Exploitability: --", this);
    exploitabilityLabel_->setStyleSheet("color: #888; font-size: 11px;");
    statsLayout->addWidget(exploitabilityLabel_);
    
    layout->addLayout(statsLayout);
    
    // Status
    statusLabel_ = new QLabel("Ready", this);
    statusLabel_->setStyleSheet("color: #27ae60; font-size: 12px; font-weight: bold;");
    layout->addWidget(statusLabel_);
    
    // Log output
    QLabel* logTitle = new QLabel("Log", this);
    logTitle->setStyleSheet("font-weight: bold; font-size: 12px; color: #aaa; margin-top: 8px;");
    layout->addWidget(logTitle);
    
    logOutput_ = new QTextEdit(this);
    logOutput_->setReadOnly(true);
    logOutput_->setMaximumHeight(120);
    logOutput_->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1a1a2a;"
        "  color: #888;"
        "  border: 1px solid #2a2a3a;"
        "  border-radius: 4px;"
        "  font-family: 'Consolas', 'Monaco', monospace;"
        "  font-size: 10px;"
        "  padding: 4px;"
        "}"
    );
    layout->addWidget(logOutput_);
}

void ProgressPanel::setProgress(int current, int total) {
    if (total > 0) {
        progressBar_->setMaximum(total);
        progressBar_->setValue(current);
        iterationLabel_->setText(QString("Iteration: %1 / %2").arg(current).arg(total));
    } else {
        progressBar_->setValue(0);
        iterationLabel_->setText("Iteration: 0 / 0");
    }
}

void ProgressPanel::setExploitability(double value) {
    if (value >= 0) {
        exploitabilityLabel_->setText(QString("Exploitability: %1 mbb/hand")
                                     .arg(value, 0, 'f', 2));
    } else {
        exploitabilityLabel_->setText("Exploitability: --");
    }
}

void ProgressPanel::setStatus(const QString& status) {
    statusLabel_->setText(status);
    
    // Color based on status
    if (status.contains("Complete") || status.contains("Ready")) {
        statusLabel_->setStyleSheet("color: #27ae60; font-size: 12px; font-weight: bold;");
    } else if (status.contains("Error")) {
        statusLabel_->setStyleSheet("color: #e74c3c; font-size: 12px; font-weight: bold;");
    } else {
        statusLabel_->setStyleSheet("color: #f39c12; font-size: 12px; font-weight: bold;");
    }
}

void ProgressPanel::log(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    logOutput_->append(QString("[%1] %2").arg(timestamp, message));
    
    // Auto-scroll to bottom
    logOutput_->verticalScrollBar()->setValue(
        logOutput_->verticalScrollBar()->maximum());
}

void ProgressPanel::clearLog() {
    logOutput_->clear();
}

void ProgressPanel::reset() {
    setProgress(0, 0);
    setExploitability(-1);
    setStatus("Ready");
}

} // namespace gui
