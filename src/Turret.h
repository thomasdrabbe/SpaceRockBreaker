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
    float        angle       = 0.f;   // current barrel angle (degrees)
    float        targetAngle = 0.f;   // angle toward current target
    float        fireTimer   = 0.f;   // counts down to next shot
    bool         active      = false;

    // Visual
    sf::Color    baseColor  = sf::Color(60,  70,  90);
    sf::Color    barrelColor= sf::Color(100, 120, 150);

    /// Aim at nearest asteroid and shoot when timer hits 0
    void update(float            dt,
                float            fireInterval,   // seconds between shots
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

private:
    static constexpr float ROTATE_SPEED = 300.f;  // deg/s toward target
    static constexpr float BARREL_LEN   = 22.f;
    static constexpr float BASE_RADIUS  = 14.f;
    static constexpr float ACQUIRE_RANGE = 99999.f; // no range limit
};

// ─────────────────────────────────────────────────────────────
//  TurretManager  — up to MAX_TURRETS placed on screen
// ─────────────────────────────────────────────────────────────
constexpr int MAX_TURRETS = 16;

class TurretManager {
public:
    TurretManager();

    /// Rebuild turret positions whenever count changes
    void setCount(int count, float areaW, float areaH);

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

    /// Beweeg alle turrets mee met de speler
    void followPlayer(sf::Vector2f playerPos);

    int activeCount() const { return m_active; }

private:
    std::array<Turret, MAX_TURRETS> m_pool;
    int                              m_active = 0;

    /// Evenly space turrets along the bottom + sides of mining area
    void layoutPositions(int count, float areaW, float areaH);
};
