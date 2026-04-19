#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <optional>
#include <set>
#include "core/card.hpp"

namespace gui {

/**
 * Widget for selecting a playing card from a visual grid.
 * Shows all 52 cards organized by rank and suit.
 */
class CardSelector : public QWidget {
    Q_OBJECT

public:
    explicit CardSelector(QWidget* parent = nullptr);
    
    // Set the selected card
    void setCard(const core::Card& card);
    void clearCard();
    
    // Get current selection
    std::optional<core::Card> selectedCard() const { return selectedCard_; }
    
    // Disable certain cards (e.g., already used)
    void setDisabledCards(const std::vector<core::Card>& cards);
    void enableAllCards();
    
    // Visual state
    void setCompact(bool compact);
    bool isCompact() const { return compact_; }

signals:
    void cardSelected(const core::Card& card);
    void cardCleared();

private slots:
    void onCardButtonClicked();

private:
    void setupUI();
    void updateButtonStyles();
    QString getCardStyle(const core::Card& card, bool selected, bool disabled) const;
    QString getSuitColor(core::Suit suit) const;
    
    QGridLayout* gridLayout_;
    QLabel* selectedLabel_;
    std::array<QPushButton*, 52> cardButtons_;
    std::optional<core::Card> selectedCard_;
    std::set<int> disabledCards_;
    bool compact_ = false;
};

/**
 * A simple card display widget (non-interactive)
 */
class CardDisplay : public QWidget {
    Q_OBJECT

public:
    explicit CardDisplay(QWidget* parent = nullptr);
    
    void setCard(const core::Card& card);
    void clearCard();
    void setSize(int width, int height);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::optional<core::Card> card_;
    int width_ = 50;
    int height_ = 70;
};

} // namespace gui
