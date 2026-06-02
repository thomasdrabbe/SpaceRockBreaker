#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <string>
#include "Constants.h"
#include "GameState.h"
#include "MiningScreen.h"
#include "Plinko.h"
#include "SkillTree.h"
#include "ChestScreen.h"
#include "NotificationSystem.h"
#include "Particle.h"
#include "RunFlowController.h"
#include "UiFlowController.h"
#include "UnlockSystem.h"

class IAudioBus;

// ─────────────────────────────────────────────────────────────
//  Game
// ─────────────────────────────────────────────────────────────
class Game {
public:
    Game();
    void run();

    void setTabVisible(Tab t, bool visible);
    bool isTabVisible(Tab t) const;
    void markTabBadge(Tab t);
    void tryStartRunFromBase();

private:
    // ── Window ────────────────────────────────────────────
    mutable sf::RenderWindow m_window;
    mutable sf::Font         m_font;
    mutable sf::Font         m_fontFallback;
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

    std::array<bool, TAB_COUNT> m_tabVisible{};
    float                       m_hitFlashTimer = 0.f;

    NotificationSystem m_notifications;
    UnlockSystem       m_unlockSystem;
    IAudioBus*         m_audio = nullptr;
    std::unique_ptr<RunFlowController> m_runFlow;
    std::unique_ptr<UiFlowController>  m_uiFlow;

    // ── Assets (gedeeld met mining UI) ────────────────────
    sf::Texture m_keyTex;
    bool          m_keyTexLoaded = false;
    sf::Texture m_chestTex;
    bool        m_chestTexLoaded = false;
    std::array<sf::Texture, GEM_TYPE_COUNT_INT> m_gemTex{};
    std::array<bool, GEM_TYPE_COUNT_INT>        m_gemTexLoaded{};
    std::array<const sf::Texture*, GEM_TYPE_COUNT_INT> m_gemTexPtrs{};

    // ── Sub-systems ───────────────────────────────────────
    MiningScreen m_mining;
    PlinkoBoard  m_plinko;
    /// Alleen Plinko slot-explosies; niet tekenen op mining-tab.
    ParticleSystem m_plinkoParticles{ MAX_PARTICLES };
    SkillTreeScreen m_skillTree;
    ChestScreen  m_chest;

    // ── Notifications (toast + timerbalk; zie NotificationSystem) ──
    void pushNotif(const std::string& text,
                   sf::Color color = sf::Color(220, 230, 255),
                   float     holdSec = 2.5f);

    // ── Save slots (0 … SAVE_SLOT_COUNT-1) ────────────────
    int  m_saveSlot = 0;
    /// True na "Doorgaan" / nieuw spel: mag naar schijf schrijven (geen auto-load bij start).
    bool m_diskSessionActive = false;
    [[nodiscard]] std::string currentSavePath() const {
        return saveSlotPath(m_saveSlot);
    }

    // ── Save timer ────────────────────────────────────────
    float m_saveTimer = 0.f;
    static constexpr float SAVE_INTERVAL = 30.f;
    // Onkwetsbaar na hit (duur hangt van moeilijkheid af)
    float m_hitCooldown = 0.f;
    float m_oreFusionTimer = 0.f;
    /// Korte blokkade na tab-wissel (voorkomt per ongeluk upgrade-koop).
    float m_uiClickSuppressRemain = 0.f;
    RunEndReason m_lastRunEndReason = RunEndReason::NONE;
    RunEndReason m_runEndOutroReason = RunEndReason::NONE;
    float        m_runEndOutroRemain = 0.f;
    static constexpr float RUN_END_OUTRO_SEC = 1.0f;
    void beginRunEndOutro(RunEndReason reason);
    void finishRunEndOutro();
    void drawRunEndOutroOverlay() const;

    void processGemPickupNotifs();
    void loadGemTextures();
    void updateTutorial();
    void drawTutorialOverlay() const;

    enum class TutorialPhase : std::uint8_t {
        INACTIVE = 0,
        TAB_SKILL,
        SECTION_ASTEROIDS,
        NODE_ORE_VALUE,
        SECTION_SHIP,
        NODE_WARP_DRIVE,
        MINING_START,
        DONE
    };
    TutorialPhase m_tutorial = TutorialPhase::INACTIVE;

    bool m_mainMenuPickDifficulty = false;
    struct SaveSlotPreview {
        bool        hasSave = false;
        int         zone = 1;
        double      credits = 0.0;
        std::string summary = "Leeg";
    };
    mutable std::array<SaveSlotPreview, SAVE_SLOT_COUNT> m_saveSlotPreview{};
    mutable bool m_saveSlotPreviewDirty = true;

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
    float m_warpCharge = 0.f;   // 0.0 .. 1.0, warp bij 1.0
    float m_warpFlashRemain = 0.f; // witte flits na warp (sec)
    /// Na loslaten Space weer true → volgende vasthoud start warp-SFX opnieuw.
    bool m_warpSfxArmed = true;

    // ── Key asteroid (één per zone, spawn na delay) ───────
    sf::Clock m_animClock;
    int       m_zonePlayLevel      = 1;
    float     m_zonePlayTime       = 0.f;
    bool      m_keySpawnedThisZone = false;
    void      resetZoneKeyState();

    /// Fullscreen chest-open anim (na 1-key open); blokkeert nieuwe opens.
    float m_chestOverlayAnim = 0.f;
    bool  m_chestLootSfxPending = false;
    void  drawChestOpenOverlay();

    /// Loot-tekst springt uit de chest (midden scherm), niet als rechter-notif.
    static constexpr float CHEST_LOOT_POPUP_SEC = 2.35f;
    bool                   m_chestLootPopupActive = false;
    std::string            m_chestLootPopupText;
    sf::Color              m_chestLootPopupColor{ 255, 220, 140 };
    float                  m_chestLootPopupRemain = 0.f;
    void                   drawChestLootPopup() const;

    sf::FloatRect miningStartRunBounds() const;
    sf::FloatRect miningStartZoneButtonBounds(int zone) const;
    int           miningStartZoneAt(sf::Vector2f pos) const;
    void          drawMiningStartZoneButtons(int selectedZone) const;
    int           m_selectedStartZone = 1;
    void          drawMiningBasePanel() const;

    bool          shouldShowRunRetreatButton() const;
    bool          shouldShowPlinkoSideDrop() const;
    float         sidePanelResourcesBottomY() const;
    /// Onderkant van de kleine "+ … on prestige"-regel (zelfde ty als drawSidePanel).
    float         sidePanelAfterPrestigeHintBottomY() const;
    float         sidePanelAuxReservedHeight() const;
    float         sidePanelAuxButtonsBaseY() const;
    sf::FloatRect plinkoSideDropButtonBounds() const;
    /// Ruimte onder DROP voor unlock-hint (gap + paneel); 0 als geen Plinko-drop.
    float         plinkoUnlockHintBelowDropBlockHeight() const;
    void          drawPlinkoUnlockHintBelowDrop(
                       bool seeThroughMiningBackdrop) const;
    sf::FloatRect runRetreatButtonBounds() const;
    bool          shouldShowTargetPriorityPanel() const;
    float         targetPriorityPanelHeight() const;
    sf::FloatRect targetPriorityPanelBounds() const;
    void          drawTargetPriorityPanel() const;
    bool          handleTargetPriorityClick(sf::Vector2f pos);
    mutable sf::FloatRect m_targetBtnL{};
    mutable sf::FloatRect m_targetBtnR{};
    void          drawSidePanelAuxButtons() const;
    void          retreatRunToBase();
    void syncMiningSystemsFromState(bool rebuildPlinkoBoard,
                                    bool clearMiningField = false);
    void          collectRunOreToState();
    void          moveRunToBaseState();

    // ── Main loop ─────────────────────────────────────────
    void processEvents();
    void update(float dt);
    void render();

    // ── Input ─────────────────────────────────────────────
    void onMouseClick (sf::Vector2f pos,
                       sf::Mouse::Button btn);
    void onMouseScroll(float delta, sf::Vector2f pos, bool shiftHeld = false);
    void onKeyPress(sf::Keyboard::Key key,
                    bool                 ctrl,
                    bool                 shift);

    // ── Tab bar ───────────────────────────────────────────
    sf::FloatRect tabRect(int visibleIdx) const;
    void          drawTabBar()     const;
    void          drawForegroundTab() const;
    int           visibleTabCount() const;
    Tab           tabFromVisibleSlot(int slot) const;
    void          clampActiveTabToVisibility();
    void          resetNewGameUi();

    [[nodiscard]] bool hubMiningBackdropTransparent() const {
        return m_activeTab != Tab::MINING
            && m_state.difficulty != Difficulty::Easy;
    }

    // ── Side panel ────────────────────────────────────────
    void drawSidePanel() const;
    // DROP (Plinko) onder "+ … on prestige"; retreat-knop onder resources-gat of onder DROP

    // ── Plinko tab ────────────────────────────────────────
    void drawPlinkoTab(bool seeThroughMiningBackdrop) const;
    void handlePlinkoClick(sf::Vector2f pos);
    void rebuildPlinko();

    // ── Prestige screen ───────────────────────────────────
    void drawPrestigeScreen()              const;
    void handlePrestigeClick(sf::Vector2f pos);

    // ── Pause overlay ─────────────────────────────────────
    void drawPauseOverlay() const;
    // ── mainmenu overlay ─────────────────────────────────────

    struct MainMenuLayout {
        float    titleX = 0.f, titleY = 0.f, titleW = 0.f;
        unsigned fTitle = 0, fBtn = 0, fSlot = 0, fHint = 0;
        float    slotY = 0.f, slotX0 = 0.f, slotW = 0.f;
        float    slotH = 0.f, slotGap = 0.f;
        float    btnW = 0.f, btnH = 0.f, gap = 0.f, firstBtnTop = 0.f;
    };
    MainMenuLayout computeMainMenuLayout() const;

    void drawMainMenu() const;
    void handleMainMenuClick(sf::Vector2f pos);
    void refreshSaveSlotPreviewCache() const;
    void invalidateSaveSlotPreviewCache();

    // ── Helpers ───────────────────────────────────────────
    void drawText(const std::string& str,
                  float x, float y,
                  unsigned size,
                  sf::Color color,
                  bool bold = false) const;

    std::string formatBig(double v) const;
    std::string pct(float v)        const;
};
