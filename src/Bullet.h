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
    float        damage   = 0.f;
    float        lifetime = 0.f;
    float        radius   = 4.f;
    bool         isCrit   = false;
    bool         alive    = false;
    sf::Color    color;
};

// ─────────────────────────────────────────────────────────────
//  BulletManager  — fixed-size pool
// ─────────────────────────────────────────────────────────────
class BulletManager {
public:
    BulletManager();

    void fire(sf::Vector2f    origin,
              sf::Vector2f    targetDir,
              float           damage,
              bool            isCrit,
              int             splitCount,
              ParticleSystem& particles);

    void update(float dt, float ox, float oy, float areaW, float areaH);
    void draw(sf::RenderTarget& target) const;

    std::array<Bullet, MAX_BULLETS>& all() { return m_pool; }
    int aliveCount() const { return m_alive; }

private:
    std::array<Bullet, MAX_BULLETS> m_pool;
    int                              m_alive = 0;

    Bullet* claim();
};
