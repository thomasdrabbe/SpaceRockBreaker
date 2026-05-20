#include "Game.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace {

constexpr float CHEST_OVERLAY_SEC = 1.05f;
constexpr float PLAYER_HIT_HP_COOLDOWN = 0.8f;

struct StartZonePickerLayout {
    int   visibleCount = 0;
    int   perRow       = START_ZONE_BUTTONS_PER_ROW;
    int   rows         = 0;
    float btnW         = 0.f;
    float btnH         = 0.f;
    float gap          = 0.f;
    float rowGap       = 0.f;
    float blockTopY    = 0.f;
    float blockHeight  = 0.f;
};

StartZonePickerLayout makeStartZonePickerLayout(int  highestReached,
                                                float scale,
                                                float cntY,
                                                float cntH) {
    StartZonePickerLayout L{};
    L.visibleCount = std::clamp(highestReached, 0, START_ZONE_PICKER_MAX_ZONES);
    L.rows         = L.visibleCount > 0
        ? (L.visibleCount + L.perRow - 1) / L.perRow
        : 0;
    L.btnW         = std::round(64.f * scale);
    L.btnH         = std::round(36.f * scale);
    L.gap          = std::round(8.f * scale);
    L.rowGap       = std::round(8.f * scale);
    L.blockTopY    = cntY + cntH * 0.48f;
    if (L.rows > 0) {
        L.blockHeight = static_cast<float>(L.rows) * L.btnH
                      + static_cast<float>(L.rows - 1) * L.rowGap;
    }
    return L;
}

sf::FloatRect startZoneButtonRect(int                       zone,
                                  float                     cx,
                                  const StartZonePickerLayout& L) {
    if (zone < 1 || zone > L.visibleCount)
        return {};

    const int index    = zone - 1;
    const int row      = index / L.perRow;
    const int col      = index % L.perRow;
    const int rowStart = row * L.perRow;
    const int rowEnd   = std::min(rowStart + L.perRow, L.visibleCount);
    const int rowCount = rowEnd - rowStart;
    const float rowW =
        static_cast<float>(rowCount) * L.btnW
        + static_cast<float>(rowCount - 1) * L.gap;
    const float left =
        cx - rowW * 0.5f + static_cast<float>(col) * (L.btnW + L.gap);
    const float top = L.blockTopY + static_cast<float>(row) * (L.btnH + L.rowGap);
    return sf::FloatRect({ left, top }, { L.btnW, L.btnH });
}

float startRunButtonTopY(const StartZonePickerLayout& L,
                         float scale,
                         float cntY,
                         float cntH,
                         float runBtnH) {
    if (L.visibleCount <= 0)
        return cntY + cntH * 0.58f - runBtnH * 0.5f;
    return L.blockTopY + L.blockHeight + std::round(24.f * scale);
}

void migrateLegacySaveIfNeeded() {
    for (int s = 0; s < SAVE_SLOT_COUNT; ++s) {
        std::ifstream probe(saveSlotPath(s), std::ios::binary);
        if (probe)
            return;
    }
    std::ifstream in(SAVE_FILE, std::ios::binary);
    if (!in)
        return;
    std::ofstream out(saveSlotPath(0), std::ios::binary);
    out << in.rdbuf();
}

/// `Game::update` vergelijkt plinko-layout met deze cache om rebuilds te batchen.
/// Na nieuw spel / laden moet de cache gereset: anders kan `rebuildPlinko()` uitblijven
/// wanneer oude en nieuwe layout toevallig gelijk zijn terwijl de board-staat wél verschilt.
struct PlinkoLayoutWatchState {
    int   lastRows       = -1;
    float lastBonus      = -1.f;
    float lastLuck       = -1.f;
    int   lastPegUp      = -1;
    float lastSlotChest  = -1.f;
    int   lastDupRolls   = -1;
};

PlinkoLayoutWatchState& plinkoLayoutWatch() {
    static PlinkoLayoutWatchState s;
    return s;
}

void resetPlinkoLayoutWatch() {
    PlinkoLayoutWatchState& w = plinkoLayoutWatch();
    w.lastRows      = -1;
    w.lastBonus     = -1.f;
    w.lastLuck      = -1.f;
    w.lastPegUp     = -1;
    w.lastSlotChest = -1.f;
    w.lastDupRolls  = -1;
}

std::string readRuntimeVersionTag() {
    auto firstLineTrimmed = [](const std::string& p) -> std::string {
        std::ifstream in(p);
        if (!in)
            return {};
        std::string line;
        if (!std::getline(in, line))
            return {};
        while (!line.empty()
               && (line.back() == '\r' || line.back() == '\n'
                   || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        return line;
    };

    const std::string appDir = applicationDirectory();
    if (!appDir.empty()) {
        const std::string fromApp = firstLineTrimmed(appDir + "/version.txt");
        if (!fromApp.empty())
            return fromApp;
    }
    const std::string fromCwd = firstLineTrimmed("version.txt");
    if (!fromCwd.empty())
        return fromCwd;
    return "unknown";
}

void drawPanelCoin(sf::RenderTarget& rw, float cx, float cy, float r) {
    sf::CircleShape rim(r);
    rim.setOrigin({ r, r });
    rim.setPosition({ cx, cy });
    rim.setFillColor(sf::Color(255, 200, 70));
    rim.setOutlineColor(sf::Color(255, 245, 200));
    rim.setOutlineThickness(std::max(1.f, r * 0.12f));
    rw.draw(rim);
    float ir = r * 0.55f;
    sf::CircleShape in(ir);
    in.setOrigin({ ir, ir });
    in.setPosition({ cx - r * 0.08f, cy - r * 0.1f });
    in.setFillColor(sf::Color(255, 235, 160));
    rw.draw(in);
}

void drawPanelCrystal(sf::RenderTarget& rw, float cx, float cy, float s) {
    sf::ConvexShape d;
    d.setPointCount(4);
    d.setPoint(0, { 0.f, -s });
    d.setPoint(1, { s * 0.72f, 0.f });
    d.setPoint(2, { 0.f, s });
    d.setPoint(3, { -s * 0.72f, 0.f });
    d.setPosition({ cx, cy });
    d.setFillColor(sf::Color(180, 120, 255));
    d.setOutlineColor(sf::Color(230, 200, 255));
    d.setOutlineThickness(1.2f);
    rw.draw(d);
    sf::ConvexShape facet;
    facet.setPointCount(3);
    facet.setPoint(0, { 0.f, -s * 0.35f });
    facet.setPoint(1, { s * 0.25f, 0.f });
    facet.setPoint(2, { 0.f, s * 0.35f });
    facet.setPosition({ cx - s * 0.22f, cy });
    facet.setFillColor(sf::Color(220, 180, 255, 200));
    rw.draw(facet);
}

void drawPanelKey(sf::RenderTarget& rw, float cx, float cy, float s,
                  const sf::Texture* keyTex) {
    if (keyTex && keyTex->getSize().x > 0u) {
        sf::Sprite spr(*keyTex);
        const sf::Vector2u tsz = keyTex->getSize();
        const float          side = s * 2.4f;
        const float          sc =
            side / std::max(1.f, static_cast<float>(std::max(tsz.x, tsz.y)));
        spr.setOrigin({ tsz.x * 0.5f, tsz.y * 0.5f });
        spr.setPosition({ cx, cy });
        spr.setScale({ sc, sc });
        rw.draw(spr);
        return;
    }
    sf::RectangleShape stem(sf::Vector2f{ s * 0.2f, s * 1.05f });
    stem.setOrigin({ stem.getSize().x * 0.5f, stem.getSize().y * 0.88f });
    stem.setPosition({ cx, cy + s * 0.12f });
    stem.setRotation(sf::degrees(-10.f));
    stem.setFillColor(sf::Color(240, 210, 130));
    stem.setOutlineColor(sf::Color(255, 255, 255, 160));
    stem.setOutlineThickness(1.f);
    rw.draw(stem);
    float br = s * 0.48f;
    sf::CircleShape bow(br);
    bow.setOrigin({ br, br });
    bow.setPosition({ cx - s * 0.1f, cy - s * 0.35f });
    bow.setFillColor(sf::Color(255, 225, 140));
    bow.setOutlineColor(sf::Color(255, 255, 255, 200));
    bow.setOutlineThickness(1.2f);
    rw.draw(bow);
}

// Bold label centered in a rectangle (origin-based; avoids drawText style order).
// `str` by value: avoids reference-to-temporary edge cases when callers pass
// `const char*` / `std::string` (implicit sf::String) — can otherwise crash in
// MSVC debug CRT inside string / strlen paths.
void drawBoldTextCenteredInRect(sf::RenderTarget& target, const sf::Font& font,
                                sf::String str, float bx, float by, float w,
                                float h, unsigned charSize, sf::Color color,
                                float yNudgePx = 0.f) {
    if (str.isEmpty())
        return;
    sf::Text text(font);
    text.setCharacterSize(charSize);
    text.setString(str);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(color);
    const sf::FloatRect lb = text.getLocalBounds();
    text.setOrigin({std::round(lb.position.x + lb.size.x * 0.5f),
                    std::round(lb.position.y + lb.size.y * 0.5f)});
    text.setPosition({std::round(bx + w * 0.5f),
                      std::round(by + h * 0.5f + yNudgePx)});
    target.draw(text);
}

class GameUnlockEffects final : public IUnlockEffects {
public:
    GameUnlockEffects(Game& game, MiningScreen& mining)
        : m_game(game), m_mining(mining) {}

    void setTabVisible(Tab tab, bool visible) override {
        m_game.setTabVisible(tab, visible);
    }
    void focusSkillTreeTab() override { m_game.focusSkillTreeTab(); }
    void setKeyAsteroidsEnabled(bool enabled) override {
        m_mining.setKeyAsteroidsEnabled(enabled);
    }

private:
    Game&         m_game;
    MiningScreen& m_mining;
};

} // namespace

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
Game::Game()
{
#if defined(_WIN32)
    {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        m_window.create(desktop, sf::String{WINDOW_TITLE}, sf::State::Fullscreen);
    }
    if (!m_window.isOpen()) {
        m_window.create(sf::VideoMode({1920, 1080}),
                        sf::String{WINDOW_TITLE},
                        sf::State::Windowed);
    }
#else
    m_window.create(sf::VideoMode::getDesktopMode(),
                    sf::String{WINDOW_TITLE},
                    sf::State::Fullscreen);
#endif
    m_window.setFramerateLimit(TARGET_FPS);
    {
        // ASCII-titel: em-dash in setTitle crasht SFML/Cocoa op macOS (nil NSString).
        const std::string ver = readRuntimeVersionTag();
        const std::string safeVer = ver.empty() ? "unknown" : ver;
        m_window.setTitle(sf::String("Space Rock Breaker v" + safeVer));
    }

    // Zelfde padlogica als textures; voorkomt mislukte loads bij andere cwd.
    const std::string fontNoto = resolveAssetPath("assets/NotoSans-Regular.ttf");
    const std::string fontAlt  = resolveAssetPath("assets/font.ttf");
    bool              mainOk =
        m_font.openFromFile(fontNoto) || m_font.openFromFile(fontAlt);
    if (!mainOk) {
        for (const std::string& sys : systemFontFallbackPaths()) {
            if (m_font.openFromFile(sys)) {
                mainOk = true;
                break;
            }
        }
    }
    bool fbOk = m_fontFallback.openFromFile(fontAlt);
    if (!fbOk) {
        for (const std::string& sys : systemFontFallbackPaths()) {
            if (m_fontFallback.openFromFile(sys)) {
                fbOk = true;
                break;
            }
        }
    }
    if (!mainOk && fbOk)
        m_font = m_fontFallback;
    if (!mainOk && !fbOk) {
        const char* msg =
            "Geen lettertype geladen. Zorg dat de map 'assets' naast het "
            "programma staat (of start vanuit de build-map).";
#if defined(_WIN32)
        MessageBoxA(nullptr, msg, "Space Rock Breaker", MB_ICONERROR);
#else
        std::fprintf(stderr, "%s\n", msg);
#endif
        std::exit(1);
    }

    initLayout();

    m_keyTexLoaded = m_keyTex.loadFromFile(resolveAssetPath("assets/key.png"));
    if (m_keyTexLoaded)
        m_keyTex.setSmooth(true);

    m_chestTexLoaded = m_chestTex.loadFromFile(
        resolveAssetPath("assets/Animated Chests/Chests.png"));
    if (!m_chestTexLoaded)
        m_chestTexLoaded =
            m_chestTex.loadFromFile(resolveAssetPath("assets/chest.png"));
    if (m_chestTexLoaded) {
        const bool sheet = chestTexIsAnimatedSheet(m_chestTex.getSize());
        m_chestTex.setSmooth(!sheet);
    }

    reinitSystems();

    migrateLegacySaveIfNeeded();
    gSfx.init();
    m_audio = &gSfx;
    m_mining.setAudioBus(m_audio);
    m_plinko.setAudioBus(m_audio);

    m_tabVisible.fill(false);
    m_tabVisible[static_cast<int>(Tab::MINING)] = true;

    m_saveSlot           = 0;
    m_diskSessionActive  = false;

    m_plinko.resetGoldenPegRarityState();
    rebuildPlinko();

    resetZoneKeyState();
    m_runFlow = std::make_unique<RunFlowController>(
        m_state,
        m_mining,
        m_runMode,
        [this]() { rebuildPlinko(); },
        [this]() { resetZoneKeyState(); });
    m_uiFlow = std::make_unique<UiFlowController>(m_activeTab, m_notifications);
    m_mining.syncTurrets(m_state);
}

void Game::resetZoneKeyState() {
    m_zonePlayLevel      = m_state.currentLevel;
    m_zonePlayTime       = 0.f;
    m_keySpawnedThisZone = false;
}

// ─────────────────────────────────────────────────────────────
//  initLayout  — berekent alle layout-waarden op basis van
//  de werkelijke venstergrootte en een schaalfactor
// ─────────────────────────────────────────────────────────────
void Game::initLayout() {
    auto sz  = m_window.getSize();
    m_scrW   = static_cast<float>(sz.x);
    m_scrH   = static_cast<float>(sz.y);

    // Schaalfactor t.o.v. referentieresolutie 1920×1080
    // Neem het gemiddelde van beide assen zodat de UI
    // proportioneel schaalt op zowel brede als hoge schermen
    m_scale  = std::min(m_scrW / 1920.f, m_scrH / 1080.f);

    m_tabH   = std::round(46.f  * m_scale);
    m_sideW  = std::round(300.f * m_scale);   // iets breder voor leesbaarheid

    m_cntX   = 0.f;
    m_cntY   = m_tabH;
    m_cntW   = std::max(64.f, m_scrW - m_sideW);
    m_cntH   = std::max(64.f, m_scrH - m_tabH);
}

// ─────────────────────────────────────────────────────────────
//  reinitSystems  — herinitialiseer subsystems met nieuwe layout
// ─────────────────────────────────────────────────────────────
void Game::reinitSystems() {
    m_mining.init(m_font, m_cntX, m_cntY, m_cntW, m_cntH, m_scale,
                  m_keyTexLoaded ? &m_keyTex : nullptr);
    if (m_audio)
        m_mining.setAudioBus(m_audio);
    m_skillTree.init(m_font, m_cntX, m_cntY, m_cntW, m_cntH, m_scale);
    m_chest.init(m_font, m_cntX, m_cntY, m_cntW, m_cntH, m_scale,
                 m_chestTexLoaded ? &m_chestTex : nullptr);
    rebuildPlinko();
}

// ═════════════════════════════════════════════════════════════
//  run
// ═════════════════════════════════════════════════════════════
void Game::run() {
    while (m_window.isOpen()) {
        float dt = m_clock.restart().asSeconds();
        dt = std::min(dt, 0.05f);

        processEvents();
        update(dt);
        render();
    }
    if (m_diskSessionActive)
        m_state.save(currentSavePath());
    invalidateSaveSlotPreviewCache();
}

// ═════════════════════════════════════════════════════════════
//  processEvents
// ═════════════════════════════════════════════════════════════
void Game::processEvents() {
    while (const std::optional event = m_window.pollEvent()) {

        if (!m_showMainMenu && m_activeTab == Tab::SKILL_TREE) {
            if (const auto* e = event->getIf<sf::Event::MouseMoved>()) {
                m_skillTree.setMousePos(
                    mapPixelToUi(m_window, sf::Vector2i(e->position)));
            }
        }
        if (!m_showMainMenu && m_activeTab == Tab::CHESTS) {
            ChestUpgradeID chestGot{};
            bool           bought = m_chest.handleEvent(
                *event, m_state, m_window, m_chestOverlayAnim > 0.f, &chestGot);
            if (bought) {
                syncMiningSystemsFromState(true);
                m_audio->play(Sfx::ChestOpen);
                m_audio->play(Sfx::LevelUp);
                m_chestLootSfxPending = true;
                m_chestOverlayAnim = CHEST_OVERLAY_SEC;
                const auto& cn =
                    GameState::chestCatalog[static_cast<int>(chestGot)];
                m_chestLootPopupActive = true;
                m_chestLootPopupText =
                    std::string("Chest: ") + cn.name + " +1  (1 key)";
                m_chestLootPopupColor = sf::Color(255, 220, 140);
                m_chestLootPopupRemain  = CHEST_LOOT_POPUP_SEC;
            }
        }

        if (event->is<sf::Event::Closed>()) {
            if (m_diskSessionActive)
                m_state.save(currentSavePath());
            invalidateSaveSlotPreviewCache();
            m_window.close();
        }
        else if (event->is<sf::Event::Resized>()) {
            initLayout();
            reinitSystems();
        }
        else if (const auto* e =
                 event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f pos =
                mapPixelToUi(m_window, sf::Vector2i(e->position));
            onMouseClick(pos, e->button);
        }
        else if (const auto* e =
                 event->getIf<sf::Event::MouseWheelScrolled>()) {
            sf::Vector2f pos =
                mapPixelToUi(m_window, sf::Vector2i(e->position));
            onMouseScroll(e->delta, pos);
        }
        else if (const auto* e =
                 event->getIf<sf::Event::KeyPressed>()) {
            onKeyPress(e->code, e->control, e->shift);
        }
    }
}
// draw lives
void Game::drawLives() const {
    float x = m_cntX + m_cntW - std::round(160.f * m_scale);
    float y = m_cntY + std::round(8.f * m_scale);
    float r = std::round(10.f * m_scale);
    float gap = std::round(28.f * m_scale);

    const int cap = m_state.maxLives();
    for (int i = 0; i < cap; i++) {
        sf::CircleShape heart(r);
        heart.setOrigin({ r, r });
        heart.setPosition({ x + i * gap, y + r });
        heart.setFillColor(i < m_state.lives
            ? sf::Color(255, 60, 80)
            : sf::Color(45, 25, 32));
        heart.setOutlineColor(sf::Color(200, 40, 60, 160));
        heart.setOutlineThickness(1.5f);
        m_window.draw(heart);
    }

    const float maxHp = std::max(1.f, m_mining.playerMaxHp());
    const float ratio =
        std::clamp(m_mining.playerHp() / maxHp, 0.f, 1.f);
    const float barW  = std::round(120.f * m_scale);
    const float barH  = std::round(10.f * m_scale);
    const float barX  = m_cntX + m_cntW - std::round(300.f * m_scale);
    const float barY  = m_cntY + std::round(8.f * m_scale);
    sf::RectangleShape hpBg(sf::Vector2f{ barW, barH });
    hpBg.setPosition({ barX, barY });
    hpBg.setFillColor(sf::Color(40, 10, 10, 200));
    m_window.draw(hpBg);
    const auto rCol = static_cast<std::uint8_t>((1.f - ratio) * 255.f);
    const auto gCol = static_cast<std::uint8_t>(ratio * 200.f);
    sf::RectangleShape hpFill(sf::Vector2f{ barW * ratio, barH });
    hpFill.setPosition({ barX, barY });
    hpFill.setFillColor(sf::Color(rCol, gCol, 30, 220));
    m_window.draw(hpFill);
    drawText("HP",
             barX - std::round(24.f * m_scale),
             barY,
             static_cast<unsigned>(std::round(barH * 1.1f)),
             sf::Color(200, 200, 200),
             false);
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Game::update(float dt) {
    if (m_chestOverlayAnim > 0.f)
        m_chestOverlayAnim = std::max(0.f, m_chestOverlayAnim - dt);
    if (m_chestLootPopupActive) {
        m_chestLootPopupRemain -= dt;
        if (m_chestLootPopupRemain <= 0.f)
            m_chestLootPopupActive = false;
    }
    if (m_chestLootSfxPending) {
        const float elapsed = CHEST_OVERLAY_SEC - m_chestOverlayAnim;
        if (elapsed >= 0.36f) {
            m_audio->play(Sfx::ChestLoot);
            m_chestLootSfxPending = false;
        }
    }
    if (m_chestOverlayAnim <= 0.f)
        m_chestLootSfxPending = false;
    if (m_warpFlashRemain > 0.f)
        m_warpFlashRemain = std::max(0.f, m_warpFlashRemain - dt);
    if (!m_showMainMenu && !m_paused && m_hitFlashTimer > 0.f)
        m_hitFlashTimer = std::max(0.f, m_hitFlashTimer - dt);

    const bool bossLiveForAudio = !m_showMainMenu
                                  && m_runMode == RunMode::RUNNING
                                  && m_mining.hasLivingBoss();

    if (!m_showMainMenu)
        m_audio->syncBossMusic(bossLiveForAudio);
    else
        m_audio->syncBossMusic(false);

    m_audio->syncMainMenuMusic(m_showMainMenu);

    const bool miningBgmActive =
        !m_showMainMenu && m_activeTab == Tab::MINING && !m_paused;
    m_audio->syncMiningAmbientMusic(miningBgmActive, bossLiveForAudio);

    if (m_paused) return;

    if (m_showMainMenu) {
        m_notifications.update(dt);
        return;
    }

    if (!m_paused) {
        GameUnlockEffects unlockFx(*this, m_mining);
        m_unlockSystem.update(m_state, m_notifications, unlockFx);
        clampActiveTabToVisibility();
    }

    m_oreFusionTimer += dt;
    while (m_oreFusionTimer >= 1.f) {
        m_oreFusionTimer -= 1.f;
        m_state.processOreFusion();
    }

    double creditsEarned = 0.0;
    double oreEarned     = 0.0;
    std::array<double, ORE_TIER_COUNT> oreByTierEarned{};

    if (m_runMode == RunMode::RUNNING && !m_showMainMenu && !m_paused) {
        const bool pauseMining =
            m_state.miningPausesWhenOffMiningTab()
            && m_activeTab != Tab::MINING
            && !m_mining.bossReturnPending();

        const float meteorHpMult = std::max(
            0.1f,
            1.f - m_state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);
        const float meteorAsteroidHp =
            meteorHpMult * m_state.levelHpMult()
            * m_state.difficultyAsteroidHpMult();

        if (!pauseMining) {
            m_mining.tickMeteorShower(dt, m_state, meteorAsteroidHp);
            m_mining.update(dt, m_state, creditsEarned, oreEarned,
                             oreByTierEarned,
                             m_warpCharge);

            if (m_state.bossCrystalPopup > 0.0) {
                pushNotif("+ " + formatBig(m_state.bossCrystalPopup)
                          + " crystals (boss)",
                          sf::Color(200, 150, 255));
                m_state.bossCrystalPopup = 0.0;
            }
            if (m_state.pendingAutoPlinkoBossNotif) {
                m_state.pendingAutoPlinkoBossNotif = false;
                pushNotif("Auto-Plinko unlocked!",
                          sf::Color(120, 220, 255));
            }

            if (m_hitCooldown > 0.f) {
                m_hitCooldown -= dt;
            } else if (m_mining.playerHit()) {
                m_mining.playerTakeDamage(30.f);
                m_hitCooldown = PLAYER_HIT_HP_COOLDOWN;
                m_mining.particles().emitExplosion(
                    m_mining.playerPos(),
                    28.f, sf::Color(255, 120, 80), 18);

                if (m_mining.playerHpZero()) {
                    m_state.loseLife();
                    m_mining.resetPlayerHp();
                    m_hitCooldown = m_state.hitInvulnerabilitySec();
                    m_mining.particles().emitExplosion(
                        m_mining.playerPos(),
                        40.f, sf::Color(255, 80, 60), 30);

                    if (m_state.isGameOver()) {
                        if (!m_audio->playGameOverMusicOnce())
                            m_audio->play(Sfx::GameOver);
                        m_state.gameOver();
                        syncMiningSystemsFromState(true, true);
                        moveRunToBaseState();
                        pushNotif("GAME OVER - terug naar zone 1",
                                  sf::Color(255, 60, 60));
                    } else {
                        if (m_activeTab != Tab::MINING
                            && m_state.difficulty != Difficulty::Easy) {
                            m_hitFlashTimer = 0.4f;
                        }
                        pushNotif("Leven verloren!  " +
                                  std::to_string(m_state.lives) + " over",
                                  sf::Color(255, 120, 60));
                    }
                } else {
                    pushNotif(
                        "Schade! "
                        + std::to_string(static_cast<int>(m_mining.playerHp()))
                        + " HP",
                        sf::Color(255, 180, 60));
                }
            }

            if (m_mining.pullMeteorEasterEgg()) {
                pushNotif(
                    "Meteor Destroyer ontgrendeld - hele shower vernietigd!",
                    sf::Color(255, 200, 120));
            }

            if (m_mining.trySpawnBoss(m_state)) {
                pushNotif("Zone boss - versla voor crystals!",
                          sf::Color(255, 100, 160));
            }

            if (m_zonePlayLevel != m_state.currentLevel) {
                m_zonePlayLevel      = m_state.currentLevel;
                m_zonePlayTime       = 0.f;
                m_keySpawnedThisZone = false;
            }
            m_zonePlayTime += dt;
            if (!m_keySpawnedThisZone
                && m_zonePlayTime >= KEY_ASTEROID_SPAWN_DELAY_SEC) {
                if (m_mining.trySpawnKeyAsteroid(m_state)) {
                    m_keySpawnedThisZone = true;
                    pushNotif("Sleutel-asteroide!",
                              sf::Color(255, 230, 160));
                }
            }

            if (m_mining.pullBossPhase2())
                pushNotif("PHASE 2 - Mini-bosses spawned!",
                          sf::Color(220, 80, 200));
            if (m_mining.pullBossPhase3())
                pushNotif("PHASE 3 - METEOR BARRAGE!",
                          sf::Color(255, 80, 60));

            if (m_mining.pullFuelEmpty()) {
                collectRunOreToState();
                syncMiningSystemsFromState(false);
                moveRunToBaseState();
                pushNotif("Fuel op - terug naar basis",
                          sf::Color(255, 140, 40));
            }

            if (m_mining.pullBossReturnToBase()) {
                collectRunOreToState();
                syncMiningSystemsFromState(false);
                moveRunToBaseState();
                pushNotif(
                    "Basis - loot binnen. Start een nieuwe run of prestige.",
                    sf::Color(160, 220, 255));
            }
        } else {
            if (m_hitCooldown > 0.f)
                m_hitCooldown -= dt;
            // Anders bewegen meteoren niet: tickMeteorShower draait wél in Game.
            m_mining.advanceMeteorsOnly(dt);
            m_mining.tickMeteorShower(dt, m_state, meteorAsteroidHp);
            m_mining.tickMeteorSpawnQueue();
        }

        if (m_activeTab == Tab::MINING && m_state.canWarp()
            && !m_mining.hasLivingBoss()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
                // Eén keer SFX per vasthoud (loslaten reset; los van m_warpCharge≈0 float).
                if (m_warpSfxArmed) {
                    const float pitch =
                        m_audio->warpPitchForChargeDuration(m_state.warpDurationSec());
                    m_audio->play(Sfx::Warp, pitch);
                    m_warpSfxArmed = false;
                }
                m_warpCharge += dt / m_state.warpDurationSec();
                if (m_warpCharge >= 1.f) {
                    // Bij succesvolle warp niet hard afkappen: laat de clip natuurlijk eindigen.
                    m_warpCharge = 0.f;
                    const int    preWarpLevel = m_state.currentLevel;
                    const bool   leavingLegBonus =
                        m_state.isBonusZone
                        && m_state.bonusZoneRarity == OreRarity::LEGENDARY;
                    m_state.doWarp();
                    syncMiningSystemsFromState(false);
                    if (m_state.isBonusZone) {
                        m_mining.spawnBonusZoneKeys(m_state);
                        m_keySpawnedThisZone = true;
                        m_zonePlayTime       = 0.f;
                    }
                    m_warpFlashRemain = WARP_FLASH_DURATION_SEC;
                    m_warpSfxArmed    = true;

                    const float warpRefillChance =
                        m_state.warpFuelRefillChance();
                    if (warpRefillChance > 0.f
                        && randFloat(0.f, 1.f) < warpRefillChance) {
                        m_mining.refillFullFuel(m_state);
                        pushNotif("Warp refill - full fuel!",
                                  sf::Color(120, 220, 160));
                    }

                    if (leavingLegBonus) {
                        const double crystalBonus =
                            3.0
                            + std::floor(
                                std::sqrt(static_cast<double>(preWarpLevel)));
                        m_state.addCrystals(crystalBonus);
                        pushNotif(
                            "Legendary bonus: +"
                                + std::to_string(static_cast<int>(crystalBonus))
                                + " crystals!",
                            sf::Color(255, 170, 0));
                    }
                    if (m_state.isBonusZone) {
                        // Indices 0..5 = COMMON .. LEGENDARY (6 entries each).
                        static const char* const msgs[] = {
                            "Bonus Zone!",
                            "Bonus Zone!",
                            "Rare Bonus Zone!",
                            "EPIC BONUS ZONE!",
                            "MYTHIC BONUS ZONE!",
                            "** LEGENDARY BONUS ZONE **",
                        };
                        static const sf::Color cols[] = {
                            sf::Color(220, 220, 220),
                            sf::Color(80, 200, 80),
                            sf::Color(70, 130, 255),
                            sf::Color(185, 60, 255),
                            sf::Color(220, 50, 50),
                            sf::Color(255, 170, 0),
                        };
                        const int idx = std::clamp(
                            static_cast<int>(m_state.bonusZoneRarity), 0,
                            static_cast<int>(OreRarity::LEGENDARY));
                        pushNotif(strFromNullableUtf8(msgs[idx]) + " - "
                                      + m_state.currentZoneName(),
                                  cols[idx]);
                    } else {
                        pushNotif("Zone " + std::to_string(m_state.currentLevel)
                                      + " - " + m_state.currentZoneName(),
                                  sf::Color(120, 220, 255));
                    }
                }
            } else {
                if (m_warpCharge > 0.001f)
                    m_audio->stopWarpSound();
                m_warpCharge = std::max(0.f, m_warpCharge - dt * 2.f);
                m_warpSfxArmed = true;
            }
        } else {
            if (m_warpCharge > 0.001f)
                m_audio->stopWarpSound();
            m_warpCharge   = 0.f;
            m_warpSfxArmed = true;
        }
    } else {
        if (m_warpCharge > 0.001f)
            m_audio->stopWarpSound();
        m_warpCharge   = 0.f;
        m_warpSfxArmed = true;
    }
    if (m_state.autoPlinkoEnabled() && m_state.ore >= m_state.plinkoBallOreCost())
        m_plinko.updateAuto(dt, m_state, 1.f / m_state.fireRatePerSec(),
                            m_state.autoPlinkoBallsPerTick(),
                            m_state.maxPlinkoBalls());

    {
        double plinkoCredits = 0.0;
        m_plinko.update(dt, plinkoCredits,
                        m_state.creditMult(),
                        m_state.bulkProcess(),
                        m_plinkoParticles);
        creditsEarned += plinkoCredits;
    }
    m_plinkoParticles.update(dt);

    if (m_activeTab == Tab::CHESTS) {
        m_chest.update(
            dt,
            mapPixelToUi(m_window, sf::Mouse::getPosition(m_window)),
            m_state,
            m_chestOverlayAnim > 0.f);
    }

    if (creditsEarned > 0.0) {
        m_state.addCredits(creditsEarned);
    }
    if (oreEarned > 0.0) {
        m_state.addOreTiered(oreByTierEarned, true);
    }

    int keyDrop = m_mining.pullPendingKeyDrop();
    if (keyDrop > 0)
        pushNotif(std::to_string(keyDrop) + " key(s)!",
                  sf::Color(255, 220, 140));

    m_notifications.update(dt);

    m_saveTimer += dt;
    if (m_diskSessionActive && m_saveTimer >= SAVE_INTERVAL) {
        m_saveTimer = 0.f;
        m_state.save(currentSavePath());
        invalidateSaveSlotPreviewCache();
    }

    PlinkoLayoutWatchState& plw = plinkoLayoutWatch();
    int   rows  = m_state.plinkoRows();
    float bonus = m_state.plinkoMultBonus();
    float luck  = m_state.plinkoLuck();
    int   pegUp = m_state.chestPegUpgradeCount();
    int   dupUp = m_state.chestDuplicatorRollCount();
    float slotChest = m_state.chestPlinkoSlotMult();
    if (rows != plw.lastRows || bonus != plw.lastBonus || luck != plw.lastLuck
        || pegUp != plw.lastPegUp || slotChest != plw.lastSlotChest
        || dupUp != plw.lastDupRolls) {
        rebuildPlinko();
        plw.lastRows      = rows;
        plw.lastBonus     = bonus;
        plw.lastLuck       = luck;
        plw.lastPegUp      = pegUp;
        plw.lastSlotChest  = slotChest;
        plw.lastDupRolls   = dupUp;
    }
}

void Game::drawChestOpenOverlay() {
    const sf::Vector2u ws = m_window.getSize();
    const float        W = ws.x > 0 ? static_cast<float>(ws.x) : 1.f;
    const float        H = ws.y > 0 ? static_cast<float>(ws.y) : 1.f;

    const float u =
        1.f - std::clamp(m_chestOverlayAnim / CHEST_OVERLAY_SEC, 0.f, 1.f);
    const float open01 = std::clamp(u * 1.18f, 0.f, 1.f);

    sf::RectangleShape dimBg(sf::Vector2f{ W, H });
    dimBg.setFillColor(sf::Color(4, 6, 14, 218));
    m_window.draw(dimBg);

    const float cx = W * 0.5f;
    const float cy = H * 0.46f;
    const float s  = std::min(W, H) * 0.42f;

    if (m_chestTexLoaded && m_chestTex.getSize().x > 0u) {
        const sf::Vector2u tsz = m_chestTex.getSize();
        const float        target = std::min(W, H) * 0.52f;
        const float        pop    = 0.86f + 0.14f * open01;

        if (chestTexIsAnimatedSheet(tsz)) {
            const sf::Vector2u fpx    = chestSheetFrameSize(tsz);
            const unsigned     openRow = 1;
            const int col =
                std::min(4, static_cast<int>(std::floor(open01 * 5.f - 1e-4f)));
            sf::Sprite spr(m_chestTex);
            spr.setTextureRect(
                chestSheetFrameRect(fpx, static_cast<unsigned>(col), openRow));
            const float tw = static_cast<float>(fpx.x);
            const float th = static_cast<float>(fpx.y);
            const float sc =
                (target / std::max(1.f, std::max(tw, th))) * pop;
            const float pivotY = th * 0.88f;
            spr.setOrigin({ tw * 0.5f, pivotY });
            spr.setPosition({ cx, cy + th * sc * 0.06f });
            spr.setScale({ sc, sc });
            m_window.draw(spr);
        } else {
            // Legacy: enkel frame; kantelt als “deksel” (geen sheet-split).
            const unsigned tw = tsz.x;
            const unsigned th = tsz.y;
            const float    sc =
                (target
                 / std::max(1.f, static_cast<float>(std::max(tw, th))))
                * pop;

            sf::Sprite spr(m_chestTex);
            const float pivotY = static_cast<float>(th) * 0.9f;
            spr.setOrigin({ static_cast<float>(tw) * 0.5f, pivotY });
            spr.setPosition({ cx, cy + static_cast<float>(th) * sc * 0.06f });
            spr.setScale({ sc, sc });
            spr.setRotation(sf::degrees(-16.f * open01));
            m_window.draw(spr);
        }
    } else {
        sf::RectangleShape base(sf::Vector2f{ s * 0.72f, s * 0.5f });
        base.setOrigin({ base.getSize().x * 0.5f, base.getSize().y * 0.5f });
        base.setPosition({ cx, cy + s * 0.1f });
        base.setFillColor(sf::Color(95, 62, 38));
        base.setOutlineColor(sf::Color(210, 170, 90, 220));
        base.setOutlineThickness(std::max(2.f, m_scale * 2.f));
        m_window.draw(base);

        sf::ConvexShape lid;
        lid.setPointCount(4);
        lid.setPoint(0, { -s * 0.38f, 0.f });
        lid.setPoint(1, { 0.f, -s * 0.28f });
        lid.setPoint(2, { s * 0.38f, 0.f });
        lid.setPoint(3, { 0.f, -s * 0.12f });
        lid.setOrigin({ -s * 0.38f, 0.f });
        lid.setPosition({ cx - s * 0.38f, cy - s * 0.12f });
        lid.setRotation(sf::degrees(-82.f * open01));
        lid.setFillColor(sf::Color(120, 78, 48));
        lid.setOutlineColor(sf::Color(255, 215, 130, 230));
        lid.setOutlineThickness(std::max(2.f, m_scale * 2.f));
        m_window.draw(lid);
    }

    const unsigned hintSize = static_cast<unsigned>(
        std::clamp(std::lround(22.f * m_scale), 16L, 36L));
    sf::Text hint(m_font);
    hint.setCharacterSize(hintSize);
    hint.setString("1 key = 1 chest");
    hint.setFillColor(sf::Color(255, 230, 180, 220));
    hint.setOutlineColor(sf::Color(0, 0, 0, 160));
    hint.setOutlineThickness(2.f);
    const sf::FloatRect hb = hint.getLocalBounds();
    hint.setOrigin({ hb.position.x + hb.size.x * 0.5f,
                     hb.position.y + hb.size.y * 0.5f });
    hint.setPosition({ cx, H * 0.82f });
    m_window.draw(hint);
}

void Game::drawChestLootPopup() const {
    if (!m_chestLootPopupActive || m_chestLootPopupRemain <= 0.f
        || m_chestLootPopupText.empty())
        return;

    const sf::Vector2u ws = m_window.getSize();
    const float        W = ws.x > 0 ? static_cast<float>(ws.x) : 1.f;
    const float        H = ws.y > 0 ? static_cast<float>(ws.y) : 1.f;
    const float        cx = W * 0.5f;
    const float        cy = H * 0.46f;

    const float elapsed =
        CHEST_LOOT_POPUP_SEC - m_chestLootPopupRemain;
    const float rise =
        195.f * (1.f - std::exp(-elapsed * 2.5f))
        + 18.f * std::sin(std::min(elapsed, 0.5f) * 12.f)
              * std::exp(-elapsed * 3.f);
    const float y = cy + 40.f - rise;

    const float popT = std::min(1.f, elapsed / 0.24f);
    const float bounce = std::sin(popT * 3.14159265f);
    const float visScale =
        (0.38f + 0.62f * (popT * popT)) * (1.f + 0.14f * bounce * (1.f - popT));

    float alpha = 1.f;
    if (elapsed < 0.09f)
        alpha = elapsed / 0.09f;
    if (m_chestLootPopupRemain < 0.55f)
        alpha = std::min(alpha, m_chestLootPopupRemain / 0.55f);

    const unsigned fs = static_cast<unsigned>(
        std::clamp(std::lround(26.f * m_scale), 18L, 44L));

    sf::Text txt(m_font);
    txt.setCharacterSize(fs);
    txt.setStyle(sf::Text::Bold);
    txt.setString(m_chestLootPopupText);
    const auto           C = m_chestLootPopupColor;
    const std::uint8_t   a =
        static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
    txt.setFillColor(sf::Color(C.r, C.g, C.b, a));
    txt.setOutlineColor(sf::Color(0, 0, 0,
                                   static_cast<std::uint8_t>(
                                       std::clamp(alpha, 0.f, 1.f) * 210.f)));
    txt.setOutlineThickness(2.5f);
    const sf::FloatRect lb = txt.getLocalBounds();
    txt.setOrigin({ lb.position.x + lb.size.x * 0.5f,
                    lb.position.y + lb.size.y * 0.5f });
    txt.setPosition({ cx, y });
    txt.setScale({ visScale, visScale });
    m_window.draw(txt);
}

// ═════════════════════════════════════════════════════════════
//  render
// ═════════════════════════════════════════════════════════════
void Game::render() {
    const sf::Vector2u ws = m_window.getSize();
    const float        rw = ws.x > 0 ? static_cast<float>(ws.x) : 1.f;
    const float        rh = ws.y > 0 ? static_cast<float>(ws.y) : 1.f;
    m_window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, { rw, rh })));

    m_window.clear(sf::Color(6, 8, 18));

    if (m_showMainMenu) {
        drawMainMenu();
        m_notifications.draw(m_window, m_font, std::round(18.f * m_scale));
        m_window.display();
        return;
    }

    drawTabBar();

    const bool drawMiningBackdrop =
        (m_activeTab == Tab::MINING)
        || (m_state.difficulty != Difficulty::Easy);
    if (drawMiningBackdrop) {
        m_mining.draw(m_window, m_state, m_warpCharge, m_warpFlashRemain,
                      m_animClock.getElapsedTime().asSeconds());
    }

    const bool dimOtherTabs =
        m_activeTab != Tab::MINING
        && m_state.difficulty != Difficulty::Easy;
    if (dimOtherTabs) {
        float a = 200.f;
        if (m_hitFlashTimer > 0.f)
            a = 60.f + (140.f * (1.f - m_hitFlashTimer / 0.4f));
        sf::RectangleShape dim(sf::Vector2f{ m_cntW, m_cntH });
        dim.setPosition({ m_cntX, m_cntY });
        dim.setFillColor(sf::Color(
            0, 0, 0, static_cast<std::uint8_t>(std::clamp(a, 0.f, 255.f))));
        m_window.draw(dim);
    }

    drawForegroundTab();

    if (m_activeTab == Tab::MINING && m_runMode == RunMode::RUNNING)
        drawLives();
    drawSidePanel();
    drawSidePanelAuxButtons();
    m_notifications.draw(m_window, m_font, m_tabH + std::round(10.f * m_scale));
    if (m_chestOverlayAnim > 0.f)
        drawChestOpenOverlay();
    if (m_chestLootPopupActive && m_chestLootPopupRemain > 0.f)
        drawChestLootPopup();
    if (m_paused) drawPauseOverlay();
    m_window.display();
}

// ═════════════════════════════════════════════════════════════
//  onMouseClick
// ═════════════════════════════════════════════════════════════
void Game::onMouseClick(sf::Vector2f pos, sf::Mouse::Button btn) {
    if (btn != sf::Mouse::Button::Left) return;
    // Alleen skippen ná minstens één update-tick: dezelfde klik die de chest
    // opent zet eerst m_chestOverlayAnim = CHEST_OVERLAY_SEC en roept daarna
    // onMouseClick aan — dan nog niet skippen.
    if (m_chestOverlayAnim > 0.f
        && m_chestOverlayAnim < CHEST_OVERLAY_SEC - 1e-4f) {
        m_chestOverlayAnim    = 0.f;
        m_chestLootSfxPending = false;
        return;
    }
    if (m_paused) {
          switch (pauseButtonAt(pos)) {
              case PauseButton::RESUME:
                  m_audio->play(Sfx::UiClick);
                  m_paused = false;
                  break;
              case PauseButton::SAVE:
                  m_audio->play(Sfx::UiClick);
                  m_state.save(currentSavePath());
                  invalidateSaveSlotPreviewCache();
                  pushNotif("Opgeslagen.", sf::Color(100, 220, 120));
                  break;
              case PauseButton::MAIN_MENU:
                  m_audio->play(Sfx::UiClick);
                  m_state.save(currentSavePath());
                  invalidateSaveSlotPreviewCache();
                  m_audio->stopGameOverMusic();
                  m_showMainMenu           = true;
                  m_mainMenuPickDifficulty = false;
                  m_paused                 = false;
                  break;
              default: break;
          }
          return;
      }
      // Main menu clicks
    if (m_showMainMenu) {
        handleMainMenuClick(pos);
        return;
    }
    const int vTabs = visibleTabCount();
    if (vTabs > 0 && pos.y >= 0.f && pos.y < m_tabH) {
        const float rowW = m_scrW - m_sideW;
        const int   slot = hitTestHorizTabSlot(pos.x, 0.f, rowW, vTabs);
        if (slot >= 0) {
            m_audio->play(Sfx::UiClick);
            const Tab t = tabFromVisibleSlot(slot);
            if (m_uiFlow)
                m_uiFlow->activateTab(t);
            else
                m_activeTab = t;
            m_prestigeConfirm = false;
            return;
        }
    }

    if (shouldShowRunRetreatButton()
        && runRetreatButtonBounds().contains(pos)) {
        m_audio->play(Sfx::UiClick);
        retreatRunToBase();
        return;
    }

    if (m_activeTab == Tab::MINING && m_runMode == RunMode::BASE) {
        const int zonePick = miningStartZoneAt(pos);
        if (zonePick > 0) {
            m_audio->play(Sfx::UiClick);
            m_selectedStartZone = zonePick;
            return;
        }
        if (miningStartRunBounds().contains(pos)) {
            m_audio->play(Sfx::UiClick);
            if (m_state.lives <= 0)
                m_state.lives = m_state.maxLives();
            m_audio->stopGameOverMusic();
            const int maxPick = std::min(m_state.highestZoneReached,
                                         START_ZONE_PICKER_MAX_ZONES);
            const int startZ =
                std::clamp(m_selectedStartZone, 1, std::max(1, maxPick));
            m_selectedStartZone = startZ;
            if (m_runFlow)
                m_runFlow->startRun(startZ);
            pushNotif(std::string("Run gestart - ") + m_state.levelLabel(),
                      sf::Color(120, 220, 255));
            return;
        }
    }

    switch (m_activeTab) {
        case Tab::SKILL_TREE:
            if (m_skillTree.handleClick(pos, m_state)) {
                syncMiningSystemsFromState(true);
                m_audio->play(Sfx::UiClick);
                pushNotif("Upgrade gekocht!", sf::Color(120, 220, 255));
            }
            break;
        case Tab::PLINKO:   handlePlinkoClick(pos);   break;
        case Tab::PRESTIGE: handlePrestigeClick(pos); break;
        default: break;
    }
}

// ═════════════════════════════════════════════════════════════
//  onMouseScroll
// ═════════════════════════════════════════════════════════════
void Game::onMouseScroll(float delta, sf::Vector2f pos) {
    if (m_activeTab == Tab::SKILL_TREE)
        m_skillTree.handleScroll(-delta, pos);
    if (m_activeTab == Tab::CHESTS)
        m_chest.scrollBy(-delta * 30.f);
}

// ═════════════════════════════════════════════════════════════
//  onKeyPress
// ═════════════════════════════════════════════════════════════
void Game::onKeyPress(sf::Keyboard::Key key, bool ctrl, bool shift) {
    using K = sf::Keyboard::Key;

    // Dev: Ctrl+Shift+C = +1 000 000 credits & +100 keys (ook vóór main menu)
    if (ctrl && shift && key == K::C) {
        constexpr double add = 1'000'000.0;
        m_state.addCredits(add);
        m_state.addKeys(100);
        m_audio->play(Sfx::UiClick);
        pushNotif("+" + formatBig(add) + " credits",
                  sf::Color(120, 255, 160));
        pushNotif("+100 keys", sf::Color(255, 220, 140));
        return;
    }

    if (m_showMainMenu) {
        if (key == K::M) {
            m_audio->setMuted(!m_audio->isMuted());
            pushNotif(m_audio->isMuted() ? "Geluid uit" : "Geluid aan",
                      sf::Color(160, 200, 255));
        }
        return;
    }

    switch (key) {
        case K::Num1:
            if (isTabVisible(Tab::MINING)) {
                if (m_uiFlow)
                    m_uiFlow->activateTab(Tab::MINING);
                else
                    m_activeTab = Tab::MINING;
            }
            break;
        case K::Num2:
            if (isTabVisible(Tab::PLINKO)) {
                if (m_uiFlow)
                    m_uiFlow->activateTab(Tab::PLINKO);
                else
                    m_activeTab = Tab::PLINKO;
            }
            break;
        case K::Num3:
            if (isTabVisible(Tab::SKILL_TREE)) {
                if (m_uiFlow)
                    m_uiFlow->activateTab(Tab::SKILL_TREE);
                else
                    m_activeTab = Tab::SKILL_TREE;
            }
            break;
        case K::Num4:
            if (isTabVisible(Tab::CHESTS)) {
                if (m_uiFlow)
                    m_uiFlow->activateTab(Tab::CHESTS);
                else
                    m_activeTab = Tab::CHESTS;
            }
            break;
        case K::Num5:
            if (isTabVisible(Tab::PRESTIGE)) {
                if (m_uiFlow)
                    m_uiFlow->activateTab(Tab::PRESTIGE);
                else
                    m_activeTab = Tab::PRESTIGE;
            }
            break;

        case K::Space:
            if (m_activeTab == Tab::PLINKO) {
                const double c = m_state.plinkoBallOreCost();
                if (m_state.ore >= c
                    && m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {
                    OreTier paidTier = OreTier::IRON;
                    if (m_state.spendOreForPlinko(c, paidTier))
                        m_plinko.dropBall(oreTierBaseValue(paidTier),
                                          oreTierColor(paidTier));
                }
            }
            break;

        case K::P:
            if (m_diskSessionActive) {
                m_state.save(currentSavePath());
                invalidateSaveSlotPreviewCache();
                m_audio->play(Sfx::UiClick);
                pushNotif("Opgeslagen.", sf::Color(100, 220, 120));
            }
            break;

        case K::M:
            m_audio->setMuted(!m_audio->isMuted());
            pushNotif(m_audio->isMuted() ? "Geluid uit" : "Geluid aan",
                      sf::Color(160, 200, 255));
            break;

        case K::Escape:
            m_paused = !m_paused;
            if (m_paused)
                pushNotif("GEPAUZEERD - Escape om door te gaan",
                          sf::Color(255, 200, 60));
            break;


        default: break;
    }
}

// ═════════════════════════════════════════════════════════════
//  Tab bar
// ═════════════════════════════════════════════════════════════
int Game::visibleTabCount() const {
    int n = 0;
    for (bool v : m_tabVisible)
        if (v) ++n;
    return n;
}

Tab Game::tabFromVisibleSlot(int slot) const {
    int seen = 0;
    for (int i = 0; i < TAB_COUNT; ++i) {
        if (!m_tabVisible[static_cast<std::size_t>(i)])
            continue;
        if (seen == slot)
            return static_cast<Tab>(i);
        ++seen;
    }
    return Tab::MINING;
}

sf::FloatRect Game::tabRect(int visibleIdx) const {
    const int n = std::max(1, visibleTabCount());
    float     tabW = (m_scrW - m_sideW) / static_cast<float>(n);
    return sf::FloatRect({ static_cast<float>(visibleIdx) * tabW, 0.f },
                         { tabW, m_tabH });
}

void Game::setTabVisible(Tab t, bool visible) {
    m_tabVisible[static_cast<int>(t)] = visible;
    clampActiveTabToVisibility();
}

bool Game::isTabVisible(Tab t) const {
    return m_tabVisible[static_cast<int>(t)];
}

void Game::focusSkillTreeTab() {
    if (!isTabVisible(Tab::SKILL_TREE))
        return;
    m_activeTab = Tab::SKILL_TREE;
    m_notifications.clearBadge(static_cast<int>(Tab::SKILL_TREE));
    m_prestigeConfirm = false;
}

void Game::clampActiveTabToVisibility() {
    if (isTabVisible(m_activeTab))
        return;
    m_activeTab = Tab::MINING;
    if (!isTabVisible(m_activeTab)) {
        for (int i = 0; i < TAB_COUNT; ++i) {
            if (m_tabVisible[static_cast<std::size_t>(i)]) {
                m_activeTab = static_cast<Tab>(i);
                return;
            }
        }
    }
}

void Game::resetNewGameUi() {
    m_tabVisible.fill(false);
    m_tabVisible[static_cast<int>(Tab::MINING)] = true;
    m_skillTree.resetScroll();
    m_activeTab       = Tab::MINING;
    m_hitFlashTimer   = 0.f;
    m_prestigeConfirm = false;
}

void Game::drawTabBar() const {
    static const char* labels[TAB_COUNT] = {
        "1  Basis", "2  Plinko", "3  Skills", "4  Chests", "5  Prestige"
    };
    static const sf::Color accents[TAB_COUNT] = {
        sf::Color( 80, 160, 255),
        sf::Color(160, 100, 255),
        sf::Color(255, 200,  60),
        sf::Color(220, 170,  70),
        sf::Color( 80, 220, 140),
    };

    unsigned tabFontSize = static_cast<unsigned>(std::round(15.f * m_scale));

    const int vTabs = visibleTabCount();
    for (int v = 0; v < vTabs; v++) {
        const Tab        t      = tabFromVisibleSlot(v);
        const int        ti     = static_cast<int>(t);
        auto             rect   = tabRect(v);
        const bool       active = (m_activeTab == t);
        const sf::Color& accent = accents[ti];

        sf::RectangleShape bg(sf::Vector2f{ rect.size.x, rect.size.y });
        bg.setPosition(rect.position);
        bg.setFillColor(active
            ? sf::Color(accent.r/5, accent.g/5, accent.b/5, 255)
            : sf::Color(10, 12, 22, 255));
        bg.setOutlineColor(active
            ? sf::Color(accent.r, accent.g, accent.b, 200)
            : sf::Color(30, 36, 60, 150));
        bg.setOutlineThickness(1.f);
        m_window.draw(bg);

        if (active) {
            sf::RectangleShape bar(sf::Vector2f{ rect.size.x - 4.f, 3.f });
            bar.setPosition({ rect.position.x + 2.f,
                              rect.position.y + rect.size.y - 3.f });
            bar.setFillColor(accent);
            m_window.draw(bar);
        }

        drawText(labels[ti],
                 rect.position.x + 14.f,
                 rect.position.y + m_tabH * 0.5f - tabFontSize * 0.5f,
                 tabFontSize,
                 active ? accent : sf::Color(130, 140, 170),
                 active);

        if (m_notifications.hasBadgeFor(ti)) {
            const float br = 7.f;
            sf::CircleShape badge(br);
            badge.setOrigin({ br, br });
            badge.setPosition({
                rect.position.x + rect.size.x - 12.f,
                rect.position.y + 10.f });
            badge.setFillColor(sf::Color(235, 55, 55));
            badge.setOutlineColor(sf::Color(80, 20, 20, 200));
            badge.setOutlineThickness(1.f);
            m_window.draw(badge);
        }
    }

    sf::RectangleShape sep(sf::Vector2f{ m_scrW, 1.f });
    sep.setPosition({ 0.f, m_tabH });
    sep.setFillColor(sf::Color(40, 50, 90, 180));
    m_window.draw(sep);
}

// ═════════════════════════════════════════════════════════════
//  drawForegroundTab  (mining zit al als achtergrond)
// ═════════════════════════════════════════════════════════════
void Game::drawForegroundTab() const {
    const bool st = hubMiningBackdropTransparent();
    switch (m_activeTab) {
        case Tab::MINING:
            if (m_runMode == RunMode::BASE)
                drawMiningBasePanel();
            break;
        case Tab::PLINKO:   drawPlinkoTab(st); break;
        case Tab::SKILL_TREE:
            m_skillTree.draw(m_window, m_state, st);
            break;
        case Tab::CHESTS:   m_chest.draw(m_window, m_state, st); break;
        case Tab::PRESTIGE: drawPrestigeScreen();            break;
    }
}

// ═════════════════════════════════════════════════════════════
//  drawSidePanel
// ═════════════════════════════════════════════════════════════
void Game::drawSidePanel() const {
    float px = m_scrW - m_sideW;
    float py = m_tabH;

    sf::RectangleShape bg(sf::Vector2f{ m_sideW, m_cntH });
    bg.setPosition({ px, py });
    bg.setFillColor(sf::Color(10, 12, 24, 250));
    bg.setOutlineColor(sf::Color(40, 50, 90, 180));
    bg.setOutlineThickness(1.f);
    m_window.draw(bg);

    float tx  = px + 14.f;
    float ty  = py + 16.f;

    // Schaal de fontgroottes mee
    unsigned fHeader = static_cast<unsigned>(std::round(13.f * m_scale));
    unsigned fNormal = static_cast<unsigned>(std::round(14.f * m_scale));
    unsigned fSmall  = static_cast<unsigned>(std::round(12.f * m_scale));
    float    gap     = std::round(24.f * m_scale);
    float    valX    = tx + std::round(110.f * m_scale);

    auto line = [&](const std::string& label,
                    const std::string& val,
                    sf::Color vc = sf::Color(255, 220, 100)) {
        drawText(label, tx,   ty, fNormal, sf::Color(120, 135, 165));
        drawText(val,   valX, ty, fNormal, vc, true);
        ty += gap;
    };

    auto lineWithIcon = [&](auto&& iconFn, const std::string& label,
                            const std::string& val, sf::Color vc) {
        drawText(label, tx, ty, fNormal, sf::Color(120, 135, 165));
        float iconR   = std::round(5.5f * m_scale);
        float iconGap = std::round(17.f * m_scale);
        float icy     = ty + fNormal * 0.52f;
        float icx     = valX - iconGap;
        iconFn(m_window, icx, icy, iconR);
        drawText(val, valX, ty, fNormal, vc, true);
        ty += gap;
    };

    auto divider = [&]() {
        sf::RectangleShape d(sf::Vector2f{ m_sideW - 28.f, 1.f });
        d.setPosition({ tx, ty });
        d.setFillColor(sf::Color(40, 50, 90));
        m_window.draw(d);
        ty += std::round(10.f * m_scale);
    };

    drawText("RESOURCES", tx, ty, fHeader, sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    lineWithIcon(drawPanelCoin,
        "Credits",
        formatBig(m_state.credits),
        sf::Color(255, 215, 70));
    line("Ore", std::to_string(static_cast<long long>(m_state.ore)),
         sf::Color(160, 225, 100));
    lineWithIcon(drawPanelCrystal,
        "Crystals",
        formatBig(m_state.crystals),
        sf::Color(170, 110, 255));
    lineWithIcon(
        [&](sf::RenderTarget& rw, float cx, float cy, float s) {
            drawPanelKey(rw, cx, cy, s,
                         m_keyTexLoaded ? &m_keyTex : nullptr);
        },
        "Keys",
        std::to_string(m_state.keys),
        sf::Color(255, 220, 140));
    line("Run",
         m_runMode == RunMode::BASE ? "Basis" : "Actief",
         sf::Color(140, 200, 255));
    {
        const char* dl = "Normaal";
        sf::Color   dc = sf::Color(140, 190, 255);
        switch (m_state.difficulty) {
            case Difficulty::Easy:
                dl = "Makkelijk";
                dc = sf::Color(120, 220, 160);
                break;
            case Difficulty::Medium: break;
            case Difficulty::Hard:
                dl = "Moeilijk";
                dc = sf::Color(255, 140, 120);
                break;
        }
        line("Moeilijkheid", dl, dc);
    }
    line("Boss bij", "Z " + std::to_string(m_state.nextBossMilestone),
         sf::Color(255, 170, 190));
    {
        const float skipAux = sidePanelAuxReservedHeight();
        if (skipAux > 0.f)
            ty += skipAux;
    }
    divider();

    drawText("STATS", tx, ty, fHeader, sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("Damage",   formatBig(m_state.gunDamage()));
    line("Fire/sec", [&]() {
        std::ostringstream s;
        s << std::fixed << std::setprecision(1) << m_state.fireRatePerSec();
        return s.str();
    }());
    line("Turrets",  std::to_string(m_state.turretCount()));
    line("Crit %",   pct(m_state.critChance()));
    line("Split",    std::to_string(m_state.splitShot()));
    line("Ore val",  formatBig(m_state.oreValueMult()) + "x");
    line("Cr mult",  formatBig(m_state.creditMult())   + "x");
    divider();

    drawText("LIFETIME", tx, ty, fHeader, sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("All cr",    "$ " + formatBig(m_state.totalCredits));
    line("Prestiges", std::to_string(m_state.prestigeCount), sf::Color(120, 220, 160));
    divider();

    double g = m_state.crystalsOnPrestige();
    std::ostringstream css;
    css << "+ " << std::fixed << std::setprecision(0) << g << " on prestige";
    drawText(css.str(), tx, ty, fSmall, sf::Color(150, 90, 240));
}
// ═════════════════════════════════════════════════════════════
//  draw mainmenu
// ═════════════════════════════════════════════════════════════

Game::MainMenuLayout Game::computeMainMenuLayout() const {
    MainMenuLayout L{};
    L.fTitle = static_cast<unsigned>(std::round(48.f * m_scale));
    L.fBtn   = static_cast<unsigned>(std::round(18.f * m_scale));
    L.fSlot  = static_cast<unsigned>(std::round(13.f * m_scale));
    L.fHint  = static_cast<unsigned>(std::round(12.f * m_scale));

    L.btnW    = std::round(300.f * m_scale);
    L.btnH    = std::round(54.f  * m_scale);
    L.gap     = std::round(16.f  * m_scale);
    L.slotH   = std::round(56.f * m_scale);
    L.slotGap = std::round(10.f * m_scale);
    L.slotW   = (L.btnW - L.slotGap * 2.f) / 3.f;
    L.slotX0  = m_scrW * 0.5f - L.btnW * 0.5f;

    sf::Text titleMeas(m_font);
    titleMeas.setString("SPACE ROCK BREAKER");
    titleMeas.setCharacterSize(L.fTitle);
    titleMeas.setStyle(sf::Text::Bold);
    L.titleW = titleMeas.getLocalBounds().size.x;

    const float gTitle = std::round(20.f * m_scale);
    // Extra ruimte: groene slotregels kunnen iets onder de slotbox uitsteken;
    // knoppen zijn opak zodat die tekst niet meer door de vulling schijnt.
    const float gBlock = std::round(26.f * m_scale);
    const float titleLineH = static_cast<float>(L.fTitle);
    const int   btnRows    = m_mainMenuPickDifficulty ? 4 : 3;
    const float totalH =
        titleLineH + gTitle + L.slotH + gBlock
        + static_cast<float>(btnRows) * L.btnH
        + static_cast<float>(btnRows - 1) * L.gap;

    float y0 = (m_scrH - totalH) * 0.5f;
    const float padTop = std::round(16.f * m_scale);
    if (y0 < padTop)
        y0 = padTop;

    L.titleY = y0;
    L.titleX = m_scrW * 0.5f - L.titleW * 0.5f;

    float y = y0 + titleLineH + gTitle;
    L.slotY       = y;
    L.firstBtnTop = y + L.slotH + gBlock;

    return L;
}

void Game::refreshSaveSlotPreviewCache() const {
    if (!m_saveSlotPreviewDirty)
        return;
    for (int s = 0; s < SAVE_SLOT_COUNT; s++) {
        int z = 1;
        double cr = 0.0;
        auto& slot = m_saveSlotPreview[static_cast<std::size_t>(s)];
        slot.hasSave = GameState::peekSaveSlot(saveSlotPath(s), z, cr);
        slot.zone = z;
        slot.credits = cr;
        if (slot.hasSave) {
            std::ostringstream os;
            os << "Z" << z << "  $" << formatBig(cr);
            slot.summary = os.str();
        } else {
            slot.summary = "Leeg";
        }
    }
    m_saveSlotPreviewDirty = false;
}

void Game::invalidateSaveSlotPreviewCache() {
    m_saveSlotPreviewDirty = true;
}

void Game::drawMainMenu() const {
    static const std::string kRuntimeVersion = readRuntimeVersionTag();
    refreshSaveSlotPreviewCache();

    sf::RectangleShape bg(sf::Vector2f{ m_scrW, m_scrH });
    bg.setFillColor(sf::Color(6, 8, 18));
    m_window.draw(bg);

    const MainMenuLayout L = computeMainMenuLayout();
    const float          cx = m_scrW * 0.5f;

    drawText("SPACE ROCK BREAKER",
             L.titleX,
             L.titleY,
             L.fTitle, sf::Color(80, 160, 255), true);
    drawText("Versie: " + kRuntimeVersion,
             std::round(20.f * m_scale),
             std::round(14.f * m_scale),
             std::max(12u, static_cast<unsigned>(std::round(14.f * m_scale))),
             sf::Color(150, 165, 200),
             true);

    for (int s = 0; s < SAVE_SLOT_COUNT; s++) {
        float bx = L.slotX0 + static_cast<float>(s) * (L.slotW + L.slotGap);
        bool  sel = (s == m_saveSlot);

        sf::RectangleShape slotBox(sf::Vector2f{ L.slotW, L.slotH });
        slotBox.setPosition({ bx, L.slotY });
        slotBox.setFillColor(sf::Color(14, 16, 32, sel ? 245 : 210));
        slotBox.setOutlineColor(sel ? sf::Color(120, 200, 255)
                                    : sf::Color(45, 55, 95));
        slotBox.setOutlineThickness(sel ? 2.5f : 1.5f);
        m_window.draw(slotBox);

        drawText("Slot " + std::to_string(s + 1),
                 bx + 10.f,
                 L.slotY + 8.f,
                 L.fSlot,
                 sel ? sf::Color(200, 230, 255) : sf::Color(140, 155, 190),
                 true);

        const auto& slot = m_saveSlotPreview[static_cast<std::size_t>(s)];
        drawText(slot.summary,
                 bx + 10.f,
                 L.slotY + 8.f + static_cast<float>(L.fSlot) + 4.f,
                 L.fHint,
                 sf::Color(110, 200, 140),
                 true);
    }

    if (m_mainMenuPickDifficulty) {
        const float subY =
            L.firstBtnTop - std::round(26.f * m_scale);
        drawText("Kies moeilijkheid (nieuw spel)",
                 cx - std::round(190.f * m_scale),
                 subY,
                 L.fHint,
                 sf::Color(200, 210, 240),
                 true);

        struct DiffBtn {
            const char* label;
            sf::Color   col;
        };
        const DiffBtn diffBtns[] = {
            { "Easy", sf::Color(80, 220, 140) },
            { "Medium", sf::Color(80, 160, 255) },
            { "Hard", sf::Color(255, 100, 80) },
        };

        for (int d = 0; d < 3; d++) {
            float bx = L.slotX0;
            float by =
                L.firstBtnTop + static_cast<float>(d) * (L.btnH + L.gap);

            sf::RectangleShape btn(sf::Vector2f{ L.btnW, L.btnH });
            btn.setPosition({ bx, by });
            btn.setFillColor(sf::Color(14, 16, 32, 255));
            btn.setOutlineColor(diffBtns[d].col);
            btn.setOutlineThickness(2.f);
            m_window.draw(btn);

            drawBoldTextCenteredInRect(m_window,
                                       m_font,
                                       diffBtns[d].label,
                                       bx,
                                       by,
                                       L.btnW,
                                       L.btnH,
                                       L.fBtn,
                                       diffBtns[d].col,
                                       0.f);
        }

        const float byBack =
            L.firstBtnTop + 3.f * (L.btnH + L.gap);
        sf::RectangleShape backBtn(sf::Vector2f{ L.btnW, L.btnH });
        backBtn.setPosition({ L.slotX0, byBack });
        backBtn.setFillColor(sf::Color(14, 16, 32, 255));
        backBtn.setOutlineColor(sf::Color(140, 145, 170));
        backBtn.setOutlineThickness(2.f);
        m_window.draw(backBtn);
        drawBoldTextCenteredInRect(m_window,
                                   m_font,
                                   "Terug",
                                   L.slotX0,
                                   byBack,
                                   L.btnW,
                                   L.btnH,
                                   L.fBtn,
                                   sf::Color(180, 185, 210),
                                   0.f);

        const std::string foot =
            "Klik een slot hierboven | M = geluid aan/uit";
        sf::Text footMeas(m_font);
        footMeas.setString(foot);
        footMeas.setCharacterSize(L.fHint);
        footMeas.setStyle(sf::Text::Bold);
        const float footW = footMeas.getLocalBounds().size.x;
        drawText(foot,
                 cx - footW * 0.5f,
                 m_scrH - std::round(36.f * m_scale),
                 L.fHint,
                 sf::Color(90, 100, 140),
                 true);
        return;
    }

    const std::string slotStatusTxt =
        m_saveSlotPreview[static_cast<std::size_t>(m_saveSlot)].summary;

    struct BtnDef { std::string label; sf::Color color; };
    const BtnDef btns[] = {
        { "Doorgaan",     sf::Color( 80, 160, 255) },
        { "Nieuw Spel",   sf::Color( 80, 220, 120) },
        { "Afsluiten",    sf::Color(255, 100,  80) },
    };

    const float padX = std::round(18.f * m_scale);

    for (int i = 0; i < 3; i++) {
        float bx = L.slotX0;
        float by = L.firstBtnTop + static_cast<float>(i) * (L.btnH + L.gap);

        sf::RectangleShape btn(sf::Vector2f{ L.btnW, L.btnH });
        btn.setPosition({ bx, by });
        btn.setFillColor(sf::Color(14, 16, 32, 255));
        btn.setOutlineColor(btns[i].color);
        btn.setOutlineThickness(2.f);
        m_window.draw(btn);

        const float midY = by + L.btnH * 0.5f;

        if (i <= 1) {
            drawText(btns[i].label,
                     bx + padX,
                     midY - static_cast<float>(L.fBtn) * 0.5f,
                     L.fBtn, btns[i].color, true);

            sf::Text hintMeas(m_font);
            hintMeas.setString(slotStatusTxt);
            hintMeas.setCharacterSize(L.fHint);
            hintMeas.setStyle(sf::Text::Bold);
            const float sw = hintMeas.getLocalBounds().size.x;
            drawText(slotStatusTxt,
                     bx + L.btnW - padX - sw,
                     midY - static_cast<float>(L.fHint) * 0.5f,
                     L.fHint,
                     sf::Color(110, 200, 140),
                     true);
        } else {
            drawBoldTextCenteredInRect(m_window,
                                       m_font,
                                       btns[i].label,
                                       bx,
                                       by,
                                       L.btnW,
                                       L.btnH,
                                       L.fBtn,
                                       btns[i].color,
                                       0.f);
        }
    }

    const std::string foot = "Klik een slot hierboven | M = geluid aan/uit";
    sf::Text          footMeas(m_font);
    footMeas.setString(foot);
    footMeas.setCharacterSize(L.fHint);
    footMeas.setStyle(sf::Text::Bold);
    const float footW = footMeas.getLocalBounds().size.x;
    drawText(foot,
             cx - footW * 0.5f,
             m_scrH - std::round(36.f * m_scale),
             L.fHint,
             sf::Color(90, 100, 140),
             true);
}

// ═════════════════════════════════════════════════════════════
//  Plinko tab
// ═════════════════════════════════════════════════════════════
void Game::rebuildPlinko() {
    if (m_audio)
        m_plinko.setAudioBus(m_audio);
    float bx = m_cntX + 30.f;
    float by = m_cntY + 20.f;
    float bw = m_cntW - 60.f;
    float bh = m_cntH - 90.f;
    float pegR = PLINKO_PEG_RADIUS;
    m_plinko.build(m_state.plinkoRows(), bx, by, bw, bh,
                   m_state.plinkoMultBonus(), m_state.plinkoLuck(),
                   m_scale, pegR, 1.f, m_state.chestPlinkoSlotMult());
    m_plinko.syncGoldenPegChestRarities(
        m_state.levelOfChest(ChestUpgradeID::PLINKO_PEG_SIZE));
    m_plinko.syncDuplicatorPegChestRarities(
        m_state.levelOfChest(ChestUpgradeID::PLINKO_DUPLICATOR_PEG));
}

void Game::drawPlinkoTab(bool seeThroughMiningBackdrop) const {
    sf::RectangleShape bg(sf::Vector2f{ m_cntW, m_cntH });
    bg.setPosition({ m_cntX, m_cntY });
    bg.setFillColor(
        hubBackdropTint(sf::Color(6, 8, 18, 255), seeThroughMiningBackdrop));
    m_window.draw(bg);

    m_plinko.draw(m_window, const_cast<sf::Font&>(m_font),
                  seeThroughMiningBackdrop);
    m_plinkoParticles.draw(m_window, const_cast<sf::Font&>(m_font));

    const float statusY = m_cntY + m_cntH - std::round(52.f * m_scale);

    unsigned fs = static_cast<unsigned>(std::round(14.f * m_scale));
    std::ostringstream os, bs;
    os << "Ore: " << static_cast<long long>(m_state.ore);
    {
        const double c = m_state.plinkoBallOreCost();
        if (c > 1.0001)
            os << "   (bal: " << formatBig(c) << " ore)";
    }
    bs << "Balls: " << m_plinko.ballsAlive() << " / " << m_state.maxPlinkoBalls();

    drawText(os.str(), m_cntX + 16.f, statusY, fs, sf::Color(170, 225, 110));
    drawText(bs.str(), m_cntX + 16.f, statusY + fs + 4.f, fs,
             sf::Color(150, 130, 195));
    if (m_state.chestDuplicatorRollCount() > 0) {
        const float ly = statusY + (fs + 4.f) * 2.f;
        drawText(
            "Cyan ring = duplicator peg (extra bal, zelfde ore; keten max 4)",
            m_cntX + 16.f, ly,
            static_cast<unsigned>(std::max(11, static_cast<int>(fs) - 2)),
            sf::Color(120, 210, 220));
    }
}

void Game::handleMainMenuClick(sf::Vector2f pos) {
    const MainMenuLayout L = computeMainMenuLayout();

    for (int s = 0; s < SAVE_SLOT_COUNT; s++) {
        float bx = L.slotX0 + static_cast<float>(s) * (L.slotW + L.slotGap);
        sf::FloatRect slotR({ bx, L.slotY }, { L.slotW, L.slotH });
        if (slotR.contains(pos)) {
            m_audio->play(Sfx::UiClick);
            m_saveSlot = s;
            invalidateSaveSlotPreviewCache();
            return;
        }
    }

    if (m_mainMenuPickDifficulty) {
        for (int d = 0; d < 3; d++) {
            sf::FloatRect r(
                { L.slotX0,
                  L.firstBtnTop + static_cast<float>(d) * (L.btnH + L.gap) },
                { L.btnW, L.btnH });
            if (!r.contains(pos))
                continue;
            m_audio->play(Sfx::UiClick);
            m_state.reset();
            resetNewGameUi();
            m_audio->stopGameOverMusic();
            m_state.difficulty =
                static_cast<Difficulty>(static_cast<int>(Difficulty::Easy) + d);
            m_state.lives = m_state.maxLives();
            m_plinko.resetGoldenPegRarityState();
            syncMiningSystemsFromState(true, true);
            resetPlinkoLayoutWatch();
            resetZoneKeyState();
            m_runMode              = RunMode::BASE;
            m_mainMenuPickDifficulty = false;
            m_state.save(currentSavePath());
            invalidateSaveSlotPreviewCache();
            m_diskSessionActive    = true;
            m_showMainMenu         = false;
            {
                GameUnlockEffects unlockFx(*this, m_mining);
                m_unlockSystem.update(m_state, m_notifications, unlockFx);
                clampActiveTabToVisibility();
            }
            pushNotif("Nieuw spel - slot " +
                          std::to_string(m_saveSlot + 1) + ".",
                      sf::Color(180, 180, 180));
            return;
        }
        sf::FloatRect rBack(
            { L.slotX0, L.firstBtnTop + 3.f * (L.btnH + L.gap) },
            { L.btnW, L.btnH });
        if (rBack.contains(pos)) {
            m_audio->play(Sfx::UiClick);
            m_mainMenuPickDifficulty = false;
        }
        return;
    }

    for (int i = 0; i < 3; i++) {
        sf::FloatRect r(
            { L.slotX0, L.firstBtnTop + static_cast<float>(i) * (L.btnH + L.gap) },
            { L.btnW, L.btnH });

        if (r.contains(pos)) {
            if (i == 0) {                          // Doorgaan
                if (m_state.load(currentSavePath())) {
                    m_audio->stopGameOverMusic();
                    m_audio->play(Sfx::UiClick);
                    pushNotif("Save geladen (slot " +
                                    std::to_string(m_saveSlot + 1) + ")!",
                                sf::Color(100, 220, 120));
                    m_plinko.resetGoldenPegRarityState();
                    rebuildPlinko();
                    resetZoneKeyState();
                    m_runMode             = RunMode::BASE;
                    m_diskSessionActive   = true;
                    m_showMainMenu        = false;
                    invalidateSaveSlotPreviewCache();
                    syncMiningSystemsFromState(false);
                    resetPlinkoLayoutWatch();
                    GameUnlockEffects unlockFx(*this, m_mining);
                    m_unlockSystem.update(m_state, m_notifications, unlockFx);
                    clampActiveTabToVisibility();
                } else {
                    pushNotif("Geen save in dit slot.",
                              sf::Color(255, 100, 80));
                }
            } else if (i == 1) {                   // Nieuw Spel
                m_audio->play(Sfx::UiClick);
                m_mainMenuPickDifficulty = true;
            } else if (i == 2) {                   // Afsluiten
                if (m_diskSessionActive)
                    m_state.save(currentSavePath());
                invalidateSaveSlotPreviewCache();
                m_window.close();
            }
            return;
        }
    }
}

void Game::handlePlinkoClick(sf::Vector2f pos) {
    sf::FloatRect btnRect = plinkoSideDropButtonBounds();
    if (!btnRect.contains(pos)) return;

    const double c = m_state.plinkoBallOreCost();
    if (m_state.ore >= c && m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {
        OreTier paidTier = OreTier::IRON;
        if (m_state.spendOreForPlinko(c, paidTier))
            m_plinko.dropBall(oreTierBaseValue(paidTier),
                              oreTierColor(paidTier));
    } else if (m_state.ore < c) {
        pushNotif("Geen ore!", sf::Color(255, 100, 80));
    }
}

// ═════════════════════════════════════════════════════════════
//  Prestige screen
// ═════════════════════════════════════════════════════════════
void Game::drawPrestigeScreen() const {
    const bool st = hubMiningBackdropTransparent();

    sf::RectangleShape bg(sf::Vector2f{ m_cntW, m_cntH });
    bg.setPosition({ m_cntX, m_cntY });
    bg.setFillColor(
        hubBackdropTint(sf::Color(6, 8, 20, 255), st));
    m_window.draw(bg);

    unsigned fTitle  = static_cast<unsigned>(std::round(24.f * m_scale));
    unsigned fNormal = static_cast<unsigned>(std::round(15.f * m_scale));
    unsigned fSmall  = static_cast<unsigned>(std::round(12.f * m_scale));
    unsigned fCard   = static_cast<unsigned>(std::round(14.f * m_scale));
    unsigned fSub    = static_cast<unsigned>(std::round(11.f * m_scale));

    float cx = m_cntX + m_cntW * 0.5f;
    float ty = m_cntY + 28.f;

    drawText("=== PRESTIGE ===",
             cx - fTitle * 5.f, ty, fTitle,
             sf::Color(160, 100, 255), true);
    ty += fTitle * 2.f;

    double gain = m_state.crystalsOnPrestige();
    std::ostringstream gs;
    gs << "Gain:  +" << std::fixed << std::setprecision(0) << gain << " Crystals";
    drawText(gs.str(), cx - 150.f, ty, fNormal, sf::Color(200, 160, 255));
    ty += fNormal + 8.f;
    drawText("( floor( sqrt( lifetime credits / 1000 ) ) )",
             cx - 165.f, ty, fSub, sf::Color(100, 85, 130));
    ty += fNormal + 10.f;

    auto divLine = [&]() {
        sf::RectangleShape d(sf::Vector2f{ m_cntW - 80.f, 1.f });
        d.setPosition({ m_cntX + 40.f, ty });
        d.setFillColor(hubBackdropTint(sf::Color(60, 40, 90, 255), st));
        m_window.draw(d);
        ty += 14.f;
    };
    divLine();

    drawText("RESETS OP PRESTIGE:",
             cx - 180.f, ty, fCard, sf::Color(255, 110, 90), true);
    ty += fCard + 8.f;
    drawText("Credits  |  Ore  |  Alle regular upgrades",
             cx - 185.f, ty, fSmall, sf::Color(200, 155, 155));
    ty += fSmall + 6.f;

    int kept = m_state.prestigeLevels[
        static_cast<int>(PrestigeUpgradeID::CRYSTAL_RETENTION)] * 2;
    std::ostringstream rs;
    rs << "Deep Retention: top " << kept << " upgrades op half level bewaard";
    drawText(rs.str(), cx - 185.f, ty, fSmall, sf::Color(140, 215, 150));
    ty += fSmall + 14.f;
    divLine();

    drawText("PERMANENTE UPGRADES  (kost: Crystals)",
             cx - 185.f, ty, fCard, sf::Color(180, 140, 255), true);
    ty += fCard + 14.f;

    float cardW = m_cntW - 80.f;
    float cardH = std::round(58.f * m_scale);
    float cardX = m_cntX + 40.f;

    int pCount = static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT);

    for (int i = 0; i < pCount; i++) {
        auto        pid  = static_cast<PrestigeUpgradeID>(i);
        const auto& def  = GameState::prestigeCatalog[i];
        int         lv   = m_state.levelOf(pid);
        double      cost = m_state.costOf(pid);
        bool        can  = m_state.canBuy(pid);

        sf::RectangleShape card(sf::Vector2f{ cardW, cardH });
        card.setPosition({ cardX, ty });
        card.setFillColor(hubBackdropTint(
            can ? sf::Color(30, 18, 55, 230) : sf::Color(18, 16, 30, 200),
            st));
        card.setOutlineColor(hubBackdropTint(
            can ? sf::Color(160, 100, 255, 180)
                : sf::Color(50, 40, 75, 110),
            st));
        card.setOutlineThickness(1.f);
        m_window.draw(card);

        drawText(def.name + "  Lv " + std::to_string(lv),
                 cardX + 10.f, ty + 7.f,
                 fCard, sf::Color(210, 200, 255), true);
        drawText(def.description,
                 cardX + 10.f, ty + 7.f + fCard + 4.f,
                 fSub, sf::Color(130, 120, 170));

        std::ostringstream cs;
        cs << std::fixed << std::setprecision(1) << cost << " crystals";
        drawText(cs.str(),
                 cardX + cardW - std::round(170.f * m_scale),
                 ty + cardH * 0.5f - fSmall * 0.5f,
                 fSmall,
                 can ? sf::Color(200, 155, 255) : sf::Color(90, 75, 110),
                 true);

        ty += cardH + 6.f;
        if (ty > m_cntY + m_cntH - 100.f) break;
    }

    ty += 10.f;
    divLine();

    float pbW = std::round(240.f * m_scale);
    float pbH = std::round(46.f  * m_scale);
    float pbX = cx - pbW * 0.5f;
    float pbY = m_cntY + m_cntH - pbH - 14.f;

    sf::RectangleShape pb(sf::Vector2f{ pbW, pbH });
    pb.setPosition({ pbX, pbY });
    pb.setFillColor(hubBackdropTint(
        m_prestigeConfirm ? sf::Color(120, 20, 200, 240)
                          : sf::Color(50, 20, 90, 220),
        st));
    pb.setOutlineColor(hubBackdropTint(
        m_prestigeConfirm ? sf::Color(220, 100, 255, 255)
                          : sf::Color(140, 80, 220, 180),
        st));
    pb.setOutlineThickness(2.f);
    m_window.draw(pb);

    drawText(m_prestigeConfirm ? "BEVESTIG PRESTIGE?" : "PRESTIGE  (+crystals)",
             pbX + 14.f, pbY + pbH * 0.5f - fCard * 0.5f,
             fCard, sf::Color(220, 180, 255), true);
}

// ═════════════════════════════════════════════════════════════
//  handlePrestigeClick
// ═════════════════════════════════════════════════════════════
void Game::handlePrestigeClick(sf::Vector2f pos) {
    float cx  = m_cntX + m_cntW * 0.5f;
    float pbW = std::round(240.f * m_scale);
    float pbH = std::round(46.f  * m_scale);
    float pbX = cx - pbW * 0.5f;
    float pbY = m_cntY + m_cntH - pbH - 14.f;

    if (sf::FloatRect({ pbX, pbY }, { pbW, pbH }).contains(pos)) {
        m_audio->play(Sfx::UiClick);
        if (!m_prestigeConfirm) {
            m_prestigeConfirm = true;
        } else {
            double gained = m_state.crystalsOnPrestige();
            m_state.doPrestige();
            syncMiningSystemsFromState(true, true);
            m_prestigeConfirm = false;

            std::ostringstream ns;
            ns << "Geprestiged! +" << std::fixed
               << std::setprecision(0) << gained << " crystals";
            pushNotif(ns.str(), sf::Color(200, 130, 255));
        }
        return;
    }

    m_prestigeConfirm = false;

    unsigned fCard = static_cast<unsigned>(std::round(14.f * m_scale));
    float ty   = m_cntY + 28.f + fCard * 2.f + (fCard + 8.f)
               + (fCard + 10.f) + 14.f + (fCard + 8.f)
               + (12.f + 6.f) + (12.f + 14.f) + 14.f + (fCard + 14.f);
    float cardW = m_cntW - 80.f;
    float cardH = std::round(58.f * m_scale);
    float cardX = m_cntX + 40.f;

    int pCount = static_cast<int>(PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT);

    for (int i = 0; i < pCount; i++) {
        auto pid = static_cast<PrestigeUpgradeID>(i);
        sf::FloatRect cardRect({ cardX, ty }, { cardW, cardH });

        if (cardRect.contains(pos) && m_state.canBuy(pid)) {
            m_state.buy(pid);
            m_audio->play(Sfx::UiClick);
            pushNotif("Permanente upgrade gekocht!", sf::Color(200, 150, 255));
            return;
        }
        ty += cardH + 6.f;
        if (ty > m_cntY + m_cntH - 100.f) break;
    }
}

// ═════════════════════════════════════════════════════════════
//  drawPauseOverlay
// ═════════════════════════════════════════════════════════════
void Game::drawPauseOverlay() const {
    sf::RectangleShape overlay(sf::Vector2f{ m_scrW, m_scrH });
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    m_window.draw(overlay);

    unsigned fTitle = static_cast<unsigned>(std::round(36.f * m_scale));
    unsigned fBtn   = static_cast<unsigned>(std::round(16.f * m_scale));

    float cx     = m_scrW * 0.5f;
    float cy     = m_scrH * 0.5f;
    float btnW   = std::round(260.f * m_scale);
    float btnH   = std::round(48.f  * m_scale);
    float gap    = std::round(14.f  * m_scale);
    float startY = cy - btnH * 0.5f;

    drawText("GEPAUZEERD",
             cx - fTitle * 3.f,
             startY - fTitle * 2.f,
             fTitle, sf::Color(255, 200, 60), true);

    struct BtnDef { std::string label; sf::Color color; };
    const BtnDef btns[] = {
        { "Doorgaan  [Escape]", sf::Color( 80, 160, 255) },
        { "Opslaan   [P]",      sf::Color( 80, 220, 120) },
        { "Main Menu",          sf::Color(255, 100,  80) },
    };

    for (int i = 0; i < 3; i++) {
        float bx = cx - btnW * 0.5f;
        float by = startY + i * (btnH + gap);

        sf::RectangleShape btn(sf::Vector2f{ btnW, btnH });
        btn.setPosition({ bx, by });
        btn.setFillColor(sf::Color(14, 16, 32, 230));
        btn.setOutlineColor(btns[i].color);
        btn.setOutlineThickness(2.f);
        m_window.draw(btn);

        drawText(btns[i].label,
                 bx + 20.f,
                 by + btnH * 0.5f - fBtn * 0.5f,
                 fBtn, btns[i].color, true);
    }

    unsigned fHint = static_cast<unsigned>(std::round(12.f * m_scale));
    drawText("M = geluid aan/uit",
             cx - 70.f * m_scale,
             startY + 3.f * (btnH + gap) + 6.f,
             fHint,
             sf::Color(120, 130, 170),
             true);
}

Game::PauseButton Game::pauseButtonAt(sf::Vector2f pos) const {
    float cx   = m_scrW * 0.5f;
    float cy   = m_scrH * 0.5f;
    float btnW = std::round(260.f * m_scale);
    float btnH = std::round(48.f  * m_scale);
    float gap  = std::round(14.f  * m_scale);
    float startY = cy - btnH * 0.5f;

    for (int i = 0; i < 3; i++) {
        sf::FloatRect r(
            { cx - btnW * 0.5f, startY + i * (btnH + gap) },
            { btnW, btnH });
        if (r.contains(pos)) {
            if (i == 0) return PauseButton::RESUME;
            if (i == 1) return PauseButton::SAVE;
            if (i == 2) return PauseButton::MAIN_MENU;
        }
    }
    return PauseButton::NONE;
}


// ═════════════════════════════════════════════════════════════
//  Notifications
// ═════════════════════════════════════════════════════════════
void Game::pushNotif(const std::string& text, sf::Color color, float holdSec) {
    m_notifications.push(text, color, holdSec, -1);
}

// ═════════════════════════════════════════════════════════════
//  Basis-paneel (mining-tab wanneer niet in run)
// ═════════════════════════════════════════════════════════════
sf::FloatRect Game::miningStartZoneButtonBounds(int zone) const {
    const StartZonePickerLayout layout = makeStartZonePickerLayout(
        m_state.highestZoneReached, m_scale, m_cntY, m_cntH);
    const float cx = m_cntX + m_cntW * 0.5f;
    return startZoneButtonRect(zone, cx, layout);
}

int Game::miningStartZoneAt(sf::Vector2f pos) const {
    const StartZonePickerLayout layout = makeStartZonePickerLayout(
        m_state.highestZoneReached, m_scale, m_cntY, m_cntH);
    for (int z = 1; z <= layout.visibleCount; ++z) {
        const sf::FloatRect rb = miningStartZoneButtonBounds(z);
        if (rb.size.x > 0.f && rb.contains(pos))
            return z;
    }
    return 0;
}

void Game::drawMiningStartZoneButtons(int selectedZone) const {
    const StartZonePickerLayout layout = makeStartZonePickerLayout(
        m_state.highestZoneReached, m_scale, m_cntY, m_cntH);
    if (layout.visibleCount <= 0)
        return;

    const float cx = m_cntX + m_cntW * 0.5f;
    const float labelY = layout.blockTopY - std::round(18.f * m_scale);
    const unsigned fLeg =
        static_cast<unsigned>(std::round(13.f * m_scale));
    drawText("Startzone (bereikt via warp):",
             cx - fLeg * 5.8f,
             labelY,
             fLeg,
             sf::Color(150, 175, 210));

    const unsigned fBtn =
        static_cast<unsigned>(std::round(13.f * m_scale));
    for (int z = 1; z <= layout.visibleCount; ++z) {
        const sf::FloatRect rb = startZoneButtonRect(z, cx, layout);
        const bool selected = (z == selectedZone);
        sf::RectangleShape btn(rb.size);
        btn.setPosition(rb.position);
        btn.setFillColor(selected
                             ? sf::Color(55, 90, 150, 240)
                             : sf::Color(28, 38, 68, 220));
        btn.setOutlineColor(selected
                                ? sf::Color(140, 210, 255, 240)
                                : sf::Color(70, 100, 150, 180));
        btn.setOutlineThickness(selected ? 2.f : 1.f);
        m_window.draw(btn);

        std::string label = "Z" + std::to_string(z);
        drawText(label,
                 rb.position.x + rb.size.x * 0.5f - fBtn * 0.55f,
                 rb.position.y + rb.size.y * 0.5f - fBtn * 0.45f,
                 fBtn,
                 selected ? sf::Color(230, 245, 255)
                          : sf::Color(180, 195, 220),
                 true);
    }
}

sf::FloatRect Game::miningStartRunBounds() const {
    const StartZonePickerLayout layout = makeStartZonePickerLayout(
        m_state.highestZoneReached, m_scale, m_cntY, m_cntH);
    float bw = std::round(320.f * m_scale);
    float bh = std::round(54.f * m_scale);
    float cx = m_cntX + m_cntW * 0.5f;
    float top = startRunButtonTopY(layout, m_scale, m_cntY, m_cntH, bh);
    return sf::FloatRect({ cx - bw * 0.5f, top }, { bw, bh });
}

void Game::drawMiningBasePanel() const {
    sf::RectangleShape bg(sf::Vector2f{ m_cntW, m_cntH });
    bg.setPosition({ m_cntX, m_cntY });
    bg.setFillColor(sf::Color(8, 10, 22));
    bg.setOutlineColor(sf::Color(50, 70, 120, 140));
    bg.setOutlineThickness(1.f);
    m_window.draw(bg);

    unsigned fTitle = static_cast<unsigned>(std::round(26.f * m_scale));
    unsigned fBody  = static_cast<unsigned>(std::round(15.f * m_scale));
    float    cx      = m_cntX + m_cntW * 0.5f;
    float    ty      = m_cntY + m_cntH * 0.22f;

    drawText("BASIS", cx - fTitle * 2.2f, ty, fTitle,
             sf::Color(160, 200, 255), true);
    ty += fTitle + 18.f;

    std::ostringstream b1;
    b1 << "Volgende zone-boss: zone " << m_state.nextBossMilestone;
    drawText(b1.str(), m_cntX + 40.f, ty, fBody, sf::Color(200, 210, 235));
    ty += 28.f;

    std::ostringstream b2;
    b2 << "Crystals: " << formatBig(m_state.crystals)
       << "  -  prestige in tab 5";
    drawText(b2.str(), m_cntX + 40.f, ty, fBody, sf::Color(180, 160, 240));
    ty += 28.f;

    drawText("Versla de boss voor bonus crystals + grote ore-drop.",
             m_cntX + 40.f, ty, fBody, sf::Color(150, 170, 200));
    ty += 40.f;

    drawText("Na de boss: loot oprapen, dan automatisch terug hier.",
             m_cntX + 40.f, ty, fBody, sf::Color(130, 150, 185));
    ty += 36.f;

    if (m_state.chestPegUpgradeCount() > 0) {
        unsigned fLeg = static_cast<unsigned>(std::round(12.f * m_scale));
        drawText("Plinko peg - kleur = rarity (hit bonus):",
                 m_cntX + 40.f, ty, fLeg, sf::Color(175, 195, 225));
        ty += std::round(20.f * m_scale);

        struct PegLegendRow { OreRarity id; const char* txt; };
        static const PegLegendRow legend[] = {
            { OreRarity::COMMON,    "Common - geen bonus" },
            { OreRarity::UNCOMMON,  "Uncommon +0.5x" },
            { OreRarity::RARE,      "Rare +1x" },
            { OreRarity::EPIC,      "Epic +2x" },
            { OreRarity::MYTHIC,    "Mythic +4x" },
            { OreRarity::LEGENDARY, "Legendary +8x" },
        };
        const float dotR     = std::max(4.f, 5.f * m_scale);
        const float lineStep = std::round(18.f * m_scale);
        for (const auto& row : legend) {
            const float dx = m_cntX + 40.f + dotR;
            sf::CircleShape dot(dotR);
            dot.setOrigin({ dotR, dotR });
            dot.setPosition({ dx, ty + dotR * 0.85f });
            dot.setFillColor(PlinkoPegRarity::fillColor(row.id));
            dot.setOutlineColor(sf::Color(255, 255, 255, 60));
            dot.setOutlineThickness(1.f);
            m_window.draw(dot);
            drawText(row.txt, dx + dotR + 8.f, ty, fLeg,
                     sf::Color(200, 210, 230));
            ty += lineStep;
        }
        ty += std::round(12.f * m_scale);
    }

    const int maxPick = std::min(m_state.highestZoneReached,
                                 START_ZONE_PICKER_MAX_ZONES);
    const int pickZone =
        std::clamp(m_selectedStartZone, 1, std::max(1, maxPick));
    drawMiningStartZoneButtons(pickZone);

    sf::FloatRect rb = miningStartRunBounds();
    sf::RectangleShape btn(rb.size);
    btn.setPosition(rb.position);
    btn.setFillColor(sf::Color(35, 55, 100, 230));
    btn.setOutlineColor(sf::Color(100, 180, 255, 220));
    btn.setOutlineThickness(2.f);
    m_window.draw(btn);

    unsigned fBtn = static_cast<unsigned>(std::round(17.f * m_scale));
    drawText("START RUN",
             rb.position.x + rb.size.x * 0.5f - fBtn * 3.2f,
             rb.position.y + rb.size.y * 0.5f - fBtn * 0.45f,
             fBtn, sf::Color(220, 240, 255), true);
}

bool Game::shouldShowRunRetreatButton() const {
    return m_runMode == RunMode::RUNNING
        && m_state.showsRetreatToBaseOnOtherTabs()
        && m_activeTab != Tab::MINING
        && !m_showMainMenu;
}

bool Game::shouldShowPlinkoSideDrop() const {
    return m_activeTab == Tab::PLINKO && !m_showMainMenu;
}

float Game::sidePanelResourcesBottomY() const {
    const float py     = m_tabH;
    float       ty     = py + 16.f;
    const unsigned fHeader =
        static_cast<unsigned>(std::round(13.f * m_scale));
    const float gap = std::round(24.f * m_scale);
    ty += static_cast<float>(fHeader) + gap + 2.f;
    ty += 7.f * gap;
    return ty;
}

float Game::sidePanelAfterPrestigeHintBottomY() const {
    const float py     = m_tabH;
    float       ty     = py + 16.f;
    const unsigned fHeader =
        static_cast<unsigned>(std::round(13.f * m_scale));
    const unsigned fSmall =
        static_cast<unsigned>(std::round(12.f * m_scale));
    const float gap = std::round(24.f * m_scale);

    ty += static_cast<float>(fHeader) + gap + 2.f;
    ty += 7.f * gap;

    {
        const float skipAux = sidePanelAuxReservedHeight();
        if (skipAux > 0.f)
            ty += skipAux;
    }
    ty += std::round(10.f * m_scale);

    ty += static_cast<float>(fHeader) + gap + 2.f;
    ty += 7.f * gap;
    ty += std::round(10.f * m_scale);

    ty += static_cast<float>(fHeader) + gap + 2.f;
    ty += 2.f * gap;
    ty += std::round(10.f * m_scale);

    return ty + static_cast<float>(fSmall);
}

float Game::sidePanelAuxReservedHeight() const {
    const float bh  = std::round(44.f * m_scale);
    const float pad = std::round(8.f * m_scale);
    // Plinko-DROP staat onderaan bij "+ … on prestige" — geen gat tussen
    // resources en STATS. Alleen "Terug naar basis" houdt het oude blok.
    if (shouldShowPlinkoSideDrop())
        return 0.f;
    if (shouldShowRunRetreatButton())
        return pad + bh;
    return 0.f;
}

float Game::sidePanelAuxButtonsBaseY() const {
    return sidePanelResourcesBottomY() + std::round(8.f * m_scale);
}

sf::FloatRect Game::plinkoSideDropButtonBounds() const {
    if (!shouldShowPlinkoSideDrop())
        return sf::FloatRect({ 0.f, 0.f }, { 0.f, 0.f });
    const float px   = m_scrW - m_sideW + 14.f;
    const float bw   = m_sideW - 28.f;
    const float bh   = std::round(44.f * m_scale);
    const float topY =
        sidePanelAfterPrestigeHintBottomY() + std::round(6.f * m_scale);
    return sf::FloatRect({ px, topY }, { bw, bh });
}

float Game::plinkoUnlockHintBelowDropBlockHeight() const {
    if (!shouldShowPlinkoSideDrop())
        return 0.f;
    const float gap    = std::round(8.f * m_scale);
    const float panelH = std::round(88.f * m_scale);
    return gap + panelH;
}

void Game::drawPlinkoUnlockHintBelowDrop(bool seeThroughMiningBackdrop) const {
    if (!shouldShowPlinkoSideDrop())
        return;

    const sf::FloatRect drop   = plinkoSideDropButtonBounds();
    const float         gap    = std::round(8.f * m_scale);
    const float         panelH = std::round(88.f * m_scale);
    const float         panelT = drop.position.y + drop.size.y + gap;
    const float         padIn  = std::round(8.f * m_scale);
    const float         panelW = drop.size.x;

    const UnlockNextHint hint = computeUnlockNextHint(m_state);
    sf::RectangleShape   hintBg(sf::Vector2f{ panelW, panelH });
    hintBg.setPosition({ drop.position.x, panelT });
    hintBg.setFillColor(hubBackdropTint(sf::Color(14, 18, 36, 220),
                                        seeThroughMiningBackdrop));
    hintBg.setOutlineColor(hubBackdropTint(sf::Color(70, 90, 140, 160),
                                           seeThroughMiningBackdrop));
    hintBg.setOutlineThickness(1.f);
    m_window.draw(hintBg);

    const unsigned fsZ = static_cast<unsigned>(std::round(11.f * m_scale));
    const unsigned fsH = static_cast<unsigned>(std::round(12.f * m_scale));
    float          ty  = panelT + std::round(6.f * m_scale);
    const float    tx  = drop.position.x + padIn;

    std::string zoneLine = m_state.levelLabel();
    zoneLine += (m_runMode == RunMode::RUNNING) ? " - run" : " - basis";
    drawText(zoneLine, tx, ty, fsZ, sf::Color(140, 200, 255));
    ty += static_cast<float>(fsZ) + 2.f;
    drawText(hint.heading, tx, ty, fsZ, sf::Color(160, 165, 190));
    ty += static_cast<float>(fsZ) + 1.f;
    drawText(hint.phaseName, tx, ty, fsH, sf::Color(255, 215, 130));
    ty += static_cast<float>(fsH) + 3.f;
    drawText(hint.progressDetail, tx, ty, fsZ, sf::Color(185, 200, 220));

    const float barW = panelW - 2.f * padIn;
    const float barH = std::round(5.f * m_scale);
    const float barY = panelT + panelH - barH - std::round(6.f * m_scale);
    const float barX = tx;
    sf::RectangleShape barBg(sf::Vector2f{ barW, barH });
    barBg.setPosition({ barX, barY });
    barBg.setFillColor(sf::Color(25, 30, 50, 220));
    m_window.draw(barBg);
    const float fillW = std::max(0.f, (barW - 2.f) * hint.progress01);
    if (fillW > 0.5f) {
        sf::RectangleShape barFill(sf::Vector2f{ fillW, barH - 2.f });
        barFill.setPosition({ barX + 1.f, barY + 1.f });
        barFill.setFillColor(sf::Color(90, 200, 140, 240));
        m_window.draw(barFill);
    }
}

sf::FloatRect Game::runRetreatButtonBounds() const {
    if (!shouldShowRunRetreatButton())
        return sf::FloatRect({ 0.f, 0.f }, { 0.f, 0.f });
    const float px = m_scrW - m_sideW + 14.f;
    const float bw = m_sideW - 28.f;
    const float bh = std::round(44.f * m_scale);
    const float rowGap = std::round(8.f * m_scale);
    float       y    = 0.f;
    if (shouldShowPlinkoSideDrop()) {
        const sf::FloatRect drop = plinkoSideDropButtonBounds();
        y = drop.position.y + drop.size.y + plinkoUnlockHintBelowDropBlockHeight()
            + rowGap;
    } else {
        y = sidePanelAuxButtonsBaseY();
    }
    return sf::FloatRect({ px, y }, { bw, bh });
}

void Game::drawSidePanelAuxButtons() const {
    const bool st = hubMiningBackdropTransparent();
    if (shouldShowPlinkoSideDrop()) {
        sf::FloatRect rb = plinkoSideDropButtonBounds();
        const double  oreCost = m_state.plinkoBallOreCost();
        bool          canDrop = m_state.ore >= oreCost
            && m_plinko.ballsAlive() < m_state.maxPlinkoBalls();
        sf::RectangleShape btn(rb.size);
        btn.setPosition(rb.position);
        btn.setFillColor(canDrop ? sf::Color(55, 25, 95, 235)
                                 : sf::Color(28, 22, 40, 180));
        btn.setOutlineColor(canDrop ? sf::Color(160, 100, 255, 230)
                                    : sf::Color(55, 45, 75, 110));
        btn.setOutlineThickness(2.f);
        m_window.draw(btn);
        unsigned fs =
            static_cast<unsigned>(std::round(14.f * m_scale));
        drawText(canDrop ? "DROP [Space]"
                         : (m_state.ore < oreCost ? "Geen ore" : "Balls vol"),
                 rb.position.x + 12.f,
                 rb.position.y + rb.size.y * 0.5f - fs * 0.5f,
                 fs,
                 canDrop ? sf::Color(210, 170, 255)
                         : sf::Color(90, 80, 110),
                 true);
        drawPlinkoUnlockHintBelowDrop(st);
    }
    if (shouldShowRunRetreatButton()) {
        sf::FloatRect rb = runRetreatButtonBounds();
        sf::RectangleShape btn(rb.size);
        btn.setPosition(rb.position);
        btn.setFillColor(sf::Color(28, 42, 72, 235));
        btn.setOutlineColor(sf::Color(120, 190, 255, 220));
        btn.setOutlineThickness(2.f);
        m_window.draw(btn);
        unsigned fs =
            static_cast<unsigned>(std::round(15.f * m_scale));
        drawText("Terug naar basis (run stoppen)",
                 rb.position.x + 14.f,
                 rb.position.y + rb.size.y * 0.5f - fs * 0.45f,
                 fs, sf::Color(200, 230, 255), true);
    }
}

void Game::retreatRunToBase() {
    collectRunOreToState();
    syncMiningSystemsFromState(false);
    moveRunToBaseState();
    pushNotif("Basis - run gestopt, loot verzameld.",
              sf::Color(160, 220, 255));
}

void Game::syncMiningSystemsFromState(bool rebuildPlinkoBoard,
                                      bool clearMiningField) {
    if (!m_runFlow)
        return;
    m_runFlow->syncFromState(rebuildPlinkoBoard, clearMiningField);
}

void Game::collectRunOreToState() {
    if (!m_runFlow)
        return;
    m_runFlow->collectRunOreToState();
}

void Game::moveRunToBaseState() {
    if (!m_runFlow)
        return;
    m_runFlow->moveToBase();
}

// ═════════════════════════════════════════════════════════════
//  drawText
// ═════════════════════════════════════════════════════════════
void Game::drawText(const std::string& str,
                     float x, float y,
                     unsigned size,
                     sf::Color color,
                     bool bold) const {
    sf::Text txt(m_font);
    txt.setCharacterSize(size);
    txt.setString(str);
    if (bold)
        txt.setStyle(sf::Text::Bold);
    txt.setFillColor(color);
    txt.setPosition({ x, y });
    m_window.draw(txt);
}

// ═════════════════════════════════════════════════════════════
//  formatBig / pct  — helpers
// ═════════════════════════════════════════════════════════════
std::string Game::formatBig(double v) const {
    return ::formatBig(v);
}

std::string Game::pct(float v) const {
    return ::pct(v);
}
