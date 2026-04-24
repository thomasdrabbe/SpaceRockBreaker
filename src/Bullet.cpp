#include "Bullet.h"
#include "Utils.h"
#include <cmath>

namespace {
    constexpr float BULLET_SPEED  = 620.f;
    constexpr float BULLET_LIFE   = 2.8f;
    constexpr float SPLIT_SPREAD  = 0.18f;
}

// ─────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────
BulletManager::BulletManager() {}

// ─────────────────────────────────────────────────────────────
//  claim
// ─────────────────────────────────────────────────────────────
Bullet* BulletManager::claim() {
    for (auto& b : m_pool)
        if (!b.alive) return &b;
    return nullptr;
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
    float baseAngle = std::atan2(targetDir.y, targetDir.x);

    for (int i = 0; i < splitCount; i++) {
        Bullet* b = claim();
        if (!b) return;

        float offset = 0.f;
        if (splitCount > 1) {
            float t = (static_cast<float>(i) /
                       (splitCount - 1)) * 2.f - 1.f;
            offset  = t * SPLIT_SPREAD;
        }

        float        angle = baseAngle + offset;
        sf::Vector2f dir   = { std::cos(angle), std::sin(angle) };

        b->pos      = origin;
        b->vel      = dir * BULLET_SPEED;
        b->damage   = damage;
        b->lifetime = BULLET_LIFE;
        b->isCrit   = isCrit;
        b->alive    = true;

        if (isCrit) {
            b->color  = sf::Color(255, 60, 60);
            b->radius = 6.f;
        } else {
            b->color  = sf::Color(160, 230, 255);
            b->radius = 4.f;
        }

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

        bool out = (b.pos.x < -20.f || b.pos.x > areaW + 20.f ||
                    b.pos.y < -20.f || b.pos.y > areaH + 20.f);

        if (b.lifetime <= 0.f || out) {
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

        // Glow ring
        float glowR = b.radius + 2.f;
        shape.setRadius(glowR);
        shape.setOrigin({ glowR, glowR });
        shape.setPosition(b.pos);
        shape.setFillColor(sf::Color(
            b.color.r, b.color.g, b.color.b, 60));
        target.draw(shape);

        // Core
        shape.setRadius(b.radius);
        shape.setOrigin({ b.radius, b.radius });
        shape.setPosition(b.pos);
        shape.setFillColor(b.color);
        target.draw(shape);

        // White centre dot
        float dotR = b.radius * 0.4f;
        shape.setRadius(dotR);
        shape.setOrigin({ dotR, dotR });
        shape.setPosition(b.pos);
        shape.setFillColor(sf::Color(255, 255, 255, 200));
        target.draw(shape);
    }
}
