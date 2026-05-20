#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Constants.h"
#include "Asteroid.h"
#include "Bullet.h"
#include "Particle.h"

class SatelliteDrone {
public:
    float orbitAngle = 0.f;
    float fireTimer  = 0.f;
    bool  active     = false;

    void update(float orbitRadius,
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
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target) const;

private:
    sf::Vector2f m_pos{};
};

constexpr int MAX_SATELLITES = 8;

class SatelliteManager {
public:
    void setCount(int count);
    void update(sf::Vector2f      playerPos,
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
                  ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

private:
    std::array<SatelliteDrone, MAX_SATELLITES> m_pool{};
    int m_active = 0;
};
