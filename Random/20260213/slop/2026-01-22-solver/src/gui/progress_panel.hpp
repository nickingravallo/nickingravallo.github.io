#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QTextEdit>

namespace gui {

/**
 * Panel showing solver progress and log output.
 */
class ProgressPanel : public QWidget {
    Q_OBJECT

public:
    explicit ProgressPanel(QWidget* parent = nullptr);
    
    // Update progress
    void setProgress(int current, int total);
    void setExploitability(double value);
    void setStatus(const QString& status);
    
    // Log output
    void log(const QString& message);
    void clearLog();
    
    // Reset all
    void reset();

private:
    void setupUI();
    
    QProgressBar* progressBar_;
    QLabel* iterationLabel_;
    QLabel* exploitabilityLabel_;
    QLabel* statusLabel_;
    QTextEdit* logOutput_;
};

} // namespace gui
