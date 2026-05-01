#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"
#include "GameState.h"

enum class ShopCategory {
    WEAPONS = 0,
    MINING,
    PLINKO,
    ECONOMY,
    ORE_TIERS,        // ← nieuw
    CATEGORY_COUNT
};

struct UpgradeCard {
    UpgradeID     id;
    sf::FloatRect bounds;
    bool          hovered    = false;
    bool          affordable = false;
};

class Shop {
public:
    Shop();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH,
              float scale = 1.f);          // ← nieuw

    bool handleEvent(const sf::Event& event, GameState& state,
                     const sf::RenderWindow& window);
    void update(sf::Vector2f mousePos, const GameState& state);
    void draw(sf::RenderTarget& target, const GameState& state) const;
    void scrollBy(float delta);

private:
    sf::Font* m_font  = nullptr;
    float     m_x     = 0.f;
    float     m_y     = 0.f;
    float     m_w     = 0.f;
    float     m_h     = 0.f;
    float     m_scale = 1.f;              // ← nieuw
    float     m_scroll    = 0.f;
    float     m_maxScroll = 0.f;

    // Schaalbare layout-waarden (berekend in init)
    float m_tabH      = 36.f;
    float m_cardH     = 82.f;
    float m_cardMargin= 8.f;
    float m_cardPad   = 10.f;
    float m_scrollBarW= 8.f;

    ShopCategory             m_activeCategory = ShopCategory::WEAPONS;
    std::vector<UpgradeCard> m_cards;

    void buildCards(const GameState& state);
    void drawBackground(sf::RenderTarget& target) const;
    void drawCategoryTabs(sf::RenderTarget& target, const GameState& state) const;
    void drawCard(sf::RenderTarget& target, const UpgradeCard& card, const GameState& state) const;
    void drawScrollBar(sf::RenderTarget& target) const;

    std::string   formatEffect(UpgradeID id, const GameState& state) const;
    sf::FloatRect tabBounds(int idx) const;
};
