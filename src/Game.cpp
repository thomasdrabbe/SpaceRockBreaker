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
    : m_window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
               WINDOW_TITLE,
               sf::Style::Fullscreen)
{
    m_window.setFramerateLimit(TARGET_FPS);

    if (!m_font.loadFromFile("assets/font.ttf"))
        m_font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    m_mining.init(m_font,
                  CONTENT_X, CONTENT_Y,
                  CONTENT_W, CONTENT_H);

    m_shop.init(m_font,
                CONTENT_X, CONTENT_Y,
                CONTENT_W, CONTENT_H);

    rebuildPlinko();

    if (m_state.load(SAVE_FILE))
        pushNotif("Save loaded!", sf::Color(100, 220, 120));

    m_mining.syncTurrets(m_state);
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
    sf::Event ev;
    while (m_window.pollEvent(ev)) {
        switch (ev.type) {

            case sf::Event::Closed:
                m_state.save(SAVE_FILE);
                m_window.close();
                break;

            case sf::Event::MouseButtonPressed: {
                sf::Vector2f pos(
                    static_cast<float>(ev.mouseButton.x),
                    static_cast<float>(ev.mouseButton.y));
                onMouseClick(pos, ev.mouseButton.button);
                break;
            }

            case sf::Event::MouseWheelScrolled: {
                sf::Vector2f pos(
                    static_cast<float>(ev.mouseWheelScroll.x),
                    static_cast<float>(ev.mouseWheelScroll.y));
                onMouseScroll(ev.mouseWheelScroll.delta, pos);
                break;
            }

            case sf::Event::KeyPressed:
                onKeyPress(ev.key.code);
                break;

            default: break;
        }

        // Forward all events to shop when on shop tab
        if (m_activeTab == Tab::SHOP) {
            bool bought = m_shop.handleEvent(ev, m_state);
            if (bought) {
                m_mining.syncTurrets(m_state);
                pushNotif("Upgrade purchased!",
                          sf::Color(120, 220, 255));
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Game::update(float dt) {

    double creditsEarned = 0.0;
    double oreEarned     = 0.0;

    // ── Mining always runs in background ──────────────────
    m_mining.update(dt, m_state, creditsEarned, oreEarned);

    // ── Plinko update (even when tab inactive for auto) ───
    if (m_state.autoPlinkoEnabled() && m_state.ore >= 1.0) {
        m_plinko.updateAuto(dt, m_state.ore,
                        1.f / m_state.fireRatePerSec());
    }
    if (m_activeTab == Tab::PLINKO) {
        double plinkoCredits = 0.0;
        m_plinko.update(dt, plinkoCredits,
                        m_state.creditMult(),
                        m_state.bulkProcess(),
                        m_mining.particles());
        creditsEarned += plinkoCredits;
    }

    // ── Shop hover update ─────────────────────────────────
    if (m_activeTab == Tab::SHOP) {
        sf::Vector2i mp = sf::Mouse::getPosition(m_window);
        m_shop.update(
            sf::Vector2f(static_cast<float>(mp.x),
                         static_cast<float>(mp.y)),
            m_state);
    }

    // ── Apply earnings ────────────────────────────────────
    if (creditsEarned > 0.0) {
        m_state.credits      += creditsEarned;
        m_state.totalCredits += creditsEarned;
    }
    if (oreEarned > 0.0) {
        m_state.ore      += oreEarned;
        m_state.totalOre += oreEarned;
    }

    // ── Notifications ─────────────────────────────────────
    updateNotifs(dt);

    // ── Auto-save ─────────────────────────────────────────
    m_saveTimer += dt;
    if (m_saveTimer >= SAVE_INTERVAL) {
        m_saveTimer = 0.f;
        m_state.save(SAVE_FILE);
    }

    // ── Rebuild plinko when upgrades change ───────────────
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

    drawTabBar();
    drawActiveTab();
    drawSidePanel();
    drawNotifs();

    m_window.display();
}

// ═════════════════════════════════════════════════════════════
//  onMouseClick
// ═════════════════════════════════════════════════════════════
void Game::onMouseClick(sf::Vector2f pos, sf::Mouse::Button btn) {
    if (btn != sf::Mouse::Left) return;

    // ── Tab bar ───────────────────────────────────────────
    for (int i = 0; i < TAB_COUNT; i++) {
        if (tabRect(i).contains(pos)) {
            m_activeTab       = static_cast<Tab>(i);
            m_prestigeConfirm = false;
            return;
        }
    }

    // ── Route to active screen ────────────────────────────
    switch (m_activeTab) {
        case Tab::PLINKO:
            handlePlinkoClick(pos);
            break;
        case Tab::PRESTIGE:
            handlePrestigeClick(pos);
            break;
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
    switch (key) {
        case sf::Keyboard::Num1: m_activeTab = Tab::MINING;   break;
        case sf::Keyboard::Num2: m_activeTab = Tab::PLINKO;   break;
        case sf::Keyboard::Num3: m_activeTab = Tab::SHOP;     break;
        case sf::Keyboard::Num4: m_activeTab = Tab::PRESTIGE; break;
        case sf::Keyboard::Space:
      if (m_activeTab == Tab::PLINKO) {
          if (m_state.ore >= 1.0 &&
              m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {
              m_state.ore -= 1.0;
              if (m_state.ore < 0.0) m_state.ore = 0.0;
              m_plinko.dropBall(1.0);
          }
      }
      break;
        case sf::Keyboard::S:
            m_state.save(SAVE_FILE);
            pushNotif("Game saved.", sf::Color(100, 220, 120));
            break;
        default: break;
    }
}

// ═════════════════════════════════════════════════════════════
//  Tab bar
// ═════════════════════════════════════════════════════════════
sf::FloatRect Game::tabRect(int idx) const {
    float tabW = (WINDOW_WIDTH - SIDE_W)
                 / static_cast<float>(TAB_COUNT);
    return { idx * tabW, 0.f, tabW, TAB_BAR_H };
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

    for (int i = 0; i < TAB_COUNT; i++) {
        auto  rect   = tabRect(i);
        bool  active = (m_activeTab == static_cast<Tab>(i));
        auto& accent = accents[i];

        sf::RectangleShape bg({ rect.width, rect.height });
        bg.setPosition(rect.left, rect.top);
        bg.setFillColor(active
            ? sf::Color(accent.r/5, accent.g/5, accent.b/5, 255)
            : sf::Color(10, 12, 22, 255));
        bg.setOutlineColor(active
            ? sf::Color(accent.r, accent.g, accent.b, 200)
            : sf::Color(30, 36, 60, 150));
        bg.setOutlineThickness(1.f);
        m_window.draw(bg);

        if (active) {
            sf::RectangleShape bar({ rect.width - 4.f, 3.f });
            bar.setPosition(rect.left + 2.f,
                            rect.top + rect.height - 3.f);
            bar.setFillColor(accent);
            m_window.draw(bar);
        }

        drawText(m_window, labels[i],
                 rect.left + 14.f,
                 rect.top  + TAB_BAR_H * 0.5f - 9.f,
                 14,
                 active ? accent : sf::Color(130, 140, 170),
                 active);
    }

    // Separator
    sf::RectangleShape sep(
        { static_cast<float>(WINDOW_WIDTH), 1.f });
    sep.setPosition(0.f, TAB_BAR_H);
    sep.setFillColor(sf::Color(40, 50, 90, 180));
    m_window.draw(sep);
}

// ═════════════════════════════════════════════════════════════
//  drawActiveTab
// ═════════════════════════════════════════════════════════════
void Game::drawActiveTab() const {
    switch (m_activeTab) {
        case Tab::MINING:   m_mining.draw(m_window, m_state); break;
        case Tab::PLINKO:   drawPlinkoTab();                   break;
        case Tab::SHOP:     m_shop.draw(m_window, m_state);   break;
        case Tab::PRESTIGE: drawPrestigeScreen();              break;
    }
}

// ═════════════════════════════════════════════════════════════
//  drawSidePanel
// ═════════════════════════════════════════════════════════════
void Game::drawSidePanel() const {
    float px = WINDOW_WIDTH - SIDE_W;
    float py = TAB_BAR_H;

    sf::RectangleShape bg({ SIDE_W, CONTENT_H });
    bg.setPosition(px, py);
    bg.setFillColor(sf::Color(10, 12, 24, 250));
    bg.setOutlineColor(sf::Color(40, 50, 90, 180));
    bg.setOutlineThickness(1.f);
    m_window.draw(bg);

    float tx  = px + 12.f;
    float ty  = py + 14.f;
    float gap = 21.f;

    auto line = [&](const std::string& label,
                    const std::string& val,
                    sf::Color vc = sf::Color(255, 220, 100)) {
        drawText(m_window, label, tx,       ty, 12,
                 sf::Color(120, 135, 165));
        drawText(m_window, val,   tx + 94.f, ty, 12, vc, true);
        ty += gap;
    };

    auto divider = [&]() {
        sf::RectangleShape d({ SIDE_W - 24.f, 1.f });
        d.setPosition(tx, ty);
        d.setFillColor(sf::Color(40, 50, 90));
        m_window.draw(d);
        ty += 10.f;
    };

    // Resources
    drawText(m_window, "RESOURCES", tx, ty, 12,
             sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("Credits",  "$ " + formatBig(m_state.credits),
         sf::Color(255, 215, 70));
    line("Ore",      formatBig(m_state.ore),
         sf::Color(160, 225, 100));
    line("Crystals", formatBig(m_state.crystals) + " \xf0\x9f\x92\x8e",
         sf::Color(170, 110, 255));
    divider();

    // Stats
    drawText(m_window, "STATS", tx, ty, 12,
             sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("Damage",    formatBig(m_state.gunDamage()));
    line("Fire/sec",
         [&]() {
             std::ostringstream s;
             s << std::fixed << std::setprecision(1)
               << m_state.fireRatePerSec();
             return s.str();
         }());
    line("Turrets",   std::to_string(m_state.turretCount()));
    line("Crit %",    pct(m_state.critChance()));
    line("Split",     std::to_string(m_state.splitShot()));
    line("Ore val",   formatBig(m_state.oreValueMult()) + "x");
    line("Cr mult",   formatBig(m_state.creditMult())   + "x");
    divider();

    // Lifetime
    drawText(m_window, "LIFETIME", tx, ty, 12,
             sf::Color(160, 180, 255), true);
    ty += gap + 2.f;
    line("All credits","$ " + formatBig(m_state.totalCredits));
    line("Prestiges",
         std::to_string(m_state.prestigeCount),
         sf::Color(120, 220, 160));
    divider();

    // Crystal preview
    double g = m_state.crystalsOnPrestige();
    std::ostringstream css;
    css << "+ " << std::fixed << std::setprecision(0)
        << g << " on prestige";
    drawText(m_window, css.str(), tx, ty, 11,
             sf::Color(150, 90, 240));
}

// ═════════════════════════════════════════════════════════════
//  Plinko tab
// ═════════════════════════════════════════════════════════════
void Game::rebuildPlinko() {
    float bx = CONTENT_X + 30.f;
    float by = CONTENT_Y + 20.f;
    float bw = CONTENT_W - 60.f;
    float bh = CONTENT_H - 90.f;
    m_plinko.build(m_state.plinkoRows(),
                   bx, by, bw, bh,
                   m_state.plinkoMultBonus(),
                   m_state.plinkoLuck());
}

void Game::drawPlinkoTab() const {
    sf::RectangleShape bg({ CONTENT_W, CONTENT_H });
    bg.setPosition(CONTENT_X, CONTENT_Y);
    bg.setFillColor(sf::Color(6, 8, 18));
    m_window.draw(bg);

    m_plinko.draw(m_window, const_cast<sf::Font&>(m_font));

    // ── Bottom bar ────────────────────────────────────────
    float barY   = CONTENT_Y + CONTENT_H - 58.f;
    float btnX   = CONTENT_X + CONTENT_W * 0.5f - 75.f;
    float btnY   = barY + 10.f;

    bool canDrop = m_state.ore > 0.0 &&
                   m_plinko.ballsAlive() < m_state.maxPlinkoBalls();

    sf::RectangleShape btn({ 150.f, 38.f });
    btn.setPosition(btnX, btnY);
    btn.setFillColor(canDrop
        ? sf::Color(55, 25, 95, 235)
        : sf::Color(28, 22, 40, 180));
    btn.setOutlineColor(canDrop
        ? sf::Color(160, 100, 255, 230)
        : sf::Color(55, 45, 75, 110));
    btn.setOutlineThickness(2.f);
    m_window.draw(btn);

    drawText(m_window,
             canDrop ? "DROP  [Space]"
                     : (m_state.ore <= 0.0 ? "No ore" : "Balls full"),
             btnX + 12.f, btnY + 10.f, 13,
             canDrop ? sf::Color(210, 170, 255)
                     : sf::Color(90, 80, 110),
             true);

    // Ore & ball counts
    std::ostringstream os, bs;
    os << "Ore: " << static_cast<long long>(m_state.ore);
    bs << "Balls: " << m_plinko.ballsAlive()
       << " / "     << m_state.maxPlinkoBalls();

    drawText(m_window, os.str(),
             CONTENT_X + 16.f, btnY + 6.f,
             13, sf::Color(170, 225, 110));
    drawText(m_window, bs.str(),
             CONTENT_X + 16.f, btnY + 24.f,
             12, sf::Color(150, 130, 195));
}

void Game::handlePlinkoClick(sf::Vector2f pos) {
    float btnX = CONTENT_X + CONTENT_W * 0.5f - 75.f;
    float btnY = CONTENT_Y + CONTENT_H - 48.f;
    sf::FloatRect btnRect(btnX, btnY, 150.f, 38.f);

    if (!btnRect.contains(pos)) return;

    if (m_state.ore >= 1.0 &&
        m_plinko.ballsAlive() < m_state.maxPlinkoBalls()) {

        m_state.ore -= 1.0;
        if (m_state.ore < 0.0) m_state.ore = 0.0;
        m_plinko.dropBall(1.0);

    } else if (m_state.ore < 1.0) {
        pushNotif("Geen ore!", sf::Color(255, 100, 80));
    }
}

// ═════════════════════════════════════════════════════════════
//  Prestige screen
// ═════════════════════════════════════════════════════════════
void Game::drawPrestigeScreen() const {

    sf::RectangleShape bg({ CONTENT_W, CONTENT_H });
    bg.setPosition(CONTENT_X, CONTENT_Y);
    bg.setFillColor(sf::Color(6, 8, 20));
    m_window.draw(bg);

    float cx = CONTENT_X + CONTENT_W * 0.5f;
    float ty = CONTENT_Y + 28.f;

    // Title
    drawText(m_window, "--- PRESTIGE ---",
             cx - 90.f, ty, 22,
             sf::Color(160, 100, 255), true);
    ty += 44.f;

    // Crystal gain
    double gain = m_state.crystalsOnPrestige();
    std::ostringstream gs;
    gs << "Gain:  +"
       << std::fixed << std::setprecision(0)
       << gain << "  Crystals";
    drawText(m_window, gs.str(),
             cx - 140.f, ty, 15, sf::Color(210, 170, 255));
    ty += 24.f;
    drawText(m_window,
             "( floor( sqrt( lifetime credits / 1000 ) ) )",
             cx - 155.f, ty, 11, sf::Color(100, 85, 130));
    ty += 30.f;

    // Divider
    auto divLine = [&]() {
        sf::RectangleShape d({ CONTENT_W - 80.f, 1.f });
        d.setPosition(CONTENT_X + 40.f, ty);
        d.setFillColor(sf::Color(60, 40, 90));
        m_window.draw(d);
        ty += 14.f;
    };
    divLine();

    // What resets
    drawText(m_window, "RESETS ON PRESTIGE:",
             cx - 170.f, ty, 13,
             sf::Color(255, 110, 90), true);
    ty += 22.f;
    drawText(m_window,
             "Credits  |  Ore  |  All regular upgrades",
             cx - 175.f, ty, 12, sf::Color(200, 155, 155));
    ty += 20.f;
    int kept = m_state.prestigeLevels[
        static_cast<int>(PrestigeUpgradeID::CRYSTAL_RETENTION)] * 2;
    std::ostringstream rs;
    rs << "Deep Retention: top " << kept
       << " upgrades kept at half level";
    drawText(m_window, rs.str(),
             cx - 175.f, ty, 12, sf::Color(140, 215, 150));
    ty += 32.f;
    divLine();

    // ── Prestige upgrade cards ────────────────────────────
    drawText(m_window,
             "PERMANENT UPGRADES  (cost: Crystals)",
             cx - 175.f, ty, 13,
             sf::Color(180, 140, 255), true);
    ty += 28.f;

    int pCount = static_cast<int>(
        PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT);
    float cardW = CONTENT_W - 80.f;
    float cardH = 52.f;
    float cardX = CONTENT_X + 40.f;

    for (int i = 0; i < pCount; i++) {
        auto        pid  = static_cast<PrestigeUpgradeID>(i);
        const auto& def  = GameState::prestigeCatalog[i];
        int         lv   = m_state.levelOf(pid);
        double      cost = m_state.costOf(pid);
        bool        can  = m_state.canBuy(pid);

        // Card bg
        sf::RectangleShape card({ cardW, cardH });
        card.setPosition(cardX, ty);
        card.setFillColor(can
            ? sf::Color(30, 18, 55, 230)
            : sf::Color(18, 16, 30, 200));
        card.setOutlineColor(can
            ? sf::Color(160, 100, 255, 180)
            : sf::Color(50, 40, 75, 110));
        card.setOutlineThickness(1.f);
        m_window.draw(card);

        // Name + level
        std::string nameStr = def.name + "  Lv " + std::to_string(lv);
        drawText(m_window, nameStr,
                 cardX + 10.f, ty + 7.f,
                 13, sf::Color(210, 200, 255), true);

        // Description
        drawText(m_window, def.description,
                 cardX + 10.f, ty + 26.f,
                 11, sf::Color(130, 120, 170));

        // Cost button
        std::ostringstream cs;
        cs << std::fixed << std::setprecision(1)
           << cost << " crystals";
        drawText(m_window, cs.str(),
                 cardX + cardW - 160.f, ty + 16.f,
                 12,
                 can ? sf::Color(200, 155, 255)
                     : sf::Color(90, 75, 110),
                 true);

        ty += cardH + 6.f;
        if (ty > CONTENT_Y + CONTENT_H - 100.f) break;
    }

    ty += 10.f;
    divLine();

    // ── Prestige / Confirm button ─────────────────────────
    float pbW = 200.f, pbH = 42.f;
    float pbX = cx - pbW * 0.5f;
    float pbY = CONTENT_Y + CONTENT_H - 70.f;

    sf::RectangleShape pb({ pbW, pbH });
    pb.setPosition(pbX, pbY);
    pb.setFillColor(m_prestigeConfirm
        ? sf::Color(120, 20, 200, 240)
        : sf::Color(50, 20, 90, 220));
    pb.setOutlineColor(m_prestigeConfirm
        ? sf::Color(220, 100, 255)
        : sf::Color(140, 80, 220, 180));
    pb.setOutlineThickness(2.f);
    m_window.draw(pb);

    drawText(m_window,
             m_prestigeConfirm
                 ? "CONFIRM PRESTIGE?"
                 : "PRESTIGE  (+crystals)",
             pbX + 14.f, pbY + 12.f, 14,
             sf::Color(220, 180, 255), true);
}

void Game::handlePrestigeClick(sf::Vector2f pos) {
    float cx  = CONTENT_X + CONTENT_W * 0.5f;
    float pbW = 200.f, pbH = 42.f;
    float pbX = cx - pbW * 0.5f;
    float pbY = CONTENT_Y + CONTENT_H - 70.f;

    // ── Prestige button ───────────────────────────────────
    if (sf::FloatRect(pbX, pbY, pbW, pbH).contains(pos)) {
        if (!m_prestigeConfirm) {
            m_prestigeConfirm = true;
        } else {
            // Do prestige
            double gained = m_state.crystalsOnPrestige();
            m_state.doPrestige();
            m_mining.clearAll();
            m_mining.syncTurrets(m_state);
            rebuildPlinko();
            m_prestigeConfirm = false;

            std::ostringstream ns;
            ns << "Prestiged! +"
               << std::fixed << std::setprecision(0)
               << gained << " crystals";
            pushNotif(ns.str(), sf::Color(200, 130, 255));
        }
        return;
    }

    // Cancel confirm if clicking elsewhere
    m_prestigeConfirm = false;

    // ── Prestige upgrade purchase ─────────────────────────
    float ty   = CONTENT_Y + 28.f + 44.f + 24.f + 30.f
               + 14.f + 22.f + 20.f + 32.f + 14.f + 28.f;
    float cardW = CONTENT_W - 80.f;
    float cardH = 52.f;
    float cardX = CONTENT_X + 40.f;

    int pCount = static_cast<int>(
        PrestigeUpgradeID::PRESTIGE_UPGRADE_COUNT);

    for (int i = 0; i < pCount; i++) {
        auto pid = static_cast<PrestigeUpgradeID>(i);
        sf::FloatRect cardRect(cardX, ty, cardW, cardH);

        if (cardRect.contains(pos) && m_state.canBuy(pid)) {
            m_state.buy(pid);
            pushNotif("Permanent upgrade bought!",
                      sf::Color(200, 150, 255));
            return;
        }
        ty += cardH + 6.f;
        if (ty > CONTENT_Y + CONTENT_H - 100.f) break;
    }
}
// ═════════════════════════════════════════════════════════════
//  Notifications
// ═════════════════════════════════════════════════════════════
void Game::pushNotif(const std::string& text, sf::Color color) {
    // Find a free slot
    for (auto& n : m_notifs) {
        if (!n.alive) {
            n.text        = text;
            n.color       = color;
            n.lifetime    = n.maxLifetime;
            n.yOffset     = 0.f;
            n.alive       = true;
            return;
        }
    }
    // If all slots full, overwrite the oldest (first slot)
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
        n.yOffset  += 28.f * dt;   // float upward
        if (n.lifetime <= 0.f)
            n.alive = false;
    }
}

void Game::drawNotifs() const {
    // Stack from bottom-right corner
    float baseX = WINDOW_WIDTH - SIDE_W - 320.f;
    float baseY = WINDOW_HEIGHT - 40.f;
    int   slot  = 0;

    for (const auto& n : m_notifs) {
        if (!n.alive) continue;

        float alpha = clamp(n.lifetime / n.maxLifetime, 0.f, 1.f);
        float y     = baseY - slot * 30.f - n.yOffset;

        // Background pill
        sf::RectangleShape bg({ 280.f, 24.f });
        bg.setPosition(baseX, y);
        bg.setFillColor(sf::Color(12, 14, 28,
            static_cast<uint8_t>(180 * alpha)));
        bg.setOutlineColor(sf::Color(n.color.r, n.color.g, n.color.b,
            static_cast<uint8_t>(160 * alpha)));
        bg.setOutlineThickness(1.f);
        m_window.draw(bg);

        // Text
        sf::Text txt;
        txt.setFont(m_font);
        txt.setCharacterSize(12);
        txt.setString(n.text);
        txt.setFillColor(sf::Color(n.color.r, n.color.g, n.color.b,
            static_cast<uint8_t>(255 * alpha)));
        txt.setPosition(baseX + 8.f, y + 4.f);
        m_window.draw(txt);

        slot++;
    }
}

// ═════════════════════════════════════════════════════════════
//  drawText  — convenience wrapper
// ═════════════════════════════════════════════════════════════
void Game::drawText(sf::RenderTarget&  target,
                     const std::string& str,
                     float x, float y,
                     unsigned size,
                     sf::Color color,
                     bool bold) const {
    sf::Text txt;
    txt.setFont(m_font);
    txt.setCharacterSize(size);
    txt.setString(str);
    txt.setFillColor(color);
    txt.setPosition(x, y);
    if (bold) txt.setStyle(sf::Text::Bold);
    target.draw(txt);
}
