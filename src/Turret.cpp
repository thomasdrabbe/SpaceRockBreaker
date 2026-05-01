#include "Turret.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  Turret::update
// ═════════════════════════════════════════════════════════════
void Turret::update(float            dt,
                     float            fireInterval,
                     float            damage,
                     float            critChance,
                     float            critMult,
                     int              splitShot,
                     AsteroidManager& asteroids,
                     BulletManager&   bullets,
                     ParticleSystem&  particles) {
    if (!active) return;

    // ── Acquire target ────────────────────────────────────
    Asteroid* target = asteroids.nearest(pos, ACQUIRE_RANGE);

    if (target) {
        float desired = toDeg(angleTo(pos, target->pos));

        float diff = desired - angle;
        while (diff >  180.f) diff -= 360.f;
        while (diff < -180.f) diff += 360.f;

        float maxStep = ROTATE_SPEED * dt;
        if (std::abs(diff) <= maxStep)
            angle = desired;
        else
            angle += (diff > 0.f ? 1.f : -1.f) * maxStep;

        targetAngle = desired;
    }

    // ── Fire timer ────────────────────────────────────────
    fireTimer -= dt;
    if (fireTimer > 0.f || !target) return;
    fireTimer = fireInterval;

    // ── Crit roll ─────────────────────────────────────────
    bool  isCrit   = chance(critChance);
    float finalDmg = isCrit ? damage * critMult : damage;

    // ── Barrel tip ────────────────────────────────────────
    float        rad = toRad(angle);
    sf::Vector2f tip = {
        pos.x + std::cos(rad) * BARREL_LEN,
        pos.y + std::sin(rad) * BARREL_LEN
    };

    sf::Vector2f dir = normalize(target->pos - tip);
    bullets.fire(tip, dir, finalDmg, isCrit, splitShot, particles);
    gSfx.play(Sfx::Shot);

    if (isCrit)
        particles.emitCritText(tip, finalDmg);
}

// ═════════════════════════════════════════════════════════════
//  Turret::draw
// ═════════════════════════════════════════════════════════════
void Turret::draw(sf::RenderTarget& target) const {
    if (!active) return;

    // ── Base hexagon ──────────────────────────────────────
    sf::CircleShape base(BASE_RADIUS, 6);
    base.setOrigin({ BASE_RADIUS, BASE_RADIUS });
    base.setPosition(pos);
    base.setFillColor(baseColor);
    base.setOutlineColor(sf::Color(
        static_cast<uint8_t>(std::min(255, (int)baseColor.r + 50)),
        static_cast<uint8_t>(std::min(255, (int)baseColor.g + 50)),
        static_cast<uint8_t>(std::min(255, (int)baseColor.b + 50))));
    base.setOutlineThickness(2.f);
    target.draw(base);

    // ── Inner ring ────────────────────────────────────────
    float        innerR = BASE_RADIUS * 0.55f;
    sf::CircleShape ring(innerR);
    ring.setOrigin({ innerR, innerR });
    ring.setPosition(pos);
    ring.setFillColor(sf::Color(
        static_cast<uint8_t>(std::min(255, (int)baseColor.r + 30)),
        static_cast<uint8_t>(std::min(255, (int)baseColor.g + 30)),
        static_cast<uint8_t>(std::min(255, (int)baseColor.b + 30))));
    target.draw(ring);

    // ── Barrel ────────────────────────────────────────────
    sf::RectangleShape barrel(sf::Vector2f{ BARREL_LEN, 6.f });
    barrel.setOrigin({ 0.f, 3.f });
    barrel.setPosition(pos);
    barrel.setRotation(sf::degrees(angle));
    barrel.setFillColor(barrelColor);
    barrel.setOutlineColor(sf::Color(200, 220, 255, 120));
    barrel.setOutlineThickness(1.f);
    target.draw(barrel);
}

// ═════════════════════════════════════════════════════════════
//  TurretManager
// ═════════════════════════════════════════════════════════════
TurretManager::TurretManager() {
    for (auto& t : m_pool) t.active = false;
}

// ─────────────────────────────────────────────────────────────
//  layoutPositions
// ─────────────────────────────────────────────────────────────
void TurretManager::layoutPositions(int count,
                                     float areaW, float areaH) {
    constexpr float INSET   = 30.f;
    constexpr float PADDING = 60.f;

    for (int i = 0; i < count; i++) {
        float t = (count == 1)
                    ? 0.5f
                    : static_cast<float>(i) / (count - 1);

        float x = PADDING + t * (areaW - PADDING * 2.f);
        float y = areaH - INSET;

        if (count > 6 && i >= count / 2) {
            int   j  = i - count / 2;
            int   n2 = count - count / 2;
            float t2 = (n2 == 1)
                        ? 0.5f
                        : static_cast<float>(j) / (n2 - 1);
            x = PADDING * 1.5f + t2 * (areaW - PADDING * 3.f);
            y = areaH - INSET - 55.f;
        }

        m_pool[i].pos    = { x, y };
        m_pool[i].angle  = -90.f;
        m_pool[i].active = true;

        if (i % 2 == 0) {
            m_pool[i].baseColor   = sf::Color(55,  70,  95);
            m_pool[i].barrelColor = sf::Color(90, 115, 150);
        } else {
            m_pool[i].baseColor   = sf::Color(70,  55,  90);
            m_pool[i].barrelColor = sf::Color(115,  90, 145);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  setCount
// ─────────────────────────────────────────────────────────────
void TurretManager::setCount(int count,
                               float areaW, float areaH) {
    count    = clamp(count, 0, MAX_TURRETS);
    m_active = count;

    for (auto& t : m_pool) t.active = false;
    if (count > 0)
        layoutPositions(count, areaW, areaH);
}

// ─────────────────────────────────────────────────────────────
//  followPlayer
// ─────────────────────────────────────────────────────────────
void TurretManager::followPlayer(sf::Vector2f playerPos) {
    int count = m_active;
    for (int i = 0; i < count; i++) {
        float angle  = (count == 1)
                        ? 0.f
                        : (i / static_cast<float>(count))
                          * 2.f * PI;
        float orbit  = 28.f;
        m_pool[i].pos = {
            playerPos.x + std::cos(angle) * orbit,
            playerPos.y + std::sin(angle) * orbit
        };
    }
}

// ─────────────────────────────────────────────────────────────
//  update
// ─────────────────────────────────────────────────────────────
void TurretManager::update(float            dt,
                             float            fireInterval,
                             float            damage,
                             float            critChance,
                             float            critMult,
                             int              splitShot,
                             AsteroidManager& asteroids,
                             BulletManager&   bullets,
                             ParticleSystem&  particles) {
    for (auto& t : m_pool) {
        if (!t.active) continue;
        t.update(dt, fireInterval, damage, critChance,
                 critMult, splitShot,
                 asteroids, bullets, particles);
    }
}

// ─────────────────────────────────────────────────────────────
//  draw
// ─────────────────────────────────────────────────────────────
void TurretManager::draw(sf::RenderTarget& target) const {
    for (const auto& t : m_pool)
        if (t.active) t.draw(target);
}
