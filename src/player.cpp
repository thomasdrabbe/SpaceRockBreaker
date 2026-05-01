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
                     float            areaW,
                     float            areaH,
                     AsteroidManager& asteroids,
                     BulletManager&   bullets,
                     ParticleSystem&  particles) {

    // ── WASD beweging ─────────────────────────────────────
    sf::Vector2f move{ 0.f, 0.f };

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        move.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        move.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        move.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        move.x += 1.f;

    if (length(move) > 0.f)
        move = normalize(move);

    pos += move * speed * dt;

    // ── Check asteroid collision ──────────────────────────
    m_hitThisFrame = false;
    for (auto& asteroid : asteroids.all()) {
        if (!asteroid.alive) continue;
        float dist = distance(pos, asteroid.pos);
        if (dist < SHIP_RADIUS + asteroid.radius * 0.6f) {
            m_hitThisFrame = true;
            break;
        }
    }

    // ── Grenzen ───────────────────────────────────────────
    pos.x = clamp(pos.x, SHIP_RADIUS, areaW - SHIP_RADIUS);
    pos.y = clamp(pos.y, SHIP_RADIUS, areaH - SHIP_RADIUS);

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
    if (fireTimer <= 0.f && target) {
        fireTimer = fireInterval;

        bool  isCrit   = chance(critChance);
        float finalDmg = isCrit ? damage * critMult : damage;

        float        rad = toRad(angle);
        sf::Vector2f tip = {
            pos.x + std::cos(rad) * BARREL_LEN,
            pos.y + std::sin(rad) * BARREL_LEN
        };
        sf::Vector2f dir = normalize(target->pos - tip);

        bullets.fire(tip, dir, finalDmg, isCrit,
                     splitShot, particles);
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

    // ── Schip driehoek ────────────────────────────────────
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
