#pragma once
#include <SFML/Graphics.hpp>
#include <array>          
#include "Constants.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Ore colour / type per asteroid tier
// ─────────────────────────────────────────────────────────────
struct OreDrop {
    sf::Color color;
    double    value;      // base credits when processed
    int       count;      // how many ore pieces drop
};

// ─────────────────────────────────────────────────────────────
//  Asteroid  — a single rock entity
// ─────────────────────────────────────────────────────────────
class Asteroid {
public:
    // ── State ─────────────────────────────────────────────
    sf::Vector2f  pos;
    sf::Vector2f  vel;
    float         rotation     = 0.f;   // degrees
    float         rotationRate = 0.f;   // deg/s
    float         radius       = 0.f;
    float         hp           = 0.f;
    float         maxHp        = 0.f;
    AsteroidTier  tier         = AsteroidTier::SMALL;
    bool          alive        = false;

    // ── Visual ────────────────────────────────────────────
    sf::Color     color;
    sf::ConvexShape shape;     // irregular polygon built on spawn

    // ── Ore payload ───────────────────────────────────────
    OreDrop       oreDrop;

    // ── Public API ────────────────────────────────────────

    /// Initialise (or re-use) this asteroid
    void spawn(AsteroidTier tier,
               sf::Vector2f pos,
               sf::Vector2f vel,
               float hpMult = 1.f);

    /// Apply damage; returns true when destroyed
    bool hit(float damage, ParticleSystem& particles);

    /// Move, rotate — call every frame
    void update(float dt);

    /// Draw the polygon + HP bar
    void draw(sf::RenderTarget& target) const;

    /// Bounce off the mining panel borders
    void bounceWalls(float left, float top,
                     float right, float bottom);

private:
    void buildShape();    // randomise polygon vertices
};

// ─────────────────────────────────────────────────────────────
//  AsteroidManager  — pool of MAX_ASTEROIDS rocks
// ─────────────────────────────────────────────────────────────
class AsteroidManager {
public:
    AsteroidManager();

    /// Spawn a new asteroid at a random border position
    void spawnRandom(float areaW, float areaH, float hpMult = 1.f);

    /// Keep the field populated (call every frame)
    void maintainField(int targetCount,
                       float areaW, float areaH,
                       float hpMult);

    void update(float dt,
                float areaW, float areaH);

    void draw(sf::RenderTarget& target) const;

    /// Returns pointer to nearest alive asteroid within maxDist,
    /// or nullptr if none found
    Asteroid* nearest(sf::Vector2f from, float maxDist = 99999.f);

    /// All slots — for bullet collision checks etc.
    std::array<Asteroid, MAX_ASTEROIDS>& all() { return m_pool; }

    int aliveCount() const { return m_alive; }

private:
    std::array<Asteroid, MAX_ASTEROIDS> m_pool;
    int                                  m_alive = 0;

    Asteroid* claim();
};