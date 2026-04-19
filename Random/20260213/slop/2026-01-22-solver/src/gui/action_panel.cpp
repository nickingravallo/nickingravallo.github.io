#include "action_panel.hpp"
#include <QFont>

namespace gui {

ActionPanel::ActionPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ActionPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    titleLabel_ = new QLabel("Actions", this);
    titleLabel_->setStyleSheet("font-weight: bold; font-size: 14px; color: #aaa;");
    mainLayout->addWidget(titleLabel_);
    
    layout_ = new QHBoxLayout();
    layout_->setSpacing(8);
    mainLayout->addLayout(layout_);
    
    mainLayout->addStretch();
}

void ActionPanel::setActions(const std::vector<solver::Action>& actions) {
    // Clear existing buttons
    clear();
    
    currentActions_ = actions;
    
    for (size_t i = 0; i < actions.size(); ++i) {
        QPushButton* btn = new QPushButton(this);
        btn->setText(QString::fromStdString(actions[i].toString()));
        btn->setProperty("actionIndex", static_cast<int>(i));
        btn->setStyleSheet(getActionStyle(actions[i]));
        btn->setMinimumWidth(80);
        btn->setMinimumHeight(40);
        
        QFont font = btn->font();
        font.setBold(true);
        btn->setFont(font);
        
        connect(btn, &QPushButton::clicked, this, &ActionPanel::onActionClicked);
        
        actionButtons_.push_back(btn);
        layout_->addWidget(btn);
    }
}

void ActionPanel::clear() {
    for (auto* btn : actionButtons_) {
        layout_->removeWidget(btn);
        delete btn;
    }
    actionButtons_.clear();
    currentActions_.clear();
}

void ActionPanel::setActionsEnabled(bool enabled) {
    for (auto* btn : actionButtons_) {
        btn->setEnabled(enabled);
    }
}

void ActionPanel::onActionClicked() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int index = btn->property("actionIndex").toInt();
    emit actionSelected(index);
}

QString ActionPanel::getActionStyle(const solver::Action& action) const {
    QColor bgColor = getActionColor(action.type);
    QColor hoverColor = bgColor.lighter(120);
    
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #3a3a4a;"
        "  color: #666;"
        "}"
    ).arg(bgColor.name(), hoverColor.name());
}

QColor ActionPanel::getActionColor(solver::ActionType type) const {
    switch (type) {
        case solver::ActionType::FOLD:
            return QColor("#3498db");  // Blue
        case solver::ActionType::CHECK:
            return QColor("#27ae60");  // Green
        case solver::ActionType::CALL:
            return QColor("#27ae60");  // Green
        case solver::ActionType::BET:
            return QColor("#e74c3c");  // Red
        case solver::ActionType::RAISE:
            return QColor("#c0392b");  // Dark red
        case solver::ActionType::ALL_IN:
            return QColor("#8e44ad");  // Purple
        default:
            return QColor("#7f8c8d");  // Gray
    }
}

} // namespace gui
