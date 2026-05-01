#include "KeyPickup.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

KeyPickup* KeyPickupManager::claim() {
    for (auto& k : m_pool)
        if (!k.alive)
            return &k;
    return nullptr;
}

void KeyPickupManager::drop(sf::Vector2f pos, int count,
                            ParticleSystem& particles) {
    const sf::Color gold(255, 210, 90);
    for (int i = 0; i < count; ++i) {
        KeyPickup* k = claim();
        if (!k)
            return;

        float angle = randFloat(0.f, 2.f * PI);
        float speed = randFloat(32.f, 95.f);

        k->pos = pos + sf::Vector2f(randFloat(-8.f, 8.f),
                                    randFloat(-8.f, 8.f));
        k->vel        = { std::cos(angle) * speed,
                          std::sin(angle) * speed };
        k->radius     = randFloat(5.f, 8.f);
        k->lifetime   = KEY_LIFETIME + randFloat(-2.f, 2.f);
        k->bobTimer   = randFloat(0.f, 2.f * PI);
        k->alive      = true;
        k->collecting = false;

        particles.emitOreCollect(
            k->pos,
            pos + sf::Vector2f(0, -40),
            gold, 1);
    }
}

void KeyPickupManager::update(float            dt,
                              sf::Vector2f     collectorPos,
                              float            collectRadius,
                              int&             keysOut,
                              ParticleSystem&  particles) {
    m_alive = 0;

    for (auto& k : m_pool) {
        if (!k.alive)
            continue;

        k.lifetime -= dt;
        if (k.lifetime <= 0.f) {
            k.alive = false;
            continue;
        }

        float dist = distance(k.pos, collectorPos);
        if (dist <= collectRadius)
            k.collecting = true;

        if (k.collecting) {
            sf::Vector2f dir = normalize(collectorPos - k.pos);
            k.pos += dir * COLLECT_PULL_SPEED * dt;

            if (distance(k.pos, collectorPos) < 8.f) {
                keysOut += 1;
                gSfx.play(Sfx::OreCollect);
                particles.emitSpark(
                    k.pos,
                    normalize(collectorPos - k.pos), 3);
                k.alive = false;
                continue;
            }
        } else {
            k.vel *= (1.f - 2.5f * dt);
            k.bobTimer += BOB_SPEED * dt;
            float bobY = std::sin(k.bobTimer) * BOB_AMP;
            k.pos += k.vel * dt;
            k.pos.y += bobY * dt;
        }

        m_alive++;
    }
}

void KeyPickupManager::draw(sf::RenderTarget& target) const {
    sf::RectangleShape stem;
    sf::CircleShape    bow;
    sf::ConvexShape    bit;

    for (const auto& k : m_pool) {
        if (!k.alive)
            continue;

        float   alpha = (k.lifetime < 3.f) ? k.lifetime / 3.f : 1.f;
        uint8_t a     = static_cast<uint8_t>(255 * alpha);

        sf::Vector2f p = k.pos;
        float        r = k.radius;

        // Glow
        sf::CircleShape glow(r + 4.f);
        glow.setOrigin({ r + 4.f, r + 4.f });
        glow.setPosition(p);
        glow.setFillColor(sf::Color(255, 210, 80,
            static_cast<uint8_t>(45 * alpha)));
        target.draw(glow);

        // Stem (slightly below center)
        stem.setSize({ r * 0.22f, r * 1.1f });
        stem.setOrigin({ stem.getSize().x * 0.5f, stem.getSize().y * 0.85f });
        stem.setPosition(p + sf::Vector2f(0.f, r * 0.15f));
        stem.setRotation(sf::degrees(-12.f));
        stem.setFillColor(sf::Color(230, 200, 120, a));
        stem.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(90 * alpha)));
        stem.setOutlineThickness(1.f);
        target.draw(stem);

        // Bow (circle at top of stem)
        float bowR = r * 0.55f;
        bow.setRadius(bowR);
        bow.setOrigin({ bowR, bowR });
        bow.setPosition(p + sf::Vector2f(-r * 0.12f, -r * 0.35f));
        bow.setFillColor(sf::Color(255, 220, 130, a));
        bow.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(130 * alpha)));
        bow.setOutlineThickness(1.2f);
        target.draw(bow);

        // Bit (small wedge)
        bit.setPointCount(3);
        bit.setPoint(0, { 0.f, 0.f });
        bit.setPoint(1, { r * 0.45f, r * 0.08f });
        bit.setPoint(2, { r * 0.35f, -r * 0.22f });
        bit.setPosition(p + sf::Vector2f(r * 0.15f, r * 0.55f));
        bit.setFillColor(sf::Color(245, 245, 255, a));
        bit.setOutlineColor(sf::Color(200, 210, 255,
            static_cast<uint8_t>(100 * alpha)));
        bit.setOutlineThickness(1.f);
        target.draw(bit);
    }
}

void KeyPickupManager::collectAll(int& keysOut) {
    for (auto& k : m_pool) {
        if (!k.alive)
            continue;
        keysOut += 1;
        k.alive = false;
    }
    m_alive = 0;
}

void KeyPickupManager::clearAll() {
    for (auto& k : m_pool)
        k.alive = false;
    m_alive = 0;
}
