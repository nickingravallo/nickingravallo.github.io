#include "card_selector.hpp"
#include <QPainter>
#include <QFont>

namespace gui {

CardSelector::CardSelector(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void CardSelector::setupUI() {
    gridLayout_ = new QGridLayout(this);
    gridLayout_->setSpacing(2);
    gridLayout_->setContentsMargins(5, 5, 5, 5);
    
    // Create header row (rank labels)
    const char* ranks = "A K Q J T 9 8 7 6 5 4 3 2";
    QStringList rankList = QString(ranks).split(' ');
    
    for (int r = 0; r < 13; ++r) {
        QLabel* label = new QLabel(rankList[12 - r], this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight: bold; color: #888;");
        gridLayout_->addWidget(label, 0, r + 1);
    }
    
    // Create suit labels and card buttons
    const char* suitSymbols[] = {"♠", "♥", "♦", "♣"};
    const char* suitColors[] = {"#1a1a2e", "#e74c3c", "#3498db", "#27ae60"};
    
    for (int s = 3; s >= 0; --s) {  // Spades, Hearts, Diamonds, Clubs
        int row = 4 - s;  // Spades at top
        
        QLabel* suitLabel = new QLabel(suitSymbols[s], this);
        suitLabel->setAlignment(Qt::AlignCenter);
        suitLabel->setStyleSheet(QString("font-size: 16px; color: %1;").arg(suitColors[s]));
        gridLayout_->addWidget(suitLabel, row, 0);
        
        for (int r = 12; r >= 0; --r) {  // Ace to 2
            int col = 13 - r;
            int cardValue = s * 13 + r;
            
            QPushButton* btn = new QPushButton(this);
            btn->setFixedSize(compact_ ? 28 : 36, compact_ ? 28 : 36);
            btn->setProperty("cardValue", cardValue);
            btn->setText(QString(core::RANK_CHARS[r]));
            btn->setStyleSheet(getCardStyle(core::Card(cardValue), false, false));
            
            connect(btn, &QPushButton::clicked, this, &CardSelector::onCardButtonClicked);
            
            cardButtons_[cardValue] = btn;
            gridLayout_->addWidget(btn, row, col);
        }
    }
    
    // Selected card display
    selectedLabel_ = new QLabel("Select a card", this);
    selectedLabel_->setAlignment(Qt::AlignCenter);
    selectedLabel_->setStyleSheet("font-size: 14px; color: #666; margin-top: 10px;");
    gridLayout_->addWidget(selectedLabel_, 5, 0, 1, 14);
}

void CardSelector::setCard(const core::Card& card) {
    if (card.isValid()) {
        selectedCard_ = card;
        selectedLabel_->setText(QString::fromStdString(card.toString()));
        selectedLabel_->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
                                     .arg(getSuitColor(card.suit())));
        updateButtonStyles();
        emit cardSelected(card);
    }
}

void CardSelector::clearCard() {
    selectedCard_.reset();
    selectedLabel_->setText("Select a card");
    selectedLabel_->setStyleSheet("font-size: 14px; color: #666;");
    updateButtonStyles();
    emit cardCleared();
}

void CardSelector::setDisabledCards(const std::vector<core::Card>& cards) {
    disabledCards_.clear();
    for (const auto& card : cards) {
        if (card.isValid()) {
            disabledCards_.insert(card.value());
        }
    }
    updateButtonStyles();
}

void CardSelector::enableAllCards() {
    disabledCards_.clear();
    updateButtonStyles();
}

void CardSelector::setCompact(bool compact) {
    compact_ = compact;
    int size = compact ? 28 : 36;
    for (auto* btn : cardButtons_) {
        if (btn) btn->setFixedSize(size, size);
    }
}

void CardSelector::onCardButtonClicked() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int cardValue = btn->property("cardValue").toInt();
    if (disabledCards_.count(cardValue)) return;
    
    core::Card card(cardValue);
    setCard(card);
}

void CardSelector::updateButtonStyles() {
    for (int i = 0; i < 52; ++i) {
        if (cardButtons_[i]) {
            core::Card card(i);
            bool selected = selectedCard_ && selectedCard_->value() == i;
            bool disabled = disabledCards_.count(i) > 0;
            cardButtons_[i]->setStyleSheet(getCardStyle(card, selected, disabled));
            cardButtons_[i]->setEnabled(!disabled);
        }
    }
}

QString CardSelector::getCardStyle(const core::Card& card, bool selected, bool disabled) const {
    QString bgColor, textColor, borderColor;
    
    if (disabled) {
        bgColor = "#2a2a2a";
        textColor = "#555";
        borderColor = "#333";
    } else if (selected) {
        bgColor = "#4a90d9";
        textColor = "#fff";
        borderColor = "#2e6da4";
    } else {
        bgColor = "#3a3a4a";
        textColor = getSuitColor(card.suit());
        borderColor = "#4a4a5a";
    }
    
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 2px solid %3;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}"
    ).arg(bgColor, textColor, borderColor, selected ? "#5aa0e9" : "#4a4a5a");
}

QString CardSelector::getSuitColor(core::Suit suit) const {
    switch (suit) {
        case core::Suit::SPADES: return "#f0f0f0";
        case core::Suit::HEARTS: return "#e74c3c";
        case core::Suit::DIAMONDS: return "#3498db";
        case core::Suit::CLUBS: return "#2ecc71";
        default: return "#fff";
    }
}

// CardDisplay implementation

CardDisplay::CardDisplay(QWidget* parent) : QWidget(parent) {
    setFixedSize(width_, height_);
}

void CardDisplay::setCard(const core::Card& card) {
    if (card.isValid()) {
        card_ = card;
        update();
    }
}

void CardDisplay::clearCard() {
    card_.reset();
    update();
}

void CardDisplay::setSize(int width, int height) {
    width_ = width;
    height_ = height;
    setFixedSize(width_, height_);
    update();
}

void CardDisplay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Background
    painter.setBrush(card_ ? QColor("#e8e8e8") : QColor("#3a3a4a"));
    painter.setPen(QPen(QColor("#555"), 2));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    
    if (card_) {
        // Card text
        QFont font("Arial", height_ / 3, QFont::Bold);
        painter.setFont(font);
        
        QColor suitColor;
        switch (card_->suit()) {
            case core::Suit::HEARTS:
            case core::Suit::DIAMONDS:
                suitColor = QColor("#c0392b");
                break;
            default:
                suitColor = QColor("#2c3e50");
                break;
        }
        painter.setPen(suitColor);
        
        QString text = QString::fromStdString(card_->toString());
        painter.drawText(rect(), Qt::AlignCenter, text);
    } else {
        // Empty card back pattern
        painter.setPen(QColor("#555"));
        painter.drawText(rect(), Qt::AlignCenter, "?");
    }
}

} // namespace gui
