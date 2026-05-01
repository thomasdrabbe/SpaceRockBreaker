#pragma once
#include <SFML/Graphics.hpp>
#include "Constants.h"
#include "Bullet.h"
#include "Asteroid.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Player
// ─────────────────────────────────────────────────────────────
class Player {
public:
    sf::Vector2f pos;
    float        angle     = -90.f;
    float        speed     = 220.f;
    float        fireTimer = 0.f;
    bool         alive     = true;
    bool wasHit() const { return m_hitThisFrame; }

    void init(float startX, float startY);

    /// nullptr = vector fallback (development zonder texture).
    void setShipSprite(const sf::Texture* tex);

    void update(float            dt,
                float            fireInterval,
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                float            panelLeft,
                float            panelTop,
                float            panelW,
                float            panelH,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

    sf::Vector2f centre() const { return pos; }

    void clearHit()     { m_hitThisFrame = false; }


private:

    bool m_hitThisFrame = false;

    const sf::Texture* m_shipTex   = nullptr;
    float              m_shipScale = 1.f;
    float              m_muzzleDist  = 18.f;
    float              m_hitRadius   = 14.f;

    static constexpr float SHIP_RADIUS  = 14.f;  // fallback body
    static constexpr float ROTATE_SPEED = 280.f;
    static constexpr float BARREL_LEN   = 18.f;
};
