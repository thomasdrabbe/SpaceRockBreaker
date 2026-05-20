#include "Bullet.h"
#include "Asteroid.h"
#include "Utils.h"
#include <cmath>

namespace {
    constexpr float BULLET_SPEED  = 620.f;
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
                          float           lifetimeSec,
                          ParticleSystem& particles,
                          bool            fromTurret) {
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
        b->lifetime = std::max(0.05f, lifetimeSec);
        b->isCrit   = isCrit;
        b->alive    = true;
        b->fromTurret = fromTurret;

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
void BulletManager::update(float dt, float ox, float oy,
                            float areaW, float areaH,
                            float homingDegPerFrame,
                            AsteroidManager* asteroids,
                            TargetMode targetMode) {
    m_alive = 0;
    for (auto& b : m_pool) {
        if (!b.alive) continue;

        if (homingDegPerFrame > 0.f && asteroids) {
            Asteroid* tgt = asteroids->pickTarget(targetMode, b.pos);
            if (tgt) {
                const float curAng = std::atan2(b.vel.y, b.vel.x);
                const float wantAng =
                    std::atan2(tgt->pos.y - b.pos.y, tgt->pos.x - b.pos.x);
                float diff = wantAng - curAng;
                while (diff > PI) diff -= 2.f * PI;
                while (diff < -PI) diff += 2.f * PI;
                const float maxTurn = toRad(homingDegPerFrame);
                const float turn = clamp(diff, -maxTurn, maxTurn);
                const float spd  = length(b.vel);
                const float ang  = curAng + turn;
                b.vel = { std::cos(ang) * spd, std::sin(ang) * spd };
            }
        }

        b.pos      += b.vel * dt;
        b.lifetime -= dt;

        bool out = (b.pos.x < ox - 20.f || b.pos.x > ox + areaW + 20.f ||
                    b.pos.y < oy - 20.f || b.pos.y > oy + areaH + 20.f);

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
