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
    // Duidelijk ander dan goud-ore: zilver/blauw “spark”.
    const sf::Color keySpark(175, 225, 255);
    for (int i = 0; i < count; ++i) {
        KeyPickup* k = claim();
        if (!k)
            return;

        float angle = randFloat(0.f, 2.f * PI);
        float speed = randFloat(32.f, 95.f);

        // Spawn hoger dan ores zodat sleutels niet in dezelfde goud-wolk verdwijnen.
        k->pos = pos + sf::Vector2f(randFloat(-16.f, 16.f),
                                    randFloat(-58.f, -32.f));
        k->vel        = { std::cos(angle) * speed,
                          std::sin(angle) * speed - randFloat(45.f, 130.f) };
        // Veel groter dan ore-pickups (~4–9) zodat de sleutel opvalt.
        k->radius     = randFloat(18.f, 26.f);
        k->lifetime   = KEY_LIFETIME + randFloat(-2.f, 2.f);
        k->bobTimer   = randFloat(0.f, 2.f * PI);
        k->alive      = true;
        k->collecting = false;

        particles.emitOreCollect(
            k->pos,
            pos + sf::Vector2f(0, -40),
            keySpark, 1);
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

            if (distance(k.pos, collectorPos)
                < std::max(10.f, k.radius * 0.45f)) {
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

void KeyPickupManager::draw(sf::RenderTarget& target,
                            const sf::Texture* keyIconTex) const {
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

        if (keyIconTex && keyIconTex->getSize().x > 0u) {
            const float glowR = r + std::max(6.f, r * 0.28f);
            sf::CircleShape glow(glowR);
            glow.setOrigin({ glowR, glowR });
            glow.setPosition(p);
            glow.setFillColor(sf::Color(160, 210, 255,
                static_cast<uint8_t>(50 * alpha)));
            target.draw(glow);

            sf::Sprite spr(*keyIconTex);
            const sf::Vector2u tsz = keyIconTex->getSize();
            const float          side = r * 2.15f;
            const float          sc =
                side / std::max(1.f, static_cast<float>(std::max(tsz.x, tsz.y)));
            spr.setOrigin({ tsz.x * 0.5f, tsz.y * 0.5f });
            spr.setPosition(p);
            spr.setScale({ sc, sc });
            spr.setColor(sf::Color(255, 255, 255, a));
            target.draw(spr);
            continue;
        }

        const float lineW = std::max(1.2f, r * 0.07f);

        const float glowR = r + std::max(6.f, r * 0.28f);
        sf::CircleShape glow(glowR);
        glow.setOrigin({ glowR, glowR });
        glow.setPosition(p);
        glow.setFillColor(sf::Color(160, 210, 255,
            static_cast<uint8_t>(50 * alpha)));
        target.draw(glow);

        stem.setSize({ r * 0.22f, r * 1.1f });
        stem.setOrigin({ stem.getSize().x * 0.5f, stem.getSize().y * 0.85f });
        stem.setPosition(p + sf::Vector2f(0.f, r * 0.15f));
        stem.setRotation(sf::degrees(-12.f));
        stem.setFillColor(sf::Color(230, 200, 120, a));
        stem.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(90 * alpha)));
        stem.setOutlineThickness(lineW);
        target.draw(stem);

        float bowR = r * 0.55f;
        bow.setRadius(bowR);
        bow.setOrigin({ bowR, bowR });
        bow.setPosition(p + sf::Vector2f(-r * 0.12f, -r * 0.35f));
        bow.setFillColor(sf::Color(255, 220, 130, a));
        bow.setOutlineColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(130 * alpha)));
        bow.setOutlineThickness(lineW * 1.1f);
        target.draw(bow);

        bit.setPointCount(3);
        bit.setPoint(0, { 0.f, 0.f });
        bit.setPoint(1, { r * 0.45f, r * 0.08f });
        bit.setPoint(2, { r * 0.35f, -r * 0.22f });
        bit.setPosition(p + sf::Vector2f(r * 0.15f, r * 0.55f));
        bit.setFillColor(sf::Color(245, 245, 255, a));
        bit.setOutlineColor(sf::Color(200, 210, 255,
            static_cast<uint8_t>(100 * alpha)));
        bit.setOutlineThickness(lineW);
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
