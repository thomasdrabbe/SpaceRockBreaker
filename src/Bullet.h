#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Single bullet
// ─────────────────────────────────────────────────────────────
struct Bullet {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float        damage      = 0.f;
    float        lifetime    = 0.f;   // seconds until auto-expire
    float        radius      = 4.f;
    bool         isCrit      = false;
    bool         alive       = false;
    sf::Color    color;
};

// ─────────────────────────────────────────────────────────────
//  BulletManager  — fixed-size pool
// ─────────────────────────────────────────────────────────────
class BulletManager {
public:
    BulletManager();

    /// Fire one or more bullets (split-shot handled here)
    /// origin     : world position of the turret barrel tip
    /// targetDir  : normalised direction toward target
    /// damage     : base damage (crit already factored in if isCrit)
    /// isCrit     : tints bullet red + triggers crit text
    /// splitCount : total number of bullets to fire (1 = normal)
    void fire(sf::Vector2f    origin,
              sf::Vector2f    targetDir,
              float           damage,
              bool            isCrit,
              int             splitCount,
              ParticleSystem& particles);

    /// Per-frame integration + wall cull
    void update(float dt, float areaW, float areaH);

    /// Draw all alive bullets
    void draw(sf::RenderTarget& target) const;

    /// Access pool for collision checks
    std::array<Bullet, MAX_BULLETS>& all() { return m_pool; }

    int aliveCount() const { return m_alive; }

private:
    std::array<Bullet, MAX_BULLETS> m_pool;
    int                              m_alive = 0;

    Bullet* claim();
};