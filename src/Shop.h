#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"
#include "GameState.h"

// ─────────────────────────────────────────────────────────────
//  Shop categories
// ─────────────────────────────────────────────────────────────
enum class ShopCategory {
    WEAPONS = 0,
    MINING,
    PLINKO,
    ECONOMY,
    CATEGORY_COUNT
};

// ─────────────────────────────────────────────────────────────
//  Upgrade card (UI element)
// ─────────────────────────────────────────────────────────────
struct UpgradeCard {
    UpgradeID     id;
    sf::FloatRect bounds;
    bool          hovered   = false;
    bool          affordable= false;
};

// ─────────────────────────────────────────────────────────────
//  Shop
// ─────────────────────────────────────────────────────────────
class Shop {
public:
    Shop();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH);

    /// Returns true if a purchase was made
    bool handleEvent(const sf::Event& event,
                     GameState&       state);

    void update(sf::Vector2f mousePos,
                const GameState& state);

    void draw(sf::RenderTarget& target,
              const GameState&  state) const;

    void scrollBy(float delta);

private:
    sf::Font* m_font   = nullptr;
    float     m_x      = 0.f;
    float     m_y      = 0.f;
    float     m_w      = 0.f;
    float     m_h      = 0.f;
    float     m_scroll    = 0.f;
    float     m_maxScroll = 0.f;

    ShopCategory             m_activeCategory = ShopCategory::WEAPONS;
    std::vector<UpgradeCard> m_cards;

    void buildCards(const GameState& state);
    void drawBackground(sf::RenderTarget& target) const;
    void drawCategoryTabs(sf::RenderTarget& target,
                          const GameState&  state) const;
    void drawCard(sf::RenderTarget&  target,
                  const UpgradeCard& card,
                  const GameState&   state) const;
    void drawScrollBar(sf::RenderTarget& target) const;

    std::string   formatEffect(UpgradeID id,
                               const GameState& state) const;
    sf::FloatRect tabBounds(int idx) const;

    static constexpr float TAB_H       = 36.f;
    static constexpr float CARD_H      = 82.f;
    static constexpr float CARD_MARGIN = 8.f;
    static constexpr float CARD_PAD    = 10.f;
    static constexpr float SCROLLBAR_W = 8.f;
};
