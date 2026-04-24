#pragma once
#include <SFML/Graphics.hpp>
#include "Constants.h"
#include "GameState.h"
#include "Asteroid.h"
#include "Bullet.h"
#include "Turret.h"
#include "Ore.h"
#include "Particle.h"
#include "Player.h"

// ─────────────────────────────────────────────────────────────
//  A single star in the background parallax layer
// ─────────────────────────────────────────────────────────────
struct Star {
    sf::Vector2f pos;
    float        speed;      // scroll speed (parallax)
    float        radius;
    uint8_t      brightness;
};

// ─────────────────────────────────────────────────────────────
//  MiningScreen  — left-panel gameplay view
//  Owns: asteroids, bullets, turrets, ore, particles, starfield
// ─────────────────────────────────────────────────────────────
class MiningScreen {
public:
    MiningScreen();

    /// Call once after window is created
    void init(sf::Font& font,
              float panelX, float panelY,
              float panelW, float panelH);

    /// Full per-frame update
    /// creditsEarned / oreEarned are accumulated and returned
    void update(float      dt,
                GameState& state,
                double&    creditsEarned,
                double&    oreEarned);

    /// Render everything inside the mining panel
    void draw(sf::RenderTarget& target,
              const GameState&  state) const;

    /// Called when turret count changes so layout rebuilds
    void syncTurrets(const GameState& state);

    /// Vacuum all ore (used on prestige)
    void collectAllOre(double& oreOut, const GameState& state);
    /// Remove all entities (used on prestige reset)
    void clearAll();

    // ── Sub-system access (Plinko needs ore drops) ────────
    OreManager&      ores()      { return m_ores;      }
    ParticleSystem&  particles() { return m_particles; }

private:
    // ── Panel geometry ────────────────────────────────────
    sf::Font* m_font   = nullptr;
    float     m_x      = 0.f;
    float     m_y      = 0.f;
    float     m_w      = 0.f;
    float     m_h      = 0.f;

    // ── Entities ──────────────────────────────────────────
    AsteroidManager m_asteroids;
    BulletManager   m_bullets;
    TurretManager   m_turrets;
    OreManager      m_ores;
    ParticleSystem  m_particles;
	Player 			m_player;

    // ── Starfield ─────────────────────────────────────────
    static constexpr int STAR_COUNT = 180;
    std::array<Star, STAR_COUNT> m_stars;
    void buildStarfield();

    // ── Collector base (centre-bottom) ────────────────────
    sf::Vector2f m_collectorPos;

    // ── Spawn throttle ────────────────────────────────────
    float m_spawnTimer    = 0.f;
    int   m_lastTurretCnt = 0;

    // ── Collision: bullets ↔ asteroids ────────────────────
    void resolveCollisions(GameState& state,
                           double&    creditsEarned);

    // ── Draw helpers ──────────────────────────────────────
    void drawStarfield  (sf::RenderTarget& target) const;
    void drawCollector  (sf::RenderTarget& target) const;
    void drawHUD        (sf::RenderTarget& target,
                         const GameState&  state)  const;
    void drawCollectRing(sf::RenderTarget& target,
                         const GameState&  state)  const;

    // Target asteroid count scales with turret count
    static int targetAsteroidCount(int turrets);
};
