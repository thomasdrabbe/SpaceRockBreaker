#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include "Constants.h"
#include "GameState.h"

class ChestScreen {
public:
    ChestScreen();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH,
              float scale = 1.f);

    bool handleEvent(const sf::Event& event, GameState& state,
                     const sf::RenderWindow& window,
                     ChestUpgradeID* outPurchased = nullptr);
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

    float m_cardMargin = 10.f;
    float m_cardPad    = 12.f;

    sf::FloatRect m_lootBtn{};
    bool          m_lootHovered = false;

    uint64_t m_layoutFp = 0;

    [[nodiscard]] uint64_t layoutFingerprint(const GameState& state) const;
    void                     rebuildLayout(const GameState& state);

    std::string formatEffect(ChestUpgradeID id,
                             const GameState& state) const;
};
