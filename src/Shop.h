#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"
#include "GameState.h"

// ─────────────────────────────────────────────────────────────
//  Category tab in the shop
// ─────────────────────────────────────────────────────────────
enum class ShopCategory {
    WEAPONS = 0,
    MINING,
    PLINKO,
    ECONOMY,
    CATEGORY_COUNT
};

// ─────────────────────────────────────────────────────────────
//  Single upgrade card (UI element)
// ─────────────────────────────────────────────────────────────
struct UpgradeCard {
    UpgradeID    id;
    sf::FloatRect bounds;      // screen rect of the card
    bool          hovered  = false;
    bool          affordable= false;
};

// ─────────────────────────────────────────────────────────────
//  Shop  — upgrade screen renderer + input handler
// ─────────────────────────────────────────────────────────────
class Shop {
public:
    Shop();

    /// Call once after window is created
    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH);

    /// Handle mouse input — returns true if a purchase was made
    bool handleEvent(const sf::Event& event,
                     GameState&       state);

    /// Update hover states
    void update(sf::Vector2f mousePos, const GameState& state);

    /// Render shop panel
    void draw(sf::RenderTarget& target,
              const GameState&  state) const;

    // Scroll support
    void scrollBy(float delta);

private:
    // ── Layout ────────────────────────────────────────────
    sf::Font* m_font    = nullptr;
    float     m_x       = 0.f;
    float     m_y       = 0.f;
    float     m_w       = 0.f;
    float     m_h       = 0.f;
    float     m_scroll  = 0.f;
    float     m_maxScroll = 0.f;

    ShopCategory m_activeCategory = ShopCategory::WEAPONS;

    // ── Cards ─────────────────────────────────────────────
    std::vector<UpgradeCard> m_cards;

    // ── Helpers ───────────────────────────────────────────
    void  buildCards(const GameState& state);
    void  drawBackground(sf::RenderTarget& target) const;
    void  drawCategoryTabs(sf::RenderTarget& target,
                            const GameState& state) const;
    void  drawCard(sf::RenderTarget&  target,
                   const UpgradeCard& card,
                   const GameState&   state) const;
    void  drawScrollBar(sf::RenderTarget& target) const;

    std::string formatEffect(UpgradeID id,
                              const GameState& state) const;

    sf::FloatRect tabBounds(int idx) const;

    // ── Constants ─────────────────────────────────────────
    static constexpr float TAB_H        = 36.f;
    static constexpr float CARD_H       = 82.f;
    static constexpr float CARD_MARGIN  = 8.f;
    static constexpr float CARD_PADDING = 10.f;
    static constexpr float SCROLLBAR_W  = 8.f;
};