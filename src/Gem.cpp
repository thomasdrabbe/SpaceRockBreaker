#include "Gem.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

GemPickup* GemPickupManager::claim() {
    for (auto& g : m_pool)
        if (!g.alive)
            return &g;
    return nullptr;
}

void GemPickupManager::setTextures(
    const std::array<const sf::Texture*, GEM_TYPE_COUNT_INT>* tex) {
    m_tex = tex;
}

void GemPickupManager::drop(sf::Vector2f pos, GemType type,
                            ParticleSystem& particles) {
    GemPickup* g = claim();
    if (!g)
        return;
    const int ti = static_cast<int>(type);
    g->pos       = pos + sf::Vector2f(randFloat(-12.f, 12.f),
                                      randFloat(-20.f, -8.f));
    g->type      = type;
    g->glowTimer = randFloat(0.f, 6.28f);
    g->alive     = true;
    if (ti >= 0 && ti < GEM_TYPE_COUNT_INT) {
        const auto& gd = GEM_DEFS[ti];
        particles.emitExplosion(g->pos, 14.f, gd.glowColor, 12);
    }
}

void GemPickupManager::update(float            dt,
                              sf::Vector2f     collectorPos,
                              float            collectRadius,
                              int&             collectedOut,
                              GemType&         collectedTypeOut,
                              ParticleSystem&  particles) {
    m_alive = 0;
    collectedOut = 0;

    for (auto& gem : m_pool) {
        if (!gem.alive)
            continue;

        gem.glowTimer += dt;
        const float dx = gem.pos.x - collectorPos.x;
        const float dy = gem.pos.y - collectorPos.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < collectRadius + 12.f) {
            gem.alive = false;
            collectedOut++;
            collectedTypeOut = gem.type;
            const int ti = static_cast<int>(gem.type);
            if (ti >= 0 && ti < GEM_TYPE_COUNT_INT) {
                particles.emitExplosion(
                    gem.pos, 16.f, GEM_DEFS[ti].glowColor, 18);
            }
            continue;
        }

        gem.pos.y += std::sin(gem.glowTimer * 2.2f) * 8.f * dt;
        m_alive++;
    }
}

void GemPickupManager::collectAll(int& countOut, GemType& typeOut) {
    countOut = 0;
    typeOut  = GemType::RUBY;
    for (auto& gem : m_pool) {
        if (!gem.alive)
            continue;
        gem.alive = false;
        countOut++;
        typeOut = gem.type;
    }
}

void GemPickupManager::clearAll() {
    for (auto& gem : m_pool)
        gem.alive = false;
    m_alive = 0;
}

void GemPickupManager::draw(sf::RenderTarget& target) const {
    const sf::RenderStates rsAdd(sf::BlendAdd);

    for (const auto& gem : m_pool) {
        if (!gem.alive)
            continue;
        const int ti = static_cast<int>(gem.type);
        if (ti < 0 || ti >= GEM_TYPE_COUNT_INT)
            continue;
        const auto& gd = GEM_DEFS[ti];

        const float glowR = 22.f;
        sf::CircleShape glow(glowR);
        glow.setOrigin({ glowR, glowR });
        glow.setPosition(gem.pos);
        const auto ga = static_cast<std::uint8_t>(
            30 + 25 * std::sin(gem.glowTimer * 3.f));
        glow.setFillColor(sf::Color(gd.glowColor.r, gd.glowColor.g,
                                    gd.glowColor.b, ga));
        target.draw(glow, rsAdd);

        const sf::Texture* tex = nullptr;
        if (m_tex && ti < static_cast<int>(m_tex->size()))
            tex = (*m_tex)[static_cast<std::size_t>(ti)];
        if (tex && tex->getSize().x > 0u) {
            sf::Sprite spr(*tex);
            const auto tsz = tex->getSize();
            const float  s = 28.f / static_cast<float>(std::max(tsz.x, tsz.y));
            spr.setScale({ s, s });
            spr.setOrigin({
                static_cast<float>(tsz.x) * 0.5f,
                static_cast<float>(tsz.y) * 0.5f });
            spr.setPosition(gem.pos);
            target.draw(spr);
        } else {
            sf::ConvexShape hex;
            hex.setPointCount(6);
            for (int i = 0; i < 6; ++i) {
                const float a =
                    static_cast<float>(i) * 3.14159f / 3.f
                    + gem.glowTimer * 0.4f;
                hex.setPoint(i, { std::cos(a) * 11.f, std::sin(a) * 11.f });
            }
            hex.setPosition(gem.pos);
            hex.setFillColor(gd.color);
            hex.setOutlineColor(gd.glowColor);
            hex.setOutlineThickness(2.f);
            target.draw(hex);
        }
    }
}
