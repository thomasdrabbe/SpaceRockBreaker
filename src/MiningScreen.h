#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "GameState.h"
#include "Asteroid.h"
#include "Bullet.h"
#include "Turret.h"
#include "Ore.h"
#include "Particle.h"
#include "Player.h"

// ─────────────────────────────────────────────────────────────
//  Star (parallax background)
// ─────────────────────────────────────────────────────────────
struct Star {
    sf::Vector2f pos;
    float        speed;
    float        radius;
    uint8_t      brightness;
};

// ─────────────────────────────────────────────────────────────
//  MiningScreen
// ─────────────────────────────────────────────────────────────
class MiningScreen {
public:
    bool playerHit() const;
    MiningScreen();

    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH);

    /// creditsEarned : credits verdiend via Plinko (niet hier)
    /// oreEarned     : ore opgepakt deze frame
    void update(float      dt,
                GameState& state,
                double&    creditsEarned,
                double&    oreEarned);

    void draw   (sf::RenderTarget& target, const GameState&  state,
    float       warpCharge = 0.f) const;


    void syncTurrets(const GameState& state);

    void collectAllOre(double&          oreOut,
                       const GameState& state);
    void clearAll();

    // ── Sub-system toegang ────────────────────────────────
    OreManager&     ores()      { return m_ores;      }
    ParticleSystem& particles() { return m_particles; }

private:
    sf::Font*    m_font = nullptr;
    float        m_x    = 0.f;
    float        m_y    = 0.f;
    float        m_w    = 0.f;
    float        m_h    = 0.f;

    // ── Entities ──────────────────────────────────────────
    AsteroidManager m_asteroids;
    BulletManager   m_bullets;
    TurretManager   m_turrets;
    OreManager      m_ores;
    ParticleSystem  m_particles;
    Player          m_player;

    // ── Starfield ─────────────────────────────────────────
    static constexpr int STAR_COUNT = 220;
    std::array<Star, STAR_COUNT> m_stars;
    void buildStarfield();

    // ── Collector volgt speler ────────────────────────────
    sf::Vector2f m_collectorPos;

    // ── State ─────────────────────────────────────────────
    int m_lastTurretCnt = 0;

    // ── Collision ─────────────────────────────────────────
    void resolveCollisions(GameState& state);

    // ── Draw helpers ──────────────────────────────────────
    void drawStarfield  (sf::RenderTarget& target) const;
    void drawCollector  (sf::RenderTarget& target) const;
    void drawCollectRing(sf::RenderTarget& target,
                         const GameState&  state)  const;
    void drawHUD(sf::RenderTarget& target, const GameState&  state,
    float             warpCharge) const;


    static int targetAsteroidCount(int turrets);
};
