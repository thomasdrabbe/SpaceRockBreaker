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
              float panelW, float panelH,
              float uiScale = 1.f);

    void draw(sf::RenderTarget& target,
              const GameState&  state,
              bool              seeThroughMiningBackdrop = false) const;

    bool handleClick(sf::Vector2f pos, GameState& state);
    void handleScroll(float delta, sf::Vector2f pos, bool shiftHeld = false);
    /// Scrollbalk of leeg gebied: true = geen node-koop op deze klik.
    bool handlePointerDown(sf::Vector2f pos);
    void handlePointerMove(sf::Vector2f pos);
    void handlePointerUp();
    void setMousePos(sf::Vector2f pos) { m_mousePos = pos; }
    void resetScroll();

private:
    sf::Font* m_font = nullptr;
    float     m_x     = 0.f;
    float     m_y     = 0.f;
    float     m_w     = 0.f;
    float     m_h     = 0.f;
    float     m_uiScale = 1.f;

    mutable float m_scrollX = 0.f;
    mutable float m_scrollY = 0.f;
    mutable sf::Vector2f m_mousePos{ -1.f, -1.f };
    mutable const UpgradeNodeDef* m_hoveredNode = nullptr;

    static constexpr float NODE_W       = 160.f;
    static constexpr float NODE_H       = 72.f;
    static constexpr float GRID_STEP_X  = 200.f;
    static constexpr float GRID_STEP_Y  = 120.f;
    static constexpr float PANEL_PAD   = 40.f;

    float nodeW()     const { return NODE_W * m_uiScale; }
    float nodeH()     const { return NODE_H * m_uiScale; }
    float gridStepX() const { return GRID_STEP_X * m_uiScale; }
    float gridStepY() const { return GRID_STEP_Y * m_uiScale; }
    float panelPad()  const { return PANEL_PAD * m_uiScale; }
    unsigned fontSz(unsigned base) const {
        return static_cast<unsigned>(std::round(base * m_uiScale));
    }

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
    void drawScrollBars(sf::RenderTarget& target) const;
    float scrollBarThickness() const;
    float viewportW() const;
    float viewportH() const;
    bool  scrollNeededX() const;
    bool  scrollNeededY() const;
    sf::FloatRect hTrackBounds() const;
    sf::FloatRect vTrackBounds() const;
    sf::FloatRect hThumbBounds() const;
    sf::FloatRect vThumbBounds() const;
    void setScrollFromHThumbCenter(float thumbCenterX);
    void setScrollFromVThumbCenter(float thumbCenterY);

    mutable float m_minScrollX = 0.f;
    mutable float m_minScrollY = 0.f;
    mutable float m_contentW   = 0.f;
    mutable float m_contentH   = 0.f;
    int           m_dragAxis   = 0; // 0 none, 1 horizontal, 2 vertical
    float         m_dragGrabOffset = 0.f;
};
