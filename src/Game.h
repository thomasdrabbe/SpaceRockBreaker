#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include "Constants.h"
#include "GameState.h"
#include "MiningScreen.h"
#include "Plinko.h"
#include "Shop.h"
#include "ChestScreen.h"

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

    // ── Pause menu state ──────────────────────────────────
    enum class PauseButton { NONE, RESUME, SAVE, MAIN_MENU };
    PauseButton pauseButtonAt(sf::Vector2f pos) const;
    bool m_showMainMenu = true;


    // ── State ─────────────────────────────────────────────
    GameState m_state;
    Tab       m_activeTab = Tab::MINING;
    bool      m_paused    = false;
    RunMode   m_runMode   = RunMode::BASE;

    // ── Sub-systems ───────────────────────────────────────
    MiningScreen m_mining;
    PlinkoBoard  m_plinko;
    Shop         m_shop;
    ChestScreen  m_chest;

    // ── Notifications ─────────────────────────────────────
    static constexpr int MAX_NOTIFS = 6;
    std::array<Notification, MAX_NOTIFS> m_notifs;
    void pushNotif(const std::string& text,
                   sf::Color color = sf::Color(220, 230, 255));

    // ── Save slots (0 … SAVE_SLOT_COUNT-1) ────────────────
    int m_saveSlot = 0;
    [[nodiscard]] std::string currentSavePath() const {
        return saveSlotPath(m_saveSlot);
    }

    // ── Save timer ────────────────────────────────────────
    float m_saveTimer = 0.f;
    static constexpr float SAVE_INTERVAL = 30.f;
    //hit cooldown
    float m_hitCooldown = 0.f;
    static constexpr float HIT_COOLDOWN = 2.f;  // seconden onkwetsbaar na hit

    void drawLives() const;

    // ── Prestige confirm ──────────────────────────────────
    bool m_prestigeConfirm = false;

    // ── Dynamic layout ────────────────────────────────────
    // Berekend op basis van werkelijke schermgrootte
    float m_scrW     = 0.f;   // werkelijke breedte
    float m_scrH     = 0.f;   // werkelijke hoogte
    float m_scale    = 1.f;   // UI-schaalfactor t.o.v. 1920x1080
    float m_tabH     = 0.f;   // tab bar hoogte
    float m_sideW    = 0.f;   // side panel breedte
    float m_cntX     = 0.f;   // content origin X
    float m_cntY     = 0.f;   // content origin Y
    float m_cntW     = 0.f;   // content breedte
    float m_cntH     = 0.f;   // content hoogte

    void initLayout();         // aanroepen na window.create()
    void reinitSystems();      // herinitialiseer subsystems na resize

    // ── Warp charge ───────────────────────────────────────
    float m_warpCharge = 0.f;   // 0.0 → 1.0, warp bij 1.0
    static constexpr float WARP_CHARGE_TIME = 2.f;

    // ── Key asteroid (één per zone, spawn na delay) ───────
    sf::Clock m_animClock;
    int       m_zonePlayLevel      = 1;
    float     m_zonePlayTime       = 0.f;
    bool      m_keySpawnedThisZone = false;
    void      resetZoneKeyState();

    sf::FloatRect miningStartRunBounds() const;
    void          drawMiningBasePanel() const;

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
    // ── mainmenu overlay ─────────────────────────────────────

    void drawMainMenu() const;
    void handleMainMenuClick(sf::Vector2f pos);

    // ── Notifications ─────────────────────────────────────
    void updateNotifs(float dt);
    void drawNotifs()  const;

    // ── Helpers ───────────────────────────────────────────
    void drawText(const std::string& str,
                  float x, float y,
                  unsigned size,
                  sf::Color color,
                  bool bold = false) const;

    std::string formatBig(double v) const;
    std::string pct(float v)        const;
};
