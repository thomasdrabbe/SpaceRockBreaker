#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Bullet.h"
#include "Asteroid.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Single turret
// ─────────────────────────────────────────────────────────────
class Turret {
public:
    sf::Vector2f pos;
    float        angle      = 0.f;
    float        targetAngle= 0.f;
    float        fireTimer  = 0.f;
    bool         active     = false;
    sf::Color    baseColor  = sf::Color(60,  70,  90);
    sf::Color    barrelColor= sf::Color(100, 120, 150);

    void update(float            dt,
                float            fireInterval,
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

private:
    static constexpr float ROTATE_SPEED  = 300.f;
    static constexpr float BARREL_LEN    = 22.f;
    static constexpr float BASE_RADIUS   = 14.f;
    static constexpr float ACQUIRE_RANGE = 99999.f;
};

// ─────────────────────────────────────────────────────────────
//  TurretManager
// ─────────────────────────────────────────────────────────────
constexpr int MAX_TURRETS = 16;

class TurretManager {
public:
    TurretManager();

    void setCount(int count, float areaW, float areaH);
    void followPlayer(sf::Vector2f playerPos);

    void update(float            dt,
                float            fireInterval,
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;
    int  activeCount() const { return m_active; }

private:
    std::array<Turret, MAX_TURRETS> m_pool;
    int                              m_active = 0;

    void layoutPositions(int count, float areaW, float areaH);
};
