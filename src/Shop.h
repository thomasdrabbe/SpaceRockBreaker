#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>
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
    void draw(sf::RenderTarget& target, const GameState& state,
              bool seeThroughMiningBackdrop = false) const;
    void scrollBy(float delta);

    void setCategoryVisible(ShopCategory cat, bool visible);
    bool isCategoryVisible(ShopCategory cat) const;
    void setMiningShowsWarpOnly(bool warpOnly);
    bool miningShowsWarpOnly() const { return m_miningShowsWarpOnly; }
    void resetProgressiveShopState();

    /// Alleen als categorie zichtbaar is: actief maken + cards bouwen (bv. toets W).
    bool trySelectCategory(ShopCategory cat, GameState& state);

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

    ShopCategory             m_activeCategory = ShopCategory::MINING;
    std::vector<UpgradeCard> m_cards;

    std::array<bool, static_cast<int>(ShopCategory::CATEGORY_COUNT)>
        m_categoryVisible{};
    bool m_miningShowsWarpOnly = true;

    uint64_t m_layoutFp = 0;

    [[nodiscard]] uint64_t layoutFingerprint(const GameState& state) const;

    void        ensureActiveCategoryVisible();
    int         visibleCategoryCount() const;
    ShopCategory categoryFromTabIndex(int visibleTabIdx) const;

    void buildCards(const GameState& state);
    void drawBackground(sf::RenderTarget& target,
                        bool               seeThroughMiningBackdrop) const;
    void drawCategoryTabs(sf::RenderTarget& target, const GameState& state,
                          bool               seeThroughMiningBackdrop) const;
    void drawCard(sf::RenderTarget& target, const UpgradeCard& card,
                  const GameState& state, bool seeThroughMiningBackdrop) const;
    void drawScrollBar(sf::RenderTarget& target,
                       bool               seeThroughMiningBackdrop) const;

    std::string   formatEffect(UpgradeID id, const GameState& state) const;
    sf::FloatRect tabBounds(int idx) const;
};
