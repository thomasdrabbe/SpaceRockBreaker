#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Constants.h"

class GameState;

struct UpgradeNodeDef {
    UpgradeID id;
    int       gridX = 0;
    int       gridY = 0;
    /// UPGRADE_COUNT = geen vereiste (altijd unlocked voor aankoop-check).
    UpgradeID requireId    = UpgradeID::UPGRADE_COUNT;
    int       requireLevel = 1;
};

extern const std::vector<UpgradeNodeDef> SKILL_TREE_NODES;

class SkillTreeScreen {
public:
    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH);

    void draw(sf::RenderTarget& target,
              const GameState&  state,
              bool              seeThroughMiningBackdrop = false) const;

    bool handleClick(sf::Vector2f pos, GameState& state);
    void handleScroll(float delta, sf::Vector2f pos);
    void setMousePos(sf::Vector2f pos) { m_mousePos = pos; }
    void resetScroll();

private:
    sf::Font* m_font = nullptr;
    float     m_x     = 0.f;
    float     m_y     = 0.f;
    float     m_w     = 0.f;
    float     m_h     = 0.f;

    mutable float m_scrollX = 0.f;
    mutable float m_scrollY = 0.f;
    mutable sf::Vector2f m_mousePos{ -1.f, -1.f };
    mutable const UpgradeNodeDef* m_hoveredNode = nullptr;

    static constexpr float NODE_W       = 160.f;
    static constexpr float NODE_H       = 72.f;
    static constexpr float GRID_STEP_X  = 200.f;
    static constexpr float GRID_STEP_Y  = 120.f;
    static constexpr float PANEL_PAD   = 40.f;

    sf::Vector2f nodeScreenPos(const UpgradeNodeDef& node) const;
    sf::FloatRect nodeRect(const UpgradeNodeDef& node) const;

    void drawConnections(sf::RenderTarget& target,
                         const GameState&  state) const;
    void drawNode(sf::RenderTarget&     target,
                  const UpgradeNodeDef& node,
                  const GameState&      state,
                  bool                  seeThroughMiningBackdrop) const;
    void drawTooltip(sf::RenderTarget&     target,
                     const UpgradeNodeDef& node,
                     const GameState&      state,
                     sf::Vector2f          nodePos) const;

    void updateScrollLimits() const;
    mutable float m_minScrollX = 0.f;
    mutable float m_minScrollY = 0.f;
};
