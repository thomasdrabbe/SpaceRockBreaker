#include "Satellite.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>

void SatelliteDrone::update(float orbitRadius,
                              float orbitSpeed,
                              sf::Vector2f centre,
                              float dt,
                              float fireInterval,
                              float damage,
                              float critChance,
                              float critMult,
                              int splitShot,
                              float bulletLifetimeSec,
                              TargetMode targetMode,
                              AsteroidManager& asteroids,
                              BulletManager& bullets,
                              ParticleSystem& particles) {
    if (!active)
        return;

    orbitAngle += orbitSpeed * dt;
    m_pos = {
        centre.x + std::cos(orbitAngle) * orbitRadius,
        centre.y + std::sin(orbitAngle) * orbitRadius };

    Asteroid* target = asteroids.pickTarget(targetMode, m_pos);
    if (!target)
        return;

    fireTimer -= dt;
    if (fireTimer > 0.f)
        return;
    fireTimer = fireInterval;

    const bool  isCrit   = chance(critChance);
    const float finalDmg = isCrit ? damage * critMult : damage;
    sf::Vector2f dir     = normalize(target->pos - m_pos);
    bullets.fire(m_pos, dir, finalDmg, isCrit, splitShot, bulletLifetimeSec,
                 particles, true);
    gSfx.play(Sfx::Shot);
}

void SatelliteDrone::draw(sf::RenderTarget& target) const {
    if (!active)
        return;
    sf::CircleShape c(5.f);
    c.setOrigin({ 5.f, 5.f });
    c.setPosition(m_pos);
    c.setFillColor(sf::Color(120, 200, 255, 220));
    c.setOutlineColor(sf::Color(200, 240, 255));
    c.setOutlineThickness(1.f);
    target.draw(c);
}

void SatelliteManager::setCount(int count) {
    m_active = std::clamp(count, 0, MAX_SATELLITES);
    for (int i = 0; i < MAX_SATELLITES; ++i)
        m_pool[static_cast<std::size_t>(i)].active = (i < m_active);
}

void SatelliteManager::update(sf::Vector2f      playerPos,
                                float            dt,
                                float            orbitRadius,
                                float            orbitSpeed,
                                float            fireInterval,
                                float            damage,
                                float            critChance,
                                float            critMult,
                                int              splitShot,
                                float            bulletLifetimeSec,
                                TargetMode       targetMode,
                                AsteroidManager& asteroids,
                                BulletManager&   bullets,
                                ParticleSystem&  particles) {
    if (m_active <= 0)
        return;
    const float step = (2.f * PI) / static_cast<float>(m_active);
    for (int i = 0; i < m_active; ++i) {
        auto& s = m_pool[static_cast<std::size_t>(i)];
        if (s.orbitAngle == 0.f && i > 0)
            s.orbitAngle = step * static_cast<float>(i);
        s.update(orbitRadius, orbitSpeed, playerPos, dt, fireInterval, damage,
                 critChance, critMult, splitShot, bulletLifetimeSec, targetMode,
                 asteroids, bullets, particles);
    }
}

void SatelliteManager::draw(sf::RenderTarget& target) const {
    for (int i = 0; i < m_active; ++i)
        m_pool[static_cast<std::size_t>(i)].draw(target);
}
