#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "Constants.h"
#include "GameState.h"
#include "MiningScreen.h"
#include "Plinko.h"
#include "Shop.h"

// ─────────────────────────────────────────────────────────────
//  Notification  — small toast message that fades out
// ─────────────────────────────────────────────────────────────
struct Notification {
    std::string  text;
    sf::Color    color;
    float        lifetime    = 2.5f;
    float        maxLifetime = 2.5f;
    float        yOffset     = 0.f;   // animated upward
    bool         alive       = false;
};

// ─────────────────────────────────────────────────────────────
//  Game  — top-level controller
// ─────────────────────────────────────────────────────────────
class Game {
public:
    Game();

    /// Main entry point — runs until window closes
    void run();

private:
    // ── Window ────────────────────────────────────────────
	mutable sf::RenderWindow m_window;
	mutable sf::Font m_font;
    sf::Clock        m_clock;

    // ── State ─────────────────────────────────────────────
    GameState    m_state;
    Tab          m_activeTab = Tab::MINING;

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
    float m_saveTimer     = 0.f;
    static constexpr float SAVE_INTERVAL = 30.f;   // seconds

    // ── Prestige confirm state ─────────────────────────────
    bool  m_prestigeConfirm = false;

    // ── Main loop phases ──────────────────────────────────
    void processEvents();
    void update(float dt);
    void render();

    // ── Event handlers ────────────────────────────────────
    void onMouseClick(sf::Vector2f pos, sf::Mouse::Button btn);
    void onMouseScroll(float delta, sf::Vector2f pos);
    void onKeyPress(sf::Keyboard::Key key);

    // ── Tab bar ───────────────────────────────────────────
    sf::FloatRect tabRect(int idx)  const;
    void          drawTabBar()      const;
    void          drawActiveTab()   const;

    // ── Side panel (right) ────────────────────────────────
    void drawSidePanel() const;

    // ── Prestige screen ───────────────────────────────────
    void drawPrestigeScreen() const;
    void handlePrestigeClick(sf::Vector2f pos);

    // ── Plinko tab ────────────────────────────────────────
    void drawPlinkoTab() const;
    void handlePlinkoClick(sf::Vector2f pos);
    void rebuildPlinko();

    // ── Notifications ─────────────────────────────────────
    void updateNotifs(float dt);
    void drawNotifs()  const;

    // ── Helpers ───────────────────────────────────────────
    void drawText(sf::RenderTarget& target,
                  const std::string& str,
                  float x, float y,
                  unsigned size,
                  sf::Color color,
                  bool bold = false) const;

    // Layout constants
    static constexpr float TAB_BAR_H    = 42.f;
    static constexpr float SIDE_W       = 220.f;
    static constexpr float CONTENT_X    = 0.f;
    static constexpr float CONTENT_Y    = TAB_BAR_H;
    static constexpr float CONTENT_W    = WINDOW_WIDTH  - SIDE_W;
    static constexpr float CONTENT_H    = WINDOW_HEIGHT - TAB_BAR_H;
};