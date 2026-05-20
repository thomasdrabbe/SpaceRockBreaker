#include "Player.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

// ═════════════════════════════════════════════════════════════
//  init
// ═════════════════════════════════════════════════════════════
void Player::init(float startX, float startY) {
    pos   = { startX, startY };
    angle = -90.f;
    resetHp();
}

void Player::takeDamage(float dmg) {
    m_hp -= dmg;
    if (m_hp < 0.f)
        m_hp = 0.f;
}

void Player::resetHp() {
    m_hp = m_maxHp;
}

void Player::addFuel(float amount) {
    m_fuel = std::min(m_maxFuel, m_fuel + amount);
}

void Player::drainFuel(float amount) {
    m_fuel = std::max(0.f, m_fuel - amount);
}

void Player::updateHpRegen(float dt) {
    if (m_hp < m_maxHp)
        m_hp = std::min(m_maxHp, m_hp + HP_REGEN_PER_SEC * dt);
}

void Player::setShipSprite(const sf::Texture* tex, float targetH) {
    m_shipTex = tex;
    if (!tex) {
        m_shipScale  = 1.f;
        m_muzzleDist = BARREL_LEN;
        m_hitRadius  = SHIP_RADIUS;
        return;
    }
    const sf::Vector2u sz = tex->getSize();
    if (sz.y < 1u) {
        m_shipTex    = nullptr;
        m_muzzleDist = BARREL_LEN;
        m_hitRadius  = SHIP_RADIUS;
        return;
    }
    m_shipScale = targetH / static_cast<float>(sz.y);
    m_muzzleDist = std::max(
        BARREL_LEN,
        m_shipScale * static_cast<float>(sz.y) * 0.48f);
    const float rw =
        m_shipScale * static_cast<float>(std::max(sz.x, sz.y)) * 0.36f;
    m_hitRadius = std::max(SHIP_RADIUS * m_shipScale, rw);
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Player::update(float            dt,
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
                     AsteroidManager& asteroids,
                     BulletManager&   bullets,
                     ParticleSystem&  particles) {

    const bool canAct = !outOfFuel();

    drainFuel(fuelPassiveDrain * dt);
    drainFuel(fuelTurretDrain * dt);

    // ── WASD beweging ─────────────────────────────────────
    sf::Vector2f move{ 0.f, 0.f };

    if (canAct) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            move.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            move.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            move.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            move.x += 1.f;
    }

    if (length(move) > 0.f) {
        move = normalize(move);
        drainFuel(fuelMoveDrain * dt);
    }

    const float fr = fuelRatio();
    if (fr < 0.25f) {
        const float speedMult = 0.3f + fr * 2.8f;
        pos += move * speed * speedMult * dt;
    } else {
        pos += move * speed * dt;
    }

    // ── Check asteroid collision ──────────────────────────
    m_hitThisFrame = false;
    for (auto& asteroid : asteroids.all()) {
        if (!asteroid.alive) continue;
        float dist = distance(pos, asteroid.pos);
        if (dist < m_hitRadius + asteroid.radius * 0.6f) {
            m_hitThisFrame = true;
            break;
        }
    }

    // ── Grenzen (paneel in wereldcoördinaten; zij/onder: tip + barrel + glow)
    const float padSide = std::max({
        m_hitRadius * 1.55f + 8.f,
        m_muzzleDist + 8.f,
        m_hitRadius + 12.f });
    // Boven minder strak: ores/key kunnen tegen de bovenrand liggen; speler moet
    // daar nog bij kunnen (collect-ring volgt midden schip).
    const float padTop =
        std::max(m_hitRadius + 6.f, padSide * 0.52f);
    pos.x = clamp(pos.x, panelLeft + padSide, panelLeft + panelW - padSide);
    pos.y = clamp(pos.y, panelTop + padTop, panelTop + panelH - padSide);

    updateHpRegen(dt);

    // ── Auto-aim ──────────────────────────────────────────
    Asteroid* target = asteroids.nearest(pos);

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
    }

    // ── Vuren ─────────────────────────────────────────────
    fireTimer -= dt;
    if (canAct && fireTimer <= 0.f && target) {
        fireTimer = fireInterval;
        drainFuel(fuelShootDrain);

        bool  isCrit   = chance(critChance);
        float finalDmg = isCrit ? damage * critMult : damage;

        float        rad = toRad(angle);
        sf::Vector2f tip = {
            pos.x + std::cos(rad) * m_muzzleDist,
            pos.y + std::sin(rad) * m_muzzleDist
        };
        sf::Vector2f dir = normalize(target->pos - tip);

        bullets.fire(tip, dir, finalDmg, isCrit, splitShot, bulletLifetimeSec,
                     particles, false);
        gSfx.play(Sfx::Shot);

        if (isCrit)
            particles.emitCritText(tip, finalDmg);
    }
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void Player::draw(sf::RenderTarget& target) const {

    float rad = toRad(angle);

    if (m_shipTex) {
        sf::Sprite spr(*m_shipTex);
        const sf::Vector2u tsz = m_shipTex->getSize();
        const sf::Vector2f origin{
            static_cast<float>(tsz.x) * 0.5f,
            static_cast<float>(tsz.y) * 0.56f };
        spr.setOrigin(origin);
        spr.setPosition(pos);
        spr.setScale({ m_shipScale, m_shipScale });
        // Texture wijst omhoog; game-angle gebruikt dezelfde 0° = rechts-conventie.
        spr.setRotation(sf::degrees(angle + 90.f));
        target.draw(spr);
        return;
    }

    // ── Schip driehoek (fallback) ─────────────────────────
    sf::ConvexShape ship(3);
    ship.setPoint(0, {
        std::cos(rad)        * SHIP_RADIUS * 1.4f,
        std::sin(rad)        * SHIP_RADIUS * 1.4f });
    ship.setPoint(1, {
        std::cos(rad + 2.4f) * SHIP_RADIUS,
        std::sin(rad + 2.4f) * SHIP_RADIUS });
    ship.setPoint(2, {
        std::cos(rad - 2.4f) * SHIP_RADIUS,
        std::sin(rad - 2.4f) * SHIP_RADIUS });
    ship.setPosition(pos);
    ship.setFillColor(sf::Color(40, 80, 160, 230));
    ship.setOutlineColor(sf::Color(100, 180, 255, 220));
    ship.setOutlineThickness(2.f);
    target.draw(ship);

    // ── Engine glow ───────────────────────────────────────
    float backRad = rad + PI;

    bool moving =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    sf::CircleShape glow(7.f);
    glow.setOrigin({ 7.f, 7.f });
    glow.setPosition({
        pos.x + std::cos(backRad) * SHIP_RADIUS * 0.8f,
        pos.y + std::sin(backRad) * SHIP_RADIUS * 0.8f });
    glow.setFillColor(moving
        ? sf::Color(255, 140, 40, 200)
        : sf::Color(255,  80, 20,  80));
    target.draw(glow);

    // ── Barrel ────────────────────────────────────────────
    sf::RectangleShape barrel(sf::Vector2f{ BARREL_LEN, 4.f });
    barrel.setOrigin({ 0.f, 2.f });
    barrel.setPosition(pos);
    barrel.setRotation(sf::degrees(angle));
    barrel.setFillColor(sf::Color(140, 200, 255, 200));
    target.draw(barrel);
}
