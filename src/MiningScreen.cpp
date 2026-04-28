#include "MiningScreen.h"
#include "Utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
MiningScreen::MiningScreen()
    : m_particles(MAX_PARTICLES) {
}


// ═════════════════════════════════════════════════════════════
//  init
// ═════════════════════════════════════════════════════════════
void MiningScreen::init(sf::Font& font,
                         float panelX, float panelY,
                         float panelW, float panelH) {
    m_font = &font;
    m_x    = panelX;
    m_y    = panelY;
    m_w    = panelW;
    m_h    = panelH;

    m_collectorPos = { m_x + m_w * 0.5f,
                       m_y + m_h * 0.5f };

    m_player.init(m_x + m_w * 0.5f,
                  m_y + m_h * 0.5f);

    buildStarfield();
}

// ─────────────────────────────────────────────────────────────
//  buildStarfield
// ─────────────────────────────────────────────────────────────
void MiningScreen::buildStarfield() {
    for (auto& s : m_stars) {
        s.pos        = { randFloat(m_x, m_x + m_w),
                         randFloat(m_y, m_y + m_h) };
        s.speed      = randFloat(4.f, 18.f);
        s.radius     = randFloat(0.5f, 2.2f);
        s.brightness = static_cast<uint8_t>(randInt(100, 255));
    }
}

// ═════════════════════════════════════════════════════════════
//  syncTurrets
// ═════════════════════════════════════════════════════════════
void MiningScreen::syncTurrets(const GameState& state) {
    int cnt = state.turretCount();
    if (cnt != m_lastTurretCnt) {
        m_turrets.setCount(cnt, m_w, m_h);
        m_lastTurretCnt = cnt;
    }
}

// ─────────────────────────────────────────────────────────────
//  targetAsteroidCount
// ─────────────────────────────────────────────────────────────
int MiningScreen::targetAsteroidCount(int turrets) {
    return std::min(6 + turrets * 3, MAX_ASTEROIDS);
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void MiningScreen::update(float      dt,
                            GameState& state,
                            double&    creditsEarned,
                            double&    oreEarned) {

    // ── Sync turrets ──────────────────────────────────────
    syncTurrets(state);

    // ── Player update ─────────────────────────────────────
    m_player.update(dt,
                    1.f / state.fireRatePerSec(),
                    state.gunDamage(),
                    state.critChance(),
                    state.critMult(),
                    state.splitShot(),
                    m_w, m_h,
                    m_asteroids,
                    m_bullets,
                    m_particles);

    // Collector volgt speler
    m_collectorPos = m_player.pos;

    // Turrets volgen speler
    m_turrets.followPlayer(m_player.pos);

    // ── HP multiplier ─────────────────────────────────────
    float hpMult = std::max(0.1f,
        1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);

    // ── Asteroid field ────────────────────────────────────
    int target = targetAsteroidCount(state.turretCount());
    int spawnTarget = target + state.levelSpawnBonus();
    m_asteroids.maintainField(spawnTarget, m_w, m_h,hpMult * state.levelHpMult(), state.maxOreTier());
    m_asteroids.update(dt, m_w, m_h);

    // ── Turrets ───────────────────────────────────────────
    m_turrets.update(dt,
                     1.f / state.fireRatePerSec(),
                     state.gunDamage(),
                     state.critChance(),
                     state.critMult(),
                     state.splitShot(),
                     m_asteroids,
                     m_bullets,
                     m_particles);

    // ── Bullets ───────────────────────────────────────────
    m_bullets.update(dt, m_w, m_h);

    // ── Collisions ────────────────────────────────────────
    resolveCollisions(state);

    // ── Ore collectie (alleen ore, geen credits) ──────────
    double oreThisFrame = 0.0;
    m_ores.update(dt,
                  m_collectorPos,
                  state.autoCollectRadius(),
                  oreThisFrame,
                  state.bulkProcess(),
                  m_particles);
    oreEarned += oreThisFrame;

    // ── Particles ─────────────────────────────────────────
    m_particles.update(dt);

    // ── Sterren scrollen ──────────────────────────────────
    for (auto& s : m_stars) {
        s.pos.y += s.speed * dt;
        if (s.pos.y > m_y + m_h + 4.f)
            s.pos = { randFloat(m_x, m_x + m_w), m_y - 4.f };
    }
}

// ═════════════════════════════════════════════════════════════
//  resolveCollisions
// ═════════════════════════════════════════════════════════════
void MiningScreen::resolveCollisions(GameState& state) {
    for (auto& bullet : m_bullets.all()) {
        if (!bullet.alive) continue;

        for (auto& asteroid : m_asteroids.all()) {
            if (!asteroid.alive) continue;

            float dist = distance(bullet.pos, asteroid.pos);
            if (dist > bullet.radius + asteroid.radius)
                continue;

            bullet.alive = false;

            bool destroyed = asteroid.hit(bullet.damage, m_particles);
            if (destroyed) {
      int count = asteroid.oreDrop.count * asteroid.rarityDropMult();
      m_ores.drop(
          asteroid.pos,
          asteroid.oreDrop.color,
          asteroid.oreDrop.value,    // ← voeg value toe
          count,
          state.oreLuckBonus(),
          m_particles);
  }
            break;
        }
    }
}

// ═════════════════════════════════════════════════════════════
//  collectAllOre
// ═════════════════════════════════════════════════════════════
void MiningScreen::collectAllOre(double&          oreOut,
                                   const GameState& state) {
    m_ores.collectAll(oreOut, state.bulkProcess());
}

// ═════════════════════════════════════════════════════════════
//  clearAll
// ═════════════════════════════════════════════════════════════
void MiningScreen::clearAll() {
    m_ores.clearAll();
    for (auto& b : m_bullets.all()) b.alive = false;
    for (auto& a : m_asteroids.all()) a.alive = false;
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void MiningScreen::draw(sf::RenderTarget& target,
                         const GameState&  state,
                         float             warpCharge) const {

    sf::View oldView = target.getView();

    float vpX = m_x / WINDOW_WIDTH;
    float vpY = m_y / WINDOW_HEIGHT;
    float vpW = m_w / WINDOW_WIDTH;
    float vpH = m_h / WINDOW_HEIGHT;

    sf::View mineView(sf::FloatRect(
        { m_x, m_y }, { m_w, m_h }));
    mineView.setViewport(sf::FloatRect(
        { vpX, vpY }, { vpW, vpH }));
    target.setView(mineView);

    // ── Background ────────────────────────────────────────
    sf::RectangleShape bg(sf::Vector2f{ m_w, m_h });
    bg.setPosition({ m_x, m_y });
    bg.setFillColor(sf::Color(4, 6, 16));
    target.draw(bg);

    drawStarfield(target);
    drawCollectRing(target, state);
    drawCollector(target);

    // ── Entities ──────────────────────────────────────────
    m_asteroids.draw(target);
    m_ores.draw(target);
    m_bullets.draw(target);
    m_turrets.draw(target);
    m_player.draw(target);
    m_particles.draw(target, *m_font);

    target.setView(oldView);

    // ── HUD (buiten clipped view) ─────────────────────────
    drawHUD(target, state, warpCharge);

}

// ─────────────────────────────────────────────────────────────
//  drawStarfield
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawStarfield(sf::RenderTarget& target) const {
    sf::CircleShape star;
    for (const auto& s : m_stars) {
        star.setRadius(s.radius);
        star.setOrigin({ s.radius, s.radius });
        star.setPosition(s.pos);
        star.setFillColor(sf::Color(
            s.brightness, s.brightness,
            s.brightness, 200));
        target.draw(star);
    }
}

// ─────────────────────────────────────────────────────────────
//  drawCollector
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawCollector(sf::RenderTarget& target) const {
    // Outer ring
    sf::CircleShape outer(18.f);
    outer.setOrigin({ 18.f, 18.f });
    outer.setPosition(m_collectorPos);
    outer.setFillColor(sf::Color(20, 30, 60, 200));
    outer.setOutlineColor(sf::Color(80, 140, 255, 200));
    outer.setOutlineThickness(3.f);
    target.draw(outer);

    // Inner core
    sf::CircleShape inner(9.f);
    inner.setOrigin({ 9.f, 9.f });
    inner.setPosition(m_collectorPos);
    inner.setFillColor(sf::Color(100, 160, 255, 230));
    target.draw(inner);

    // Centre dot
    sf::CircleShape dot(3.5f);
    dot.setOrigin({ 3.5f, 3.5f });
    dot.setPosition(m_collectorPos);
    dot.setFillColor(sf::Color(220, 235, 255));
    target.draw(dot);
}

// ─────────────────────────────────────────────────────────────
//  drawCollectRing
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawCollectRing(sf::RenderTarget& target,
                                    const GameState&  state) const {
    float r = state.autoCollectRadius();

    sf::CircleShape ring(r);
    ring.setOrigin({ r, r });
    ring.setPosition(m_collectorPos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(80, 140, 255, 35));
    ring.setOutlineThickness(1.5f);
    target.draw(ring);
}

// ─────────────────────────────────────────────────────────────
//  drawHUD
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawHUD(sf::RenderTarget& target,
                             const GameState&  state,
                             float             warpCharge) const {

// ── Warp UI ───────────────────────────────────────────────
// ── Warp UI ───────────────────────────────────────────────
if (state.warpDriveUnlocked()) {
    float barW = m_w * 0.25f;                        // 25% van schermbreed
    float barH = m_h * 0.014f;                       // ~1.4% van schermhoog
    float barX = m_x + m_w * 0.5f - barW * 0.5f;
    float barY = m_y + m_h - barH - m_h * 0.02f;
    unsigned fontSize = static_cast<unsigned>(
        std::max(10.f, m_h * 0.016f));

    // Achtergrond
    sf::RectangleShape bg(sf::Vector2f(barW, barH));
    bg.setPosition(sf::Vector2f(barX, barY));
    bg.setFillColor(sf::Color(20, 20, 40, 200));
    bg.setOutlineColor(sf::Color(80, 140, 255, 160));
    bg.setOutlineThickness(1.f);
    target.draw(bg);

    // Charge fill
    if (warpCharge > 0.f) {
        sf::RectangleShape fill(sf::Vector2f(
            barW * warpCharge, barH));
        fill.setPosition(sf::Vector2f(barX, barY));
        uint8_t g = static_cast<uint8_t>(120 + 135 * warpCharge);
        fill.setFillColor(sf::Color(60, g, 255, 220));
        target.draw(fill);
    }

    // Label
    sf::Text lbl(*m_font);
    lbl.setCharacterSize(fontSize);
    lbl.setFillColor(sf::Color(160, 200, 255));
    if (!state.canWarp())
        lbl.setString("Warp: " +
            std::to_string(static_cast<int>(state.oreThisLevel))
            + " / " +
            std::to_string(state.oreWarpRequirement()) +
            " ore");
    else if (warpCharge <= 0.f)
        lbl.setString("Warp ready  —  hold Space");
    else
        lbl.setString("Warping...");

    float lw = lbl.getLocalBounds().size.x;
    lbl.setPosition(sf::Vector2f(
        barX + (barW - lw) * 0.5f,
        barY - barH - fontSize * 1.2f));
    target.draw(lbl);
}

    // Zone label — linksboven
    sf::Text zoneLabel(*m_font);
    zoneLabel.setString(state.levelLabel());
    zoneLabel.setCharacterSize(15);
    zoneLabel.setStyle(sf::Text::Bold);
    zoneLabel.setFillColor(sf::Color(180, 210, 255));
    zoneLabel.setPosition({ m_x + 8.f, m_y + 8.f });
    target.draw(zoneLabel);    // HUD achtergrond
    sf::RectangleShape hudBg(sf::Vector2f{ 220.f, 96.f });
    hudBg.setPosition({ m_x + 5.f, m_y + 4.f });
    hudBg.setFillColor(sf::Color(4, 6, 16, 170));
    hudBg.setOutlineColor(sf::Color(40, 60, 100, 120));
    hudBg.setOutlineThickness(1.f);
    target.draw(hudBg);

    float hx    = m_x + 10.f;
    float hy    = m_y + 8.f;
    float lineH = 17.f;

    auto drawLine = [&](const std::string& s,
                         sf::Color col = sf::Color(200, 210, 240)) {
        sf::Text txt(*m_font);
        txt.setCharacterSize(13);
        txt.setString(s);
        txt.setFillColor(col);
        txt.setPosition({ hx, hy });
        target.draw(txt);
        hy += lineH;
    };

    drawLine("Asteroids : " +
             std::to_string(m_asteroids.aliveCount()),
             sf::Color(180, 200, 255));
    drawLine("Bullets   : " +
             std::to_string(m_bullets.aliveCount()),
             sf::Color(160, 230, 255));
    drawLine("Ore drops : " +
             std::to_string(m_ores.aliveCount()),
             sf::Color(200, 220, 140));
    drawLine("Particles : " +
             std::to_string(m_particles.alive()),
             sf::Color(160, 160, 180));
    drawLine("Turrets   : " +
             std::to_string(m_turrets.activeCount()),
             sf::Color(255, 180, 100));


}
