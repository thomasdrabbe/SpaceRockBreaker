#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Constants.h"
#include "GameState.h"

struct ChestCard {
    ChestUpgradeID id;
    sf::FloatRect  bounds;
    bool           hovered    = false;
    bool           affordable = false;
};

class ChestScreen {
public:
    ChestScreen();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH,
              float scale = 1.f);

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
    float     m_scale = 1.f;

    float m_scroll     = 0.f;
    float m_maxScroll  = 0.f;
    float m_headerH    = 52.f;
    float m_cardH      = 96.f;
    float m_cardMargin = 10.f;
    float m_cardPad    = 12.f;
    float m_scrollBarW = 8.f;

    std::vector<ChestCard> m_cards;

    void buildCards(const GameState& state);
    void drawScrollBar(sf::RenderTarget& target) const;
    void drawCard(sf::RenderTarget& target,
                   const ChestCard& card,
                   const GameState& state) const;
    std::string formatEffect(ChestUpgradeID id,
                             const GameState& state) const;
};
