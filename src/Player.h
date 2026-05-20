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

    float hp() const { return m_hp; }
    float maxHp() const { return m_maxHp; }
    void  takeDamage(float dmg);
    /// Shield stats van GameState; rest gaat naar HP.
    void  applyDamage(float dmg,
                      float shieldMaxHp,
                      float shieldRechargePerSec,
                      float shieldRechargeDelaySec,
                      int   shieldExtraHits);
    void  resetHp();
    void  updateHpRegen(float dt);
    void  updateShield(float dt,
                       float shieldMaxHp,
                       float shieldRechargePerSec,
                       float shieldRechargeDelaySec,
                       int   shieldExtraHits);
    float shieldHp() const { return m_shieldHp; }
    float shieldMaxHp() const { return m_shieldMax; }

    float fuel()    const { return m_fuel; }
    float maxFuel() const { return m_maxFuel; }
    float fuelRatio() const {
        return m_maxFuel > 0.f ? m_fuel / m_maxFuel : 0.f;
    }
    void addFuel(float amount);
    void drainFuel(float amount);
    bool outOfFuel() const { return m_fuel <= 0.f; }
    void setMaxFuel(float newMax) {
        m_maxFuel = newMax;
        m_fuel    = std::min(m_fuel, m_maxFuel);
    }

    /// nullptr = vector fallback (development zonder texture).
    void setShipSprite(const sf::Texture* tex, float targetH = 112.f);

    void update(float            dt,
                float            fireInterval,
                float            damage,
                float            critChance,
                float            critMult,
                int              splitShot,
                float            bulletLifetimeSec,
                float            panelLeft,
                float            panelTop,
                float            panelW,
                float            panelH,
                float            fuelPassiveDrain,
                float            fuelMoveDrain,
                float            fuelShootDrain,
                float            fuelTurretDrain,
                TargetMode       targetMode,
                AsteroidManager& asteroids,
                BulletManager&   bullets,
                ParticleSystem&  particles);

    void draw(sf::RenderTarget& target) const;

    sf::Vector2f centre() const { return pos; }

    void clearHit()     { m_hitThisFrame = false; }


private:

    bool m_hitThisFrame = false;

    float m_hp    = 100.f;
    float m_maxHp = 100.f;
    float m_shieldHp     = 0.f;
    float m_shieldMax    = 0.f;
    int   m_shieldHits   = 0;
    float m_shieldDelayT = 0.f;
    float m_fuel    = 100.f;
    float m_maxFuel = 100.f;
    static constexpr float HP_REGEN_PER_SEC = 5.f;

    const sf::Texture* m_shipTex   = nullptr;
    float              m_shipScale = 1.f;
    float              m_muzzleDist  = 18.f;
    float              m_hitRadius   = 14.f;

    static constexpr float SHIP_RADIUS  = 14.f;  // fallback body
    static constexpr float ROTATE_SPEED = 280.f;
    static constexpr float BARREL_LEN   = 18.f;
};
