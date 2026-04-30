#include "Game.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
Game::Game()
    : m_window(
        sf::VideoMode::getDesktopMode(),
        WINDOW_TITLE)
{
    m_window.setPosition(sf::Vector2i(0, 0));
    m_window.setFramerateLimit(TARGET_FPS);

    if (!m_font.openFromFile("assets/font.ttf"))
        m_font.openFromFile("C:/Windows/Fonts/arial.ttf");


    initLayout();
    reinitSystems();

    if (m_state.load(SAVE_FILE))
        pushNotif("Save geladen!", sf::Color(100, 220, 120));
    else
        pushNotif("Nieuw spel gestart.", sf::Color(180, 180, 180));

    m_mining.syncTurrets(m_state);
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
    m_cntW   = m_scrW - m_sideW;
    m_cntH   = m_scrH - m_tabH;
}

// ─────────────────────────────────────────────────────────────
//  reinitSystems  — herinitialiseer subsystems met nieuwe layout
// ─────────────────────────────────────────────────────────────
void Game::reinitSystems() {
    m_mining.init(m_font, m_cntX, m_cntY, m_cntW, m_cntH);
    m_shop.init  (m_font, m_cntX, m_cntY, m_cntW, m_cntH, m_scale);  // ← m_scale erbij
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
    m_state.save(SAVE_FILE);
}

// ═════════════════════════════════════════════════════════════
//  processEvents
// ═════════════════════════════════════════════════════════════
void Game::processEvents() {
    while (const std::optional event = m_window.pollEvent()) {

        if (m_activeTab == Tab::SHOP) {
            bool bought = m_shop.handleEvent(*event, m_state);
            if (bought) {
                m_mining.syncTurrets(m_state);
                pushNotif("Upgrade gekocht!", sf::Color(120, 220, 255));
            }
        }

        if (event->is<sf::Event::Closed>()) {
            m_state.save(SAVE_FILE);
            m_window.close();
        }
        else if (const auto* e =
                 event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f pos(
                static_cast<float>(e->position.x),
                static_cast<float>(e->position.y));
            onMouseClick(pos, e->button);
        }
        else if (const auto* e =
                 event->getIf<sf::Event::MouseWheelScrolled>()) {
            sf::Vector2f pos(
                static_cast<float>(e->position.x),
                static_cast<float>(e->position.y));
            onMouseScroll(e->delta, pos);
        }
        else if (const auto* e =
                 event->getIf<sf::Event::KeyPressed>()) {
            onKeyPress(e->code);
        }
    }
}
// draw lives
void Game::drawLives() const {
    float x = m_cntX + m_cntW - std::round(160.f * m_scale);
    float y = m_cntY + std::round(8.f * m_scale);
    float r = std::round(10.f * m_scale);
    float gap = std::round(28.f * m_scale);

    for (int i = 0; i < GameState::MAX_LIVES; i++) {
        sf::CircleShape heart(r);
        heart.setOrigin({ r, r });
        heart.setPosition({ x + i * gap, y + r });
        heart.setFillColor(i < m_state.lives
            ? sf::Color(255, 60, 80)
            : sf::Color(60, 20, 30));
        heart.setOutlineColor(sf::Color(200, 40, 60, 160));
        heart.setOutlineThickness(1.5f);
        m_window.draw(heart);
    }
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Game::update(float dt) {
    if (m_paused) return;

    double creditsEarned = 0.0;
    double oreEarned     = 0.0;

    m_mining.update(dt, m_state, creditsEarned, oreEarned);
    // ── Lives check ───────────────────────────────────────
    if (m_hitCooldown > 0.f) {
        m_hitCooldown -= dt;
    } else if (m_mining.playerHit()) {
        m_hitCooldown = HIT_COOLDOWN;
        m_state.loseLife();
        m_mining.particles().emitExplosion(
            sf::Vector2f(m_cntW * 0.5f, m_cntH * 0.5f),
            40.f, sf::Color(255, 80, 60), 30);

        if (m_state.isGameOver()) {
            m_state.gameOver();
            m_mining.clearAll();
            m_mining.syncTurrets(m_state);
            rebuildPlinko();
            pushNotif("GAME OVER — terug naar zone 1",
                      sf::Color(255, 60, 60));
        } else {
            pushNotif("Leven verloren!  " +
                      std::to_string(m_state.lives) + " over",
                      sf::Color(255, 120, 60));
        }
    }
    // ── Warp charge (Space vasthouden in Mining tab) ───────
        if (m_activeTab == Tab::MINING && m_state.canWarp()) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
                m_warpCharge += dt / WARP_CHARGE_TIME;
                if (m_warpCharge >= 1.f) {
                    m_warpCharge = 0.f;
                    m_state.doWarp();
                    m_mining.clearAll();
                    m_mining.syncTurrets(m_state);
                    pushNotif("Zone " + std::to_string(m_state.currentLevel) + "!",
                              sf::Color(120, 220, 255));
                }
            } else {
                m_warpCharge = std::max(0.f, m_warpCharge - dt * 2.f);
            }
        } else {
            m_warpCharge = 0.f;
        }
    if (m_state.autoPlinkoEnabled() && m_state.ore >= 1.0)
        m_plinko.updateAuto(dt, m_state.ore, 1.f / m_state.fireRatePerSec());

    {
        double plinkoCredits = 0.0;
        m_plinko.update(dt, plinkoCredits,
                        m_state.creditMult(),
                        m_state.bulkProcess(),
                        m_mining.particles());
        creditsEarned += plinkoCredits;
    }

    if (m_activeTab == Tab::SHOP) {
        sf::Vector2i mp = sf::Mouse::getPosition(m_window);
        m_shop.update(
            sf::Vector2f(static_cast<float>(mp.x),
                         static_cast<float>(mp.y)),
            m_state);
    }

    if (creditsEarned > 0.0) {
        m_state.credits      += creditsEarned;
        m_state.totalCredits += creditsEarned;
    }
    if (oreEarned > 0.0) {
        m_state.ore      += oreEarned;
        m_state.totalOre += oreEarned;
        m_state.oreThisLevel += oreEarned;

    }

    updateNotifs(dt);

    m_saveTimer += dt;
    if (m_saveTimer >= SAVE_INTERVAL) {
        m_saveTimer = 0.f;
        m_state.save(SAVE_FILE);
    }

    static int   lastRows  = -1;
    static float lastBonus = -1.f;
    static float lastLuck  = -1.f;
    int   rows  = m_state.plinkoRows();
    float bonus = m_state.plinkoMultBonus();
    float luck  = m_state.plinkoLuck();
    if (rows != lastRows || bonus != lastBonus || luck != lastLuck) {
        rebuildPlinko();
        lastRows  = rows;
        lastBonus = bonus;
        lastLuck  = luck;
    }
}

// ═════════════════════════════════════════════════════════════
//  render
// ═════════════════════════════════════════════════════════════
void Game::render() {
    m_window.clear(sf::Color(6, 8, 18));

    if (m_showMainMenu) {
        drawMainMenu();
        m_window.display();
        return;
    }

    drawTabBar();
    drawActiveTab();
    if (m_activeTab == Tab::MINING)
    drawLives();
    drawSidePanel();
    drawNotifs();
    if (m_paused) drawPauseOverlay();
    m_window.display();
}

// ═════════════════════════════════════════════════════════════
//  onMouseClick
// ═════════════════════════════════════════════════════════════
void Game::onMouseClick(sf::Vector2f pos, sf::Mouse::Button btn) {
    if (btn != sf::Mouse::Button::Left) return;
    if (m_paused) {
          switch (pauseButtonAt(pos)) {
              case PauseButton::RESUME:
                  m_paused = false;
                  break;
              case PauseButton::SAVE:
                  m_state.save(SAVE_FILE);
                  pushNotif("Opgeslagen.", sf::Color(100, 220, 120));
                  break;
              case PauseButton::MAIN_MENU:
                  m_state.save(SAVE_FILE);
                  m_showMainMenu = true;
                  m_paused       = false;
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
    for (int i = 0; i < TAB_COUNT; i++) {
        if (tabRect(i).contains(pos)) {
            m_activeTab       = static_cast<Tab>(i);
            m_prestigeConfirm = false;
            return;
        }
    }

    switch (m_activeTab) {
        case Tab::PLINKO:   handlePlinkoClick(pos);   break;
        case Tab::PRESTIGE: handlePrestigeClick(pos); break;
        default: break;
    }
}

// ═════════════════════════════════════════════════════════════
//  onMouseScroll
// ═════════════════════════════════════════════════════════════
void Game::onMouseScroll(float delta, sf::Vector2f /*pos*/) {
    if (m_activeTab == Tab::SHOP)
        m_shop.scrollBy(-delta * 30.f);
}

// ═════════════════════════════════════════════════════════════
//  onKeyPress
// ═════════════════════════════════════════════════════════════
void Game::onKeyPress(sf::Keyboard::Key key) {
    using K = sf::Keyboard::Key;

    switch (key) {
        case K::Num1: m_activeTab = Tab::MINING;   break;
        case K::Num2: m_activeTab = Tab::PLINKO;   break;
        case K::Num3: m_activeTab = Tab::SHOP;     break;
        case K::Num4: m_activeTab = Tab::PRESTIGE; break;

        case K::Space:
            if (m_activeTab == Tab::PLINKO) {
                if (m_state.ore >= 1.0 &&
                    m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {
                    m_state.ore -= 1.0;
                    if (m_state.ore < 0.0) m_state.ore = 0.0;
                    m_plinko.dropBall(m_state.ore >= 1.0 ? 1.0 : 0.0);

                }
            }
            break;

        case K::P:
            m_state.save(SAVE_FILE);
            pushNotif("Opgeslagen.", sf::Color(100, 220, 120));
            break;

        case K::Escape:
            m_paused = !m_paused;
            if (m_paused)
                pushNotif("GEPAUZEERD  —  Escape om door te gaan",
                          sf::Color(255, 200, 60));
            break;


        default: break;
    }
}

// ═════════════════════════════════════════════════════════════
//  Tab bar
// ═════════════════════════════════════════════════════════════
sf::FloatRect Game::tabRect(int idx) const {
    float tabW = (m_scrW - m_sideW) / static_cast<float>(TAB_COUNT);
    return sf::FloatRect({ idx * tabW, 0.f }, { tabW, m_tabH });
}

void Game::drawTabBar() const {
    const char* labels[TAB_COUNT] = {
        "1  Mining", "2  Plinko", "3  Shop", "4  Prestige"
    };
    const sf::Color accents[TAB_COUNT] = {
        sf::Color( 80, 160, 255),
        sf::Color(160, 100, 255),
        sf::Color(255, 200,  60),
        sf::Color( 80, 220, 140),
    };

    unsigned tabFontSize = static_cast<unsigned>(std::round(15.f * m_scale));

    for (int i = 0; i < TAB_COUNT; i++) {
        auto  rect   = tabRect(i);
        bool  active = (m_activeTab == static_cast<Tab>(i));
        auto& accent = accents[i];

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

        drawText(labels[i],
                 rect.position.x + 14.f,
                 rect.position.y + m_tabH * 0.5f - tabFontSize * 0.5f,
                 tabFontSize,
                 active ? accent : sf::Color(130, 140, 170),
                 active);
    }

    sf::RectangleShape sep(sf::Vector2f{ m_scrW, 1.f });
    sep.setPosition({ 0.f, m_tabH });
    sep.setFillColor(sf::Color(40, 50, 90, 180));
    m_window.draw(sep);
}

// ═════════════════════════════════════════════════════════════
//  drawActiveTab
// ═════════════════════════════════════════════════════════════
void Game::drawActiveTab() const {
    switch (m_activeTab) {
        case Tab::MINING:   m_mining.draw(m_window, m_state, m_warpCharge); break;
        case Tab::PLINKO:   drawPlinkoTab();                   break;
        case Tab::SHOP:     m_shop.draw(m_window, m_state);    break;
        case Tab::PRESTIGE: drawPrestigeScreen();              break;
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

    auto divider = [&]() {
        sf::RectangleShape d(sf::Vector2f{ m_sideW - 28.f, 1.f });
        d.setPosition({ tx, ty });
        d.setFillColor(sf::Color(40, 50, 90));
        m_window.draw(d);
        ty += std::round(10.f * m_scale);
    };

    drawText("RESOURCES", tx, ty, fHeader, sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("Credits",  "$ " + formatBig(m_state.credits),  sf::Color(255, 215, 70));
    line("Ore",      std::to_string(static_cast<long long>(m_state.ore)), sf::Color(160, 225, 100));
    line("Crystals", formatBig(m_state.crystals),         sf::Color(170, 110, 255));
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

void Game::drawMainMenu() const {
    sf::RectangleShape bg(sf::Vector2f{ m_scrW, m_scrH });
    bg.setFillColor(sf::Color(6, 8, 18));
    m_window.draw(bg);

    unsigned fTitle = static_cast<unsigned>(std::round(48.f * m_scale));
    unsigned fBtn   = static_cast<unsigned>(std::round(18.f * m_scale));

    float cx     = m_scrW * 0.5f;
    float cy     = m_scrH * 0.5f;
    float btnW   = std::round(300.f * m_scale);
    float btnH   = std::round(54.f  * m_scale);
    float gap    = std::round(16.f  * m_scale);

    drawText("SPACE ROCK BREAKER",
             cx - fTitle * 4.5f,
             cy - fTitle * 3.f,
             fTitle, sf::Color(80, 160, 255), true);

    struct BtnDef { std::string label; sf::Color color; };
    const BtnDef btns[] = {
        { "Doorgaan",     sf::Color( 80, 160, 255) },
        { "Nieuw Spel",   sf::Color( 80, 220, 120) },
        { "Afsluiten",    sf::Color(255, 100,  80) },
    };

    for (int i = 0; i < 3; i++) {
        float bx = cx - btnW * 0.5f;
        float by = cy - btnH * 0.5f + i * (btnH + gap);

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
}

// ═════════════════════════════════════════════════════════════
//  Plinko tab
// ═════════════════════════════════════════════════════════════
void Game::rebuildPlinko() {
    float bx = m_cntX + 30.f;
    float by = m_cntY + 20.f;
    float bw = m_cntW - 60.f;
    float bh = m_cntH - 90.f;
    m_plinko.build(m_state.plinkoRows(), bx, by, bw, bh,
                   m_state.plinkoMultBonus(), m_state.plinkoLuck(),
                   m_scale);   // ← nieuw
}

void Game::drawPlinkoTab() const {
    sf::RectangleShape bg(sf::Vector2f{ m_cntW, m_cntH });
    bg.setPosition({ m_cntX, m_cntY });
    bg.setFillColor(sf::Color(6, 8, 18));
    m_window.draw(bg);

    m_plinko.draw(m_window, const_cast<sf::Font&>(m_font));

    float btnW = std::round(180.f * m_scale);
    float btnH = std::round(44.f  * m_scale);
    float btnX = m_cntX + m_cntW * 0.5f - btnW * 0.5f;
    float btnY = m_cntY + m_cntH - btnH - 10.f;

    unsigned fs = static_cast<unsigned>(std::round(14.f * m_scale));

    bool canDrop = m_state.ore >= 1.0 &&
                   m_plinko.ballsAlive() < m_state.maxPlinkoBalls();

    sf::RectangleShape btn(sf::Vector2f{ btnW, btnH });
    btn.setPosition({ btnX, btnY });
    btn.setFillColor(canDrop ? sf::Color(55, 25, 95, 235)
                             : sf::Color(28, 22, 40, 180));
    btn.setOutlineColor(canDrop ? sf::Color(160, 100, 255, 230)
                                : sf::Color(55, 45, 75, 110));
    btn.setOutlineThickness(2.f);
    m_window.draw(btn);

    drawText(canDrop ? "DROP  [Space]"
                     : (m_state.ore < 1.0 ? "Geen ore" : "Balls vol"),
             btnX + 12.f, btnY + btnH * 0.5f - fs * 0.5f, fs,
             canDrop ? sf::Color(210, 170, 255) : sf::Color(90, 80, 110),
             true);

    std::ostringstream os, bs;
    os << "Ore: " << static_cast<long long>(m_state.ore);
    bs << "Balls: " << m_plinko.ballsAlive() << " / " << m_state.maxPlinkoBalls();

    drawText(os.str(), m_cntX + 16.f, btnY + 6.f,              fs, sf::Color(170, 225, 110));
    drawText(bs.str(), m_cntX + 16.f, btnY + 6.f + fs + 4.f,   fs, sf::Color(150, 130, 195));
}

void Game::handleMainMenuClick(sf::Vector2f pos) {
    float cx     = m_scrW * 0.5f;
    float cy     = m_scrH * 0.5f;
    float btnW   = std::round(300.f * m_scale);
    float btnH   = std::round(54.f  * m_scale);
    float gap    = std::round(16.f  * m_scale);

    for (int i = 0; i < 3; i++) {
        sf::FloatRect r(
            { cx - btnW * 0.5f, cy - btnH * 0.5f + i * (btnH + gap) },
            { btnW, btnH });

        if (r.contains(pos)) {
            if (i == 0) {                          // Doorgaan
                if (m_state.load(SAVE_FILE))
                    pushNotif("Save geladen!", sf::Color(100, 220, 120));
                else
                    pushNotif("Geen save gevonden.", sf::Color(255, 100, 80));
                m_showMainMenu = false;
            } else if (i == 1) {                   // Nieuw Spel
                m_state.reset();
                m_mining.clearAll();
                m_mining.syncTurrets(m_state);
                rebuildPlinko();
                m_showMainMenu = false;
                pushNotif("Nieuw spel gestart.", sf::Color(180, 180, 180));
            } else if (i == 2) {                   // Afsluiten
                m_state.save(SAVE_FILE);
                m_window.close();
            }
            return;
        }
    }
}

void Game::handlePlinkoClick(sf::Vector2f pos) {
    float btnW = std::round(180.f * m_scale);
    float btnH = std::round(44.f  * m_scale);
    float btnX = m_cntX + m_cntW * 0.5f - btnW * 0.5f;
    float btnY = m_cntY + m_cntH - btnH - 10.f;
    sf::FloatRect btnRect({ btnX, btnY }, { btnW, btnH });

    if (!btnRect.contains(pos)) return;

    if (m_state.ore >= 1.0 &&
        m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {
        m_state.ore -= 1.0;
        if (m_state.ore < 0.0) m_state.ore = 0.0;
        m_plinko.dropBall(m_state.ore >= 1.0 ? 1.0 : 0.0);
    } else if (m_state.ore < 1.0) {
        pushNotif("Geen ore!", sf::Color(255, 100, 80));
    }
}

// ═════════════════════════════════════════════════════════════
//  Prestige screen
// ═════════════════════════════════════════════════════════════
void Game::drawPrestigeScreen() const {
    sf::RectangleShape bg(sf::Vector2f{ m_cntW, m_cntH });
    bg.setPosition({ m_cntX, m_cntY });
    bg.setFillColor(sf::Color(6, 8, 20));
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
        d.setFillColor(sf::Color(60, 40, 90));
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
        card.setFillColor(can ? sf::Color(30, 18, 55, 230)
                              : sf::Color(18, 16, 30, 200));
        card.setOutlineColor(can ? sf::Color(160, 100, 255, 180)
                                 : sf::Color(50, 40, 75, 110));
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
    pb.setFillColor(m_prestigeConfirm ? sf::Color(120, 20, 200, 240)
                                      : sf::Color(50, 20, 90, 220));
    pb.setOutlineColor(m_prestigeConfirm ? sf::Color(220, 100, 255)
                                         : sf::Color(140, 80, 220, 180));
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
        if (!m_prestigeConfirm) {
            m_prestigeConfirm = true;
        } else {
            double gained = m_state.crystalsOnPrestige();
            m_state.doPrestige();
            m_mining.clearAll();
            m_mining.syncTurrets(m_state);
            rebuildPlinko();
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
void Game::pushNotif(const std::string& text, sf::Color color) {
    for (auto& n : m_notifs) {
        if (!n.alive) {
            n.text     = text;
            n.color    = color;
            n.lifetime = n.maxLifetime;
            n.yOffset  = 0.f;
            n.alive    = true;
            return;
        }
    }
    auto& n   = m_notifs[0];
    n.text    = text;
    n.color   = color;
    n.lifetime= n.maxLifetime;
    n.yOffset = 0.f;
    n.alive   = true;
}

void Game::updateNotifs(float dt) {
    for (auto& n : m_notifs) {
        if (!n.alive) continue;
        n.lifetime -= dt;
        n.yOffset  += 28.f * dt;
        if (n.lifetime <= 0.f) n.alive = false;
    }
}

void Game::drawNotifs() const {
    unsigned fs    = static_cast<unsigned>(std::round(13.f * m_scale));
    float    notifW = std::round(340.f * m_scale);
    float    notifH = std::round(26.f  * m_scale);
    float    baseX  = m_scrW - m_sideW - notifW - 10.f;
    float    baseY  = m_scrH - 40.f;
    int      slot   = 0;

    for (const auto& n : m_notifs) {
        if (!n.alive) continue;

        float alpha = clamp(n.lifetime / n.maxLifetime, 0.f, 1.f);
        float y     = baseY - slot * (notifH + 4.f) - n.yOffset;

        sf::RectangleShape bg(sf::Vector2f{ notifW, notifH });
        bg.setPosition({ baseX, y });
        bg.setFillColor(sf::Color(12, 14, 28,
            static_cast<uint8_t>(180 * alpha)));
        bg.setOutlineColor(sf::Color(n.color.r, n.color.g, n.color.b,
            static_cast<uint8_t>(160 * alpha)));
        bg.setOutlineThickness(1.f);
        m_window.draw(bg);

        sf::Text txt(m_font);
        txt.setCharacterSize(fs);
        txt.setString(n.text);
        txt.setFillColor(sf::Color(n.color.r, n.color.g, n.color.b,
            static_cast<uint8_t>(255 * alpha)));
        txt.setPosition({ baseX + 8.f, y + notifH * 0.5f - fs * 0.5f });
        m_window.draw(txt);

        slot++;
    }
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
    txt.setFillColor(color);
    txt.setPosition({ x, y });
    if (bold) txt.setStyle(sf::Text::Bold);
    m_window.draw(txt);
}

// ═════════════════════════════════════════════════════════════
//  formatBig / pct  — helpers
// ═════════════════════════════════════════════════════════════
std::string Game::formatBig(double v) const {
    if (v >= 1e12) { std::ostringstream s; s << std::fixed << std::setprecision(2) << v/1e12 << "T"; return s.str(); }
    if (v >= 1e9 ) { std::ostringstream s; s << std::fixed << std::setprecision(2) << v/1e9  << "B"; return s.str(); }
    if (v >= 1e6 ) { std::ostringstream s; s << std::fixed << std::setprecision(2) << v/1e6  << "M"; return s.str(); }
    if (v >= 1e3 ) { std::ostringstream s; s << std::fixed << std::setprecision(1) << v/1e3  << "K"; return s.str(); }
    std::ostringstream s; s << static_cast<long long>(v); return s.str();
}

std::string Game::pct(float v) const {
    std::ostringstream s;
    s << std::fixed << std::setprecision(1) << v * 100.f << "%";
    return s.str();
}
