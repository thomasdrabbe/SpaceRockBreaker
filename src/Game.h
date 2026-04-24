#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include "Constants.h"
#include "GameState.h"
#include "MiningScreen.h"
#include "Plinko.h"
#include "Shop.h"

// ─────────────────────────────────────────────────────────────
//  Notification
// ─────────────────────────────────────────────────────────────
struct Notification {
    std::string text;
    sf::Color   color;
    float       lifetime    = 2.5f;
    float       maxLifetime = 2.5f;
    float       yOffset     = 0.f;
    bool        alive       = false;
};

// ─────────────────────────────────────────────────────────────
//  Game
// ─────────────────────────────────────────────────────────────
class Game {
public:
    Game();
    void run();

private:
    // ── Window ────────────────────────────────────────────
    mutable sf::RenderWindow m_window;
    mutable sf::Font         m_font;
    sf::Clock        m_clock;

    // ── State ─────────────────────────────────────────────
    GameState m_state;
    Tab       m_activeTab = Tab::MINING;
    bool      m_paused    = false;

    // ── Sub-systems ───────────────────────────────────────
    MiningScreen m_mining;
    PlinkoBoard  m_plinko;
    Shop         m_shop;

    // ── Notifications ─────────────────────────────────────
    static constexpr int MAX_NOTIFS = 6;
    std::array<Notification, MAX_NOTIFS> m_notifs;
    void pushNotif(const std::string& text,
                   sf::Color color = sf::Color(220, 230, 255));

    // ── Save timer ────────────────────────────────────────
    float m_saveTimer = 0.f;
    static constexpr float SAVE_INTERVAL = 30.f;

    // ── Prestige confirm ──────────────────────────────────
    bool m_prestigeConfirm = false;

    // ── Main loop ─────────────────────────────────────────
    void processEvents();
    void update(float dt);
    void render();

    // ── Input ─────────────────────────────────────────────
    void onMouseClick (sf::Vector2f pos,
                       sf::Mouse::Button btn);
    void onMouseScroll(float delta, sf::Vector2f pos);
    void onKeyPress   (sf::Keyboard::Key key);

    // ── Tab bar ───────────────────────────────────────────
    sf::FloatRect tabRect(int idx) const;
    void          drawTabBar()     const;
    void          drawActiveTab()  const;

    // ── Side panel ────────────────────────────────────────
    void drawSidePanel() const;

    // ── Plinko tab ────────────────────────────────────────
    void drawPlinkoTab()             const;
    void handlePlinkoClick(sf::Vector2f pos);
    void rebuildPlinko();

    // ── Prestige screen ───────────────────────────────────
    void drawPrestigeScreen()              const;
    void handlePrestigeClick(sf::Vector2f pos);

    // ── Pause overlay ─────────────────────────────────────
    void drawPauseOverlay() const;

    // ── Notifications ─────────────────────────────────────
    void updateNotifs(float dt);
    void drawNotifs()  const;

    // ── Helper ────────────────────────────────────────────
    void drawText(const std::string& str,
                  float x, float y,
                  unsigned size,
                  sf::Color color,
                  bool bold = false) const;

    // ── Layout constants ──────────────────────────────────
    static constexpr float TAB_BAR_H = 46.f;
    static constexpr float SIDE_W    = 240.f;
    static constexpr float CONTENT_X = 0.f;
    static constexpr float CONTENT_Y = TAB_BAR_H;
    static constexpr float CONTENT_W = WINDOW_WIDTH  - SIDE_W;
    static constexpr float CONTENT_H = WINDOW_HEIGHT - TAB_BAR_H;
};
