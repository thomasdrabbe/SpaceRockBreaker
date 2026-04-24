#pragma once
#include <SFML/Graphics.hpp>
#include "Constants.h"
#include "Bullet.h"
#include "Asteroid.h"
#include "Particle.h"

class Player {
public:
    sf::Vector2f pos;
    float        angle     = -90.f;  // graden, start omhoog
    float        speed     = 220.f;  // px/s
    float        fireTimer = 0.f;
    bool         alive     = true;

    void init(float startX, float startY);

    void update(float            dt,
                float            fireInterval,
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                float            areaW,
                float            areaH,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

    sf::Vector2f centre() const { return pos; }

private:
    static constexpr float SHIP_RADIUS   = 14.f;
    static constexpr float ROTATE_SPEED  = 280.f;  // deg/s
    static constexpr float BARREL_LEN    = 18.f;
};