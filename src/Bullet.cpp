#include "Bullet.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────────────────────
namespace {
    constexpr float BULLET_SPEED   = 620.f;   // px/s
    constexpr float BULLET_LIFE    = 2.8f;    // seconds before auto-expire
    constexpr float SPLIT_SPREAD   = 0.18f;   // radians half-angle for split
}

// ─────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────
BulletManager::BulletManager() {
    // pool is zero-initialised; alive=false for all
}

// ─────────────────────────────────────────────────────────────
//  claim  — find next free slot
// ─────────────────────────────────────────────────────────────
Bullet* BulletManager::claim() {
    for (auto& b : m_pool)
        if (!b.alive) return &b;
    return nullptr;   // pool full
}

// ─────────────────────────────────────────────────────────────
//  fire
// ─────────────────────────────────────────────────────────────
void BulletManager::fire(sf::Vector2f    origin,
                          sf::Vector2f    targetDir,
                          float           damage,
                          bool            isCrit,
                          int             splitCount,
                          ParticleSystem& particles) {

    // Base angle from direction vector
    float baseAngle = std::atan2(targetDir.y, targetDir.x);

    // Spread the split shots evenly around the base angle
    // e.g. splitCount=3 → angles: base-spread, base, base+spread
    for (int i = 0; i < splitCount; i++) {
        Bullet* b = claim();
        if (!b) return;

        float offset = 0.f;
        if (splitCount > 1) {
            // Map i to [-1, 1] range then multiply by spread
            float t = (splitCount == 1)
                        ? 0.f
                        : (static_cast<float>(i) / (splitCount - 1)) * 2.f - 1.f;
            offset = t * SPLIT_SPREAD;
        }

        float angle = baseAngle + offset;
        sf::Vector2f dir = { std::cos(angle), std::sin(angle) };

        b->pos      = origin;
        b->vel      = dir * BULLET_SPEED;
        b->damage   = damage;
        b->lifetime = BULLET_LIFE;
        b->isCrit   = isCrit;
        b->alive    = true;

        // Visual: crits are bright red, normal are cyan/white
        if (isCrit) {
            b->color  = sf::Color(255, 60, 60);
            b->radius = 6.f;
        } else {
            b->color  = sf::Color(160, 230, 255);
            b->radius = 4.f;
        }

        // Small muzzle spark at origin
        particles.emitSpark(origin, dir, 3);
    }
}

// ─────────────────────────────────────────────────────────────
//  update
// ─────────────────────────────────────────────────────────────
void BulletManager::update(float dt, float areaW, float areaH) {
    m_alive = 0;
    for (auto& b : m_pool) {
        if (!b.alive) continue;

        b.pos      += b.vel * dt;
        b.lifetime -= dt;

        // Kill if expired or out of bounds
        bool outOfBounds = (b.pos.x < -20.f || b.pos.x > areaW + 20.f ||
                            b.pos.y < -20.f || b.pos.y > areaH + 20.f);
        if (b.lifetime <= 0.f || outOfBounds) {
            b.alive = false;
            continue;
        }

        m_alive++;
    }
}

// ─────────────────────────────────────────────────────────────
//  draw
// ─────────────────────────────────────────────────────────────
void BulletManager::draw(sf::RenderTarget& target) const {
    sf::CircleShape shape;

    for (const auto& b : m_pool) {
        if (!b.alive) continue;

        // Outer glow ring
        shape.setRadius(b.radius + 2.f);
        shape.setOrigin(b.radius + 2.f, b.radius + 2.f);
        shape.setPosition(b.pos);
        shape.setFillColor(sf::Color(
            b.color.r,
            b.color.g,
            b.color.b,
            60));
        target.draw(shape);

        // Core bullet
        shape.setRadius(b.radius);
        shape.setOrigin(b.radius, b.radius);
        shape.setPosition(b.pos);
        shape.setFillColor(b.color);
        target.draw(shape);

        // Bright white centre dot for readability
        shape.setRadius(b.radius * 0.4f);
        shape.setOrigin(b.radius * 0.4f, b.radius * 0.4f);
        shape.setPosition(b.pos);
        shape.setFillColor(sf::Color(255, 255, 255, 200));
        target.draw(shape);
    }
}