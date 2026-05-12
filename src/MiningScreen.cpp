#include "MiningScreen.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <array>
#include <cstdint>

namespace {

/// Zelfde hash-truc als `GameState::zoneNameFor`: per zone een vaste, donkere
/// kleur (unsigned modulo — nooit negatieve index).
[[nodiscard]] sf::Color normalMiningBackdrop(int zone) {
    const unsigned uz = static_cast<unsigned>(std::max(1, zone));
    auto             ch = [&](unsigned salt, int lo, unsigned span) -> uint8_t {
        const unsigned mix = (uz * 2654435761u) ^ (salt * 2246822519u);
        const int      v   = lo + static_cast<int>(mix % span);
        return static_cast<uint8_t>(std::clamp(v, 0, 255));
    };
    // Iets hogere basis + bredere span → zones duidelijker van elkaar te onderscheiden.
    return sf::Color(ch(1u, 5, 40u), ch(2u, 7, 46u), ch(3u, 12, 54u));
}

[[nodiscard]] sf::Color bonusMiningBackdrop(OreRarity r) {
    switch (r) {
        case OreRarity::COMMON:
            return sf::Color(22, 14, 36);
        case OreRarity::UNCOMMON:
            return sf::Color(14, 26, 38);
        case OreRarity::RARE:
            return sf::Color(12, 16, 48);
        case OreRarity::EPIC:
            return sf::Color(40, 12, 52);
        case OreRarity::MYTHIC:
            return sf::Color(48, 10, 30);
        case OreRarity::LEGENDARY:
            return sf::Color(44, 24, 10);
        default:
            return sf::Color(18, 12, 32);
    }
}

[[nodiscard]] sf::Color miningBackdropBase(const GameState& state) {
    if (state.isBonusZone)
        return bonusMiningBackdrop(state.bonusZoneRarity);
    return normalMiningBackdrop(state.currentLevel);
}

/// Lichte RGB-nudge voor sterren in gewone zones (past bij zone-tint).
[[nodiscard]] std::array<int, 3> normalZoneStarRgbBias(int zone) {
    const unsigned uz = static_cast<unsigned>(std::max(1, zone));
    auto             d = [&](unsigned salt, unsigned span) -> int {
        const unsigned mix = (uz * 2654435761u) ^ (salt * 2246822519u);
        return static_cast<int>(mix % span) - static_cast<int>(span / 2u);
    };
    // Sterkere tint op sterren (ongeveer ±6 i.p.v. ±3).
    return { d(19u, 13u), d(29u, 13u), d(41u, 13u) };
}

} // namespace

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
MiningScreen::MiningScreen()
    : m_particles(MAX_PARTICLES) {
    m_audio = &gSfx;
}
void MiningScreen::setAudioBus(IAudioBus* audioBus) {
    if (audioBus)
        m_audio = audioBus;
}



// ═════════════════════════════════════════════════════════════
//  init
// ═════════════════════════════════════════════════════════════
void MiningScreen::init(sf::Font& font,
                         float panelX, float panelY,
                         float panelW, float panelH,
                         const sf::Texture* keyIconTex) {
    m_font       = &font;
    m_keyIconTex = keyIconTex;
    m_x    = panelX;
    m_y    = panelY;
    m_w    = panelW;
    m_h    = panelH;

    m_collectorPos = { m_x + m_w * 0.5f,
                       m_y + m_h * 0.5f };

    m_player.init(m_x + m_w * 0.5f,
                  m_y + m_h * 0.5f);

    buildStarfield();

    if (m_playerShipTex.loadFromFile(resolveAssetPath("assets/player_ship.png"))) {
        m_playerShipTex.setSmooth(true);
        m_player.setShipSprite(&m_playerShipTex);
    } else
        m_player.setShipSprite(nullptr);
}

// ─────────────────────────────────────────────────────────────
//  buildStarfield
// ─────────────────────────────────────────────────────────────
void MiningScreen::buildStarfield() {
    for (auto& s : m_stars) {
        s.pos        = { randFloat(m_x, m_x + m_w),
                         randFloat(m_y, m_y + m_h) };
        s.speed      = randFloat(4.f, 18.f);
        s.radius     = randFloat(0.5f, 2.2f);
        s.brightness = static_cast<uint8_t>(randInt(100, 255));
    }
}

namespace {

[[nodiscard]] float nebulaModF(float v, float period) {
    if (period <= 0.f)
        return 0.f;
    float m = std::fmod(v, period);
    if (m < 0.f)
        m += period;
    return m;
}

void nebulaStir(uint32_t& s) {
    s ^= s << 13u;
    s ^= s >> 17u;
    s ^= s << 5u;
}

static constexpr int         NEBULA_SOFT_LAYER_COUNT = 6;
static constexpr float       NEBULA_LAYER_RAD[NEBULA_SOFT_LAYER_COUNT] = {
    0.25f, 0.45f, 0.60f, 0.75f, 0.88f, 1.00f};
static constexpr float       NEBULA_LAYER_AMUL[NEBULA_SOFT_LAYER_COUNT] = {
    0.90f, 0.70f, 0.50f, 0.30f, 0.15f, 0.06f};

[[nodiscard]] sf::Color lerpRgb(const sf::Color& a, const sf::Color& b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [&](std::uint8_t xa, std::uint8_t xb) {
        return static_cast<std::uint8_t>(
            std::round(float(xa) + t * (float(xb) - float(xa))));
    };
    return sf::Color(ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b));
}

/// Dunne gloeiende “aders” (random walk), additief — referentie: interne structuur.
void nebulaDrawFilaments(sf::RenderTarget& target,
                         float             animTime,
                         int               zone,
                         bool              isBonusZone,
                         OreRarity         bonusRarity,
                         float             mx,
                         float             my,
                         float             mw,
                         float             mh,
                         float             intensity,
                         const sf::Color&  prim,
                         const sf::Color&  sec) {
    if (!isBonusZone && zone < 5)
        return;

    const sf::RenderStates rsAdd(sf::BlendAdd);
    uint32_t              seed =
        static_cast<uint32_t>(std::max(1, zone)) * 2654435761u;
    if (isBonusZone)
        seed ^= 0xD16Eu
                ^ (static_cast<uint32_t>(bonusRarity) * 747796405u);

    const float minDim = std::max(40.f, std::min(mw, mh));

    for (int f = 0; f < 9; ++f) {
        uint32_t t = seed ^ static_cast<uint32_t>(f * 92837111u);
        nebulaStir(t);
        float x = mx + (t & 0xFFFFu) / 65536.f * mw;
        nebulaStir(t);
        float y = my + (t & 0xFFFFu) / 65536.f * mh;
        nebulaStir(t);
        float ang = (t & 4095u) / 4096.f * 6.2831853f;
        const int   nseg  = 24;
        const float step  = minDim * 0.016f;
        float       thick = 1.85f;
        if (zone >= 10 && zone < 15 && !isBonusZone)
            thick = 2.45f;
        if (isBonusZone && bonusRarity >= OreRarity::EPIC)
            thick = 2.1f;

        for (int si = 0; si < nseg; ++si) {
            nebulaStir(t);
            ang += ((t & 2047u) / 2047.f - 0.5f) * 0.62f;
            const float x2 = x + std::cos(ang) * step;
            const float y2 = y + std::sin(ang) * step;
            const float len = std::max(1.f, std::hypot(x2 - x, y2 - y));
            const float midx = (x + x2) * 0.5f;
            const float midy = (y + y2) * 0.5f;
            const float deg =
                std::atan2(y2 - y, x2 - x) * 180.f / 3.14159265358979323846f;

            sf::RectangleShape seg(
                { len, thick + static_cast<float>(t & 7u) * 0.11f });
            seg.setOrigin({ len * 0.5f, seg.getSize().y * 0.5f });
            seg.setPosition({ midx, midy });
            seg.setRotation(sf::degrees(deg));
            const float   u = static_cast<float>(si) / static_cast<float>(nseg - 1);
            sf::Color     c = lerpRgb(prim, sec, u);
            const float   pulse =
                std::sin(animTime * 0.5f + si * 0.17f + f * 0.4f) * 0.22f + 0.78f;
            const float   boost =
                (isBonusZone && bonusRarity >= OreRarity::LEGENDARY) ? 1.35f : 1.f;
            const auto alpha = static_cast<std::uint8_t>(std::clamp(
                22.f * intensity * pulse * boost, 5.f, 115.f));
            c.a = alpha;
            seg.setFillColor(c);
            target.draw(seg, rsAdd);
            x = x2;
            y = y2;
        }
    }
}

} // namespace

float MiningScreen::nebulaIntensityForZone(int      zone,
                                           bool     isBonusZone,
                                           OreRarity bonusRarity) {
    if (isBonusZone) {
        const int ri = std::clamp(static_cast<int>(bonusRarity), 0, 5);
        return 0.7f + (static_cast<float>(ri) / 5.f) * 0.3f;
    }
    if (zone < 5)
        return 0.f;
    if (zone < 10)
        return 0.4f;
    if (zone < 20)
        return 0.6f;
    if (zone < 30)
        return 0.75f;
    return 0.85f;
}

sf::Color MiningScreen::nebulaColorForZone(int      zone,
                                          bool     isBonusZone,
                                          OreRarity bonusRarity,
                                          bool     secondary) {
    if (isBonusZone) {
        const int ri = std::clamp(static_cast<int>(bonusRarity), 0, 5);
        // Warm goud / amber naar legendarisch “zonnecore” (referentie bonus).
        static constexpr std::uint8_t kBonSecR[] = { 115, 125, 138, 152, 172,
                                                      195 };
        static constexpr std::uint8_t kBonSecG[] = { 38,  44,  50,  54,  62,
                                                      72 };
        static constexpr std::uint8_t kBonSecB[] = { 10,  12,  12,  11,  10,
                                                      14 };
        static constexpr std::uint8_t kBonPriR[] = { 215, 225, 235, 245, 252,
                                                      255 };
        static constexpr std::uint8_t kBonPriG[] = { 150, 165, 185, 200, 218,
                                                      235 };
        static constexpr std::uint8_t kBonPriB[] = { 55,  48,  42,  38,  32,
                                                      28 };
        if (secondary)
            return { kBonSecR[ri], kBonSecG[ri], kBonSecB[ri] };
        return { kBonPriR[ri], kBonPriG[ri], kBonPriB[ri] };
    }
    if (zone < 5)
        return sf::Color(0, 0, 0, 0);
    // Zone 5–9: cyan kern / diep blauw volume (blauwe nevel-referentie).
    if (zone < 10) {
        return secondary ? sf::Color(18, 52, 105)
                         : sf::Color(130, 218, 255);
    }
    // Zone 10–14: fel oranje-rode highlights / diep karmozijn (rode nevel).
    if (zone < 15) {
        return secondary ? sf::Color(72, 8, 14)
                         : sf::Color(255, 185, 140);
    }
    if (zone < 20)
        return secondary ? sf::Color(18, 95, 82) : sf::Color(35, 185, 105);
    if (zone < 30)
        return secondary ? sf::Color(52, 14, 118) : sf::Color(200, 120, 255);
    if (zone < 50)
        return secondary ? sf::Color(22, 4, 48) : sf::Color(130, 70, 210);
    return secondary ? sf::Color(8, 14, 58) : sf::Color(70, 120, 220);
}

void MiningScreen::buildNebulaClouds(int      zone,
                                     bool     isBonusZone,
                                     OreRarity bonusRarity) {
    uint32_t s = static_cast<uint32_t>(std::max(1, zone)) * 2654435761u;
    if (isBonusZone)
        s ^= 0xA341316Cu;
    s ^= static_cast<uint32_t>(bonusRarity) * 2246822519u;

    auto uf = [&]() -> float {
        nebulaStir(s);
        return (s & 0xFFFFFFu) / float(0x1000000u);
    };

    const float intensity =
        nebulaIntensityForZone(zone, isBonusZone, bonusRarity);

    for (int i = 0; i < NEBULA_CLOUD_COUNT; ++i) {
        NebulaCloud& c = m_nebulaClouds[static_cast<std::size_t>(i)];
        // Tot ~60% buiten de rand voor organischer overlap.
        c.basePos.x = m_x - m_w * 0.60f + uf() * (m_w * 2.20f);
        c.basePos.y = m_y - m_h * 0.60f + uf() * (m_h * 2.20f);
        c.radiusX   = (0.15f + uf() * 0.30f) * m_w;
        c.radiusY   = c.radiusX * (0.4f + uf() * 0.5f);
        c.rotation  = uf() * 360.f;
        c.alpha       = (15.f + uf() * 25.f) * (0.88f + 0.12f * intensity);
        c.driftX      = uf() * 16.f - 8.f;
        c.driftY      = uf() * 16.f - 8.f;
        c.phase       = uf() * 6.2831853f;
    }

    for (int i = 0; i < NEBULA_HIGHLIGHT_COUNT; ++i) {
        NebulaCloud& c = m_nebulaHighlights[static_cast<std::size_t>(i)];
        c.basePos.x = m_x - m_w * 0.55f + uf() * (m_w * 2.10f);
        c.basePos.y = m_y - m_h * 0.55f + uf() * (m_h * 2.10f);
        c.radiusX   = (0.034f + uf() * 0.086f) * m_w;
        c.radiusY   = c.radiusX * (0.45f + uf() * 0.45f);
        c.rotation  = uf() * 360.f;
        c.alpha     = 48.f + uf() * 52.f;
        c.driftX    = uf() * 12.f - 6.f;
        c.driftY    = uf() * 12.f - 6.f;
        c.phase     = uf() * 6.2831853f;
    }
}

void MiningScreen::drawNebula(sf::RenderTarget& target,
                              float             animTime,
                              int               zone,
                              bool              isBonusZone,
                              OreRarity         bonusRarity) const {
    const float intensity =
        nebulaIntensityForZone(zone, isBonusZone, bonusRarity);
    if (intensity <= 0.f)
        return;

    const float spanX = m_w + 320.f;
    const float spanY = m_h + 320.f;

    const sf::Color prim = nebulaColorForZone(zone, isBonusZone, bonusRarity, false);
    const sf::Color sec  = nebulaColorForZone(zone, isBonusZone, bonusRarity, true);

    const sf::RenderStates rsAdd(sf::BlendAdd);

    auto drawSoftCloud = [&](const NebulaCloud& cloud,
                             float               extraAlphaScale,
                             const sf::RenderStates& rs) {
        const float ox =
            nebulaModF(animTime * cloud.driftX + cloud.phase * 12.f, spanX);
        const float oy =
            nebulaModF(animTime * cloud.driftY + cloud.phase * 9.3f, spanY);
        const float px = cloud.basePos.x + ox - spanX * 0.5f;
        const float py = cloud.basePos.y + oy - spanY * 0.5f;
        const float pulse =
            std::sin(animTime * 0.3f + cloud.phase) * 0.15f + 0.85f;

        for (int li = NEBULA_SOFT_LAYER_COUNT - 1; li >= 0; --li) {
            const float     t = static_cast<float>(li) / 5.f;
            const sf::Color rgb = lerpRgb(prim, sec, t);
            const float     radX = cloud.radiusX * NEBULA_LAYER_RAD[li];
            const float     radY = cloud.radiusY * NEBULA_LAYER_RAD[li];
            if (radX < 0.5f)
                continue;
            const float aFloat = std::clamp(
                cloud.alpha * NEBULA_LAYER_AMUL[li] * pulse * intensity
                    * extraAlphaScale,
                0.f, 255.f);
            const auto a = static_cast<std::uint8_t>(aFloat);

            sf::CircleShape blob(radX);
            blob.setOrigin({ radX, radX });
            blob.setPosition({ px, py });
            blob.setScale({ 1.f, radY / radX });
            blob.setRotation(sf::degrees(cloud.rotation));
            blob.setFillColor(sf::Color(rgb.r, rgb.g, rgb.b, a));
            blob.setOutlineThickness(0.f);
            target.draw(blob, rs);
        }
    };

    for (int i = 0; i < NEBULA_CLOUD_COUNT; ++i)
        drawSoftCloud(m_nebulaClouds[static_cast<std::size_t>(i)], 1.f,
                      sf::RenderStates::Default);

    nebulaDrawFilaments(target, animTime, zone, isBonusZone, bonusRarity, m_x,
                        m_y, m_w, m_h, intensity, prim, sec);

    for (int i = 0; i < NEBULA_HIGHLIGHT_COUNT; ++i)
        drawSoftCloud(m_nebulaHighlights[static_cast<std::size_t>(i)], 0.42f,
                      rsAdd);

    if (isBonusZone && bonusRarity >= OreRarity::LEGENDARY) {
        const int   nParticles = 26;
        const float wrapH      = m_h + 100.f;
        for (int i = 0; i < nParticles; ++i) {
            uint32_t h = static_cast<uint32_t>(std::max(1, zone)) * 2654435761u
                       ^ static_cast<uint32_t>(i + 1) * 2246822519u;
            nebulaStir(h);
            const float xNorm = (h & 0xFFFFu) / 65536.f;
            nebulaStir(h);
            const float speed = 22.f + static_cast<float>((h >> 8) % 40) * 0.35f;
            const float yOff =
                nebulaModF(animTime * speed + static_cast<float>(i * 37), wrapH);
            const float px = m_x + xNorm * m_w;
            const float py = m_y + yOff - 40.f;
            nebulaStir(h);
            const float rad = 1.f + static_cast<float>((h >> 4) % 30) * 0.067f;
            nebulaStir(h);
            const auto alpha = static_cast<std::uint8_t>(
                60 + static_cast<int>((h >> 3) % 61));

            sf::CircleShape dust(rad);
            dust.setOrigin({ rad, rad });
            dust.setPosition({ px, py });
            dust.setFillColor(sf::Color(255, 200, 50, alpha));
            target.draw(dust);
        }
    }
}

void MiningScreen::rebuildNebulaBaseTexture(int      zone,
                                            bool     isBonusZone,
                                            OreRarity bonusRarity) {
    const float intensity =
        nebulaIntensityForZone(zone, isBonusZone, bonusRarity);
    const unsigned uw =
        std::max(1u, static_cast<unsigned>(std::ceil(std::max(1.f, m_w))));
    const unsigned uh =
        std::max(1u, static_cast<unsigned>(std::ceil(std::max(1.f, m_h))));

    if (m_nebulaBaseRtt.getSize().x != uw || m_nebulaBaseRtt.getSize().y != uh) {
        if (!m_nebulaBaseRtt.resize({ uw, uh })) {
            m_nebulaBaseRttReady = false;
            return;
        }
        m_nebulaBaseRtt.setSmooth(true);
    }

    m_nebulaBaseRtt.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, { m_w, m_h })));
    m_nebulaBaseRtt.clear(sf::Color(0, 0, 0, 0));

    const sf::Color prim = nebulaColorForZone(zone, isBonusZone, bonusRarity, false);
    const sf::Color sec  = nebulaColorForZone(zone, isBonusZone, bonusRarity, true);

    if (intensity > 0.f) {
        sf::VertexArray grad(sf::PrimitiveType::TriangleStrip, 4);
        const auto aCorn = static_cast<std::uint8_t>(std::clamp(
            55.f * intensity, 18.f, 125.f));
        const auto aMid = static_cast<std::uint8_t>(std::clamp(
            82.f * intensity, 22.f, 145.f));
        grad[0] = { { 0.f, 0.f }, sf::Color(sec.r, sec.g, sec.b, aCorn) };
        grad[1] = { { m_w, 0.f },
                    sf::Color(prim.r, prim.g, prim.b,
                              static_cast<std::uint8_t>(aMid * 0.88f)) };
        grad[2] = { { 0.f, m_h },
                    sf::Color(prim.r, prim.g, prim.b,
                              static_cast<std::uint8_t>(aMid * 0.88f)) };
        grad[3] = { { m_w, m_h }, sf::Color(sec.r, sec.g, sec.b, aCorn) };
        m_nebulaBaseRtt.draw(grad);

        uint32_t rng = static_cast<uint32_t>(std::max(1, zone)) * 2654435761u;
        if (isBonusZone)
            rng ^= 0x91EC0Eu;
        rng ^= static_cast<uint32_t>(bonusRarity) * 1597334677u;
        auto uf = [&]() -> float {
            nebulaStir(rng);
            return (rng & 0xFFFFFFu) / float(0x1000000u);
        };

        const float maxDim = std::max(m_w, m_h);
        for (int k = 0; k < 5; ++k) {
            const float cx = uf() * m_w;
            const float cy = uf() * m_h;
            const float rMax = maxDim * (0.40f + uf() * 0.42f);
            const float ryS  = 0.48f + uf() * 0.48f;
            const float rot  = uf() * 360.f;
            for (int li = NEBULA_SOFT_LAYER_COUNT - 1; li >= 0; --li) {
                const float     t = static_cast<float>(li) / 5.f;
                const sf::Color rgb = lerpRgb(prim, sec, t);
                const float     rad = rMax * NEBULA_LAYER_RAD[li];
                if (rad < 8.f)
                    continue;
                const float a = std::clamp(
                    30.f * NEBULA_LAYER_AMUL[li] * intensity, 8.f, 130.f);
                sf::CircleShape blob(rad);
                blob.setOrigin({ rad, rad });
                blob.setPosition({ cx, cy });
                blob.setScale({ 1.f, ryS });
                blob.setRotation(sf::degrees(rot));
                blob.setFillColor(sf::Color(
                    rgb.r, rgb.g, rgb.b, static_cast<std::uint8_t>(a)));
                blob.setOutlineThickness(0.f);
                m_nebulaBaseRtt.draw(blob);
            }
        }
    }

    m_nebulaBaseRtt.display();
    m_nebulaBaseRttReady = true;
}

void MiningScreen::drawNebulaTexture(sf::RenderTarget& target,
                                      const GameState&  state) const {
    if (!m_nebulaBaseRttReady)
        return;
    if (nebulaIntensityForZone(state.currentLevel, state.isBonusZone,
                               state.bonusZoneRarity)
        <= 0.f)
        return;

    const sf::Texture& tex = m_nebulaBaseRtt.getTexture();
    const auto         tsz = tex.getSize();
    if (tsz.x == 0u || tsz.y == 0u)
        return;

    sf::Sprite spr(tex);
    spr.setPosition({ m_x, m_y });
    spr.setScale({ m_w / static_cast<float>(tsz.x),
                   m_h / static_cast<float>(tsz.y) });
    target.draw(spr);
}

// ═════════════════════════════════════════════════════════════
//  syncTurrets
// ═════════════════════════════════════════════════════════════
void MiningScreen::syncTurrets(const GameState& state) {
    int cnt = state.turretCount();
    if (cnt != m_lastTurretCnt) {
        m_turrets.setCount(cnt, m_w, m_h);
        m_lastTurretCnt = cnt;
    }
}

// ─────────────────────────────────────────────────────────────
//  targetAsteroidCount
// ─────────────────────────────────────────────────────────────
int MiningScreen::targetAsteroidCount(int turrets) {
    return std::min(6 + turrets * 3, MAX_ASTEROIDS);
}
// health player hit
bool MiningScreen::playerHit() const {
    return m_player.wasHit();
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void MiningScreen::update(float      dt,
                            GameState& state,
                            double&    creditsEarned,
                            double&    oreEarned,
                            std::array<double, ORE_TIER_COUNT>& oreByTierEarned,
                            float      warpChargeStars) {
    //
    m_player.clearHit();
    // ── Sync turrets ──────────────────────────────────────
    syncTurrets(state);

    const int z = state.currentLevel;
    const int iw = static_cast<int>(std::ceil(std::max(1.f, m_w)));
    const int ih = static_cast<int>(std::ceil(std::max(1.f, m_h)));
    if (z != m_lastNebulaZone || state.isBonusZone != m_lastNebulaBonus
        || state.bonusZoneRarity != m_lastNebulaRarity || iw != m_nebulaCachedIw
        || ih != m_nebulaCachedIh) {
        buildNebulaClouds(z, state.isBonusZone, state.bonusZoneRarity);
        rebuildNebulaBaseTexture(z, state.isBonusZone, state.bonusZoneRarity);
        m_lastNebulaZone    = z;
        m_lastNebulaBonus   = state.isBonusZone;
        m_lastNebulaRarity  = state.bonusZoneRarity;
        m_nebulaCachedIw    = iw;
        m_nebulaCachedIh    = ih;
    }

    // ── Player update ─────────────────────────────────────
    m_player.update(dt,
                    1.f / state.fireRatePerSec(),
                    state.gunDamage(),
                    state.critChance(),
                    state.critMult(),
                    state.splitShot(),
                    state.bulletLifetimeSec(),
                    m_x, m_y, m_w, m_h,
                    m_asteroids,
                    m_bullets,
                    m_particles);

    // Collector volgt speler
    m_collectorPos = m_player.pos;

    // Turrets volgen speler
    m_turrets.followPlayer(m_player.pos);

    // ── HP multiplier ─────────────────────────────────────
    float hpMult = std::max(0.1f,
        1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);

    const float asteroidHp =
        hpMult * state.levelHpMult() * state.difficultyAsteroidHpMult();

    // ── Asteroid field ────────────────────────────────────
    int target = targetAsteroidCount(state.turretCount());
    int spawnTarget =
        std::min(target + state.levelSpawnBonus(),
                 MAX_ASTEROIDS - ASTEROID_POOL_EVENT_HEADROOM);
    m_asteroids.maintainField(spawnTarget, m_x, m_y, m_w, m_h, asteroidHp,
                              state.maxOreTier());
    m_asteroids.tickMeteorSpawnQueue();
    m_asteroids.update(dt, m_x, m_y, m_w, m_h, m_player.pos);

    // ── Turrets ───────────────────────────────────────────
    m_turrets.update(dt,
                     1.f / state.fireRatePerSec(),
                     state.gunDamage(),
                     state.critChance(),
                     state.critMult(),
                     state.splitShot(),
                     state.bulletLifetimeSec(),
                     m_asteroids,
                     m_bullets,
                     m_particles);

    // ── Bullets ───────────────────────────────────────────
    m_bullets.update(dt, m_x, m_y, m_w, m_h);

    // ── Collisions ────────────────────────────────────────
    resolveCollisions(state);

    // ── Ore collectie (alleen ore, geen credits) ──────────
    double oreThisFrame = 0.0;
    m_ores.update(dt,
                  m_collectorPos,
                  state.autoCollectRadius(),
                  oreThisFrame,
                  &oreByTierEarned,
                  state.bulkProcess(),
                  m_particles);
    oreEarned += oreThisFrame;

    int keysThisFrame = 0;
    m_keyPickups.update(dt,
                        m_collectorPos,
                        state.autoCollectRadius(),
                        keysThisFrame,
                        m_particles);
    if (keysThisFrame > 0) {
        state.addKeys(keysThisFrame);
        m_pendingKeyDrop += keysThisFrame;
    }

    // ── Particles ─────────────────────────────────────────
    if (state.isBonusZone
        && state.bonusZoneRarity == OreRarity::LEGENDARY) {
        for (int i = 0; i < 4; ++i) {
            const float x = randFloat(m_x + 24.f, m_x + m_w - 24.f);
            m_particles.emitOrePieces(
                sf::Vector2f(x, m_y + randFloat(-6.f, 18.f)),
                sf::Color(255, 215, 120, 235), 1);
        }
    }
    m_particles.update(dt);

    // ── Sterren scrollen (warp opladen; steeds sneller naar einde) ───
    const float w = std::clamp(warpChargeStars, 0.f, 1.f);
    const float we = std::pow(w, WARP_STAR_STREAK_POW);
    const float streak =
        (1.f + we * WARP_STAR_STREAK_LINEAR + we * we * WARP_STAR_STREAK_QUAD)
        * WARP_STAR_STREAK_SCALE;
    for (auto& s : m_stars) {
        s.pos.y += s.speed * dt * streak;
        if (s.pos.y > m_y + m_h + 4.f)
            s.pos = { randFloat(m_x, m_x + m_w), m_y - 4.f };
    }
}

void MiningScreen::advanceMeteorsOnly(float dt) {
    m_asteroids.updateMeteorsOnly(dt, m_x, m_y, m_w, m_h, m_player.pos);
}

void MiningScreen::tickMeteorSpawnQueue() {
    m_asteroids.tickMeteorSpawnQueue();
}

void MiningScreen::resetMeteorShowerSchedule() {
    m_meteorTimeToNext = -1.f;
}

void MiningScreen::tickMeteorShower(float dt, GameState& state,
                                    float asteroidHpMult) {
    if (m_meteorTimeToNext < 0.f)
        m_meteorTimeToNext = state.meteorShowerIntervalSec();
    const float interval = state.meteorShowerIntervalSec();
    m_meteorTimeToNext -= dt;
    if (m_meteorTimeToNext <= 0.f) {
        m_asteroids.spawnMeteorSwarm(m_x, m_y, m_w, m_h,
                                     state.meteorShowerMeteorCount(),
                                     asteroidHpMult,
                                     state.maxOreTier());
        m_meteorTimeToNext = interval;
    }
}

// ═════════════════════════════════════════════════════════════
//  resolveCollisions
// ═════════════════════════════════════════════════════════════
void MiningScreen::resolveCollisions(GameState& state) {
    for (auto& bullet : m_bullets.all()) {
        if (!bullet.alive) continue;

        for (auto& asteroid : m_asteroids.all()) {
            if (!asteroid.alive) continue;

            float dist = distance(bullet.pos, asteroid.pos);
            if (dist > bullet.radius + asteroid.radius)
                continue;

            bullet.alive = false;

            bool destroyed = asteroid.hit(bullet.damage, m_particles);
            if (destroyed) {
                const double bzMult =
                    state.isBonusZone
                        ? static_cast<double>(state.bonusZoneOreValueMult())
                        : 1.0;
                if (asteroid.isBoss)
                    m_audio->play(Sfx::BossExplode);
                else
                    m_audio->play(Sfx::Explosion);
                if (asteroid.isKeyAsteroid) {
                    const int nk = asteroid.keyPickupCount >= 0
                        ? asteroid.keyPickupCount
                        : randInt(1, 3);
                    m_keyPickups.drop(asteroid.pos, nk, m_particles);
                    m_ores.drop(
                        asteroid.pos,
                        asteroid.oreDrop.color,
                        asteroid.oreDrop.value * bzMult,
                        asteroid.oreDrop.count,
                        asteroid.oreTier,
                        state.oreLuckBonus(),
                        m_particles);
                } else if (asteroid.isBoss) {
                    int count =
                        asteroid.oreDrop.count * asteroid.rarityDropMult();
                    m_ores.drop(
                        asteroid.pos,
                        asteroid.oreDrop.color,
                        asteroid.oreDrop.value * bzMult,
                        count,
                        asteroid.oreTier,
                        state.oreLuckBonus(),
                        m_particles);
                    if (static_cast<int>(state.maxOreTier())
                        >= static_cast<int>(OreTier::GOLD)) {
                        m_ores.drop(
                            asteroid.pos,
                            sf::Color(255, 215, 50),
                            20.0 * bzMult,
                            26,
                            OreTier::GOLD,
                            state.oreLuckBonus(),
                            m_particles);
                    }
                    if (static_cast<int>(state.maxOreTier())
                        >= static_cast<int>(OreTier::SILVER)) {
                        m_ores.drop(
                            asteroid.pos,
                            sf::Color(210, 215, 225),
                            8.0 * bzMult,
                            20,
                            OreTier::SILVER,
                            state.oreLuckBonus(),
                            m_particles);
                    }
                    state.registerBossDefeated();
                    m_pendingBossReturnToBase = true;
                } else {
                    int count =
                        asteroid.oreDrop.count * asteroid.rarityDropMult();
                    m_ores.drop(
                        asteroid.pos,
                        asteroid.oreDrop.color,
                        asteroid.oreDrop.value * bzMult,
                        count,
                        asteroid.oreTier,
                        state.oreLuckBonus(),
                        m_particles);
                }
            }
            break;
        }
    }
}

// ═════════════════════════════════════════════════════════════
//  collectAllOre
// ═════════════════════════════════════════════════════════════
void MiningScreen::collectAllOre(double& oreOut,
                                 std::array<double, ORE_TIER_COUNT>& oreByTierOut,
                                 GameState& state) {
    m_ores.collectAll(oreOut, &oreByTierOut, state.bulkProcess());
    int k = 0;
    m_keyPickups.collectAll(k);
    state.addKeys(k);
    m_pendingKeyDrop += k;
}

// ═════════════════════════════════════════════════════════════
//  clearAll
// ═════════════════════════════════════════════════════════════
void MiningScreen::clearAll() {
    m_ores.clearAll();
    m_keyPickups.clearAll();
    for (auto& b : m_bullets.all()) b.alive = false;
    for (auto& a : m_asteroids.all()) a.alive = false;
    m_asteroids.clearMeteorSpawnQueue();
    m_pendingBossReturnToBase = false;
    resetMeteorShowerSchedule();
}

void MiningScreen::prepareNewRun() {
    clearAll();
    m_pendingKeyDrop = 0;
    m_player.init(m_x + m_w * 0.5f, m_y + m_h * 0.5f);
    m_lastTurretCnt   = -1;
    m_lastNebulaZone  = -1;
    m_lastNebulaBonus = false;
    m_lastNebulaRarity = OreRarity::COMMON;
    m_nebulaCachedIw   = -1;
    m_nebulaCachedIh   = -1;
    m_nebulaBaseRttReady = false;
}

bool MiningScreen::pullBossReturnToBase() {
    if (!m_pendingBossReturnToBase)
        return false;
    if (m_ores.aliveCount() > 0 || m_keyPickups.aliveCount() > 0)
        return false;
    m_pendingBossReturnToBase = false;
    return true;
}

bool MiningScreen::trySpawnKeyAsteroid(GameState& state) {
    float hpMult = std::max(0.1f,
        1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);
    hpMult *= state.levelHpMult();
    hpMult *= state.difficultyAsteroidHpMult();
    const OreTier kt =
        GameState::clampKeyOreTier(OreTier::GOLD, state.maxOreTier());
    return m_asteroids.trySpawnKey(m_x, m_y, m_w, m_h, hpMult, kt, -1);
}

void MiningScreen::spawnBonusZoneKeys(const GameState& state) {
    if (!state.isBonusZone || !state.keyAsteroidsEnabled)
        return;
    float hpMult = std::max(0.1f,
        1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);
    hpMult *= state.levelHpMult();
    hpMult *= state.difficultyAsteroidHpMult();
    const OreTier ot = GameState::clampKeyOreTier(state.bonusZoneMinOreTier(),
                                                  state.maxOreTier());

    auto one = [&](int keys) {
        m_asteroids.spawnBonusKeyAsteroid(m_x, m_y, m_w, m_h, hpMult, ot, keys);
    };

    switch (state.bonusZoneRarity) {
        case OreRarity::COMMON:
            one(1);
            break;
        case OreRarity::UNCOMMON:
            one(randInt(1, 2));
            break;
        case OreRarity::RARE:
            one(randInt(1, 2));
            one(randInt(1, 2));
            break;
        case OreRarity::EPIC:
            one(randInt(2, 3));
            one(randInt(2, 3));
            break;
        case OreRarity::MYTHIC:
            one(randInt(2, 3));
            one(randInt(2, 3));
            one(randInt(2, 3));
            break;
        case OreRarity::LEGENDARY:
            one(3);
            one(3);
            one(3);
            break;
        default:
            break;
    }
}

bool MiningScreen::hasLivingBoss() const {
    return m_asteroids.hasLivingBoss();
}

bool MiningScreen::trySpawnBoss(GameState& state) {
    if (state.currentLevel != state.nextBossMilestone)
        return false;
    float hpMult = std::max(0.1f,
        1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);
    hpMult *= state.levelHpMult();
    if (state.nextBossMilestone == 3) {
        // Eerste boss bewust toegankelijker maken.
        hpMult *= 0.5f;
    }
    return m_asteroids.trySpawnBoss(m_x, m_y, m_w, m_h, hpMult,
                                    state.maxOreTier());
}

int MiningScreen::pullPendingKeyDrop() {
    int k       = m_pendingKeyDrop;
    m_pendingKeyDrop = 0;
    return k;
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void MiningScreen::draw(sf::RenderTarget& target,
                         const GameState&  state,
                         float             warpCharge,
                         float             warpFlashRemain,
                         float             animTime) const {

    sf::View oldView = target.getView();

    // Viewport moet t.o.v. de echte venstergrootte — niet WINDOW_* constanten
    // (anders klopt onder/rechts niet bij andere desktop-resoluties).
    const auto   pxSz = target.getSize();
    const float  rw   = std::max(1.f, static_cast<float>(pxSz.x));
    const float  rh   = std::max(1.f, static_cast<float>(pxSz.y));
    float vpX = std::clamp(m_x / rw, 0.f, 1.f);
    float vpY = std::clamp(m_y / rh, 0.f, 1.f);
    float vpW = std::clamp(m_w / rw, 0.001f, 1.f);
    float vpH = std::clamp(m_h / rh, 0.001f, 1.f);
    vpW        = std::min(vpW, 1.f - vpX);
    vpH        = std::min(vpH, 1.f - vpY);

    sf::View mineView(sf::FloatRect(
        { m_x, m_y }, { m_w, m_h }));
    mineView.setViewport(sf::FloatRect(
        { vpX, vpY }, { vpW, vpH }));
    target.setView(mineView);

    // ── Background ────────────────────────────────────────
    sf::RectangleShape bg(sf::Vector2f{ m_w, m_h });
    bg.setPosition({ m_x, m_y });
    bg.setFillColor(miningBackdropBase(state));
    target.draw(bg);

    drawNebulaTexture(target, state);
    drawNebula(target, animTime, state.currentLevel, state.isBonusZone,
               state.bonusZoneRarity);
    drawStarfield(target, warpCharge, state);
    drawZoneBackground(target, state, animTime);
    drawCollectRing(target, state);
    drawCollector(target);

    // ── Entities ──────────────────────────────────────────
    m_asteroids.draw(target, animTime, m_font, m_keyIconTex);
    m_ores.draw(target);
    m_keyPickups.draw(target, m_keyIconTex);
    m_bullets.draw(target);
    m_turrets.draw(target);
    m_player.draw(target);
    if (m_font)
        m_particles.draw(target, *m_font);

    drawWarpFlashOverlay(target, warpFlashRemain);

    target.setView(oldView);

    // ── HUD (buiten clipped view) ─────────────────────────
    drawHUD(target, state, warpCharge);

}

// ─────────────────────────────────────────────────────────────
//  drawStarfield
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawStarfield(sf::RenderTarget& target,
                                 float             warpCharge,
                                 const GameState& state) const {
    const float w  = std::clamp(warpCharge, 0.f, 1.f);
    const float we = std::pow(w, WARP_STAR_STREAK_POW);
    const float glow =
        1.f + we * 1.1f * WARP_STAR_STREAK_SCALE; // helderder naar einde oplading
    sf::CircleShape star;
    for (const auto& s : m_stars) {
        star.setRadius(s.radius);
        star.setOrigin({ s.radius, s.radius });
        star.setPosition(s.pos);
        const int base = std::min(
            255,
            static_cast<int>(static_cast<float>(s.brightness) * glow));
        uint8_t cr = static_cast<uint8_t>(base);
        uint8_t cg = static_cast<uint8_t>(base);
        uint8_t cb = static_cast<uint8_t>(base);
        if (state.isBonusZone) {
            switch (state.bonusZoneRarity) {
                case OreRarity::COMMON:
                    cr = static_cast<uint8_t>(std::min(255, base + 8));
                    cg = static_cast<uint8_t>(std::min(255, base + 10));
                    break;
                case OreRarity::UNCOMMON:
                    cg = static_cast<uint8_t>(
                        std::min(255, base + 20));
                    break;
                case OreRarity::RARE:
                    cb = static_cast<uint8_t>(
                        std::min(255, base + 28));
                    break;
                case OreRarity::EPIC:
                    cr = static_cast<uint8_t>(
                        std::min(255, base + 24));
                    cb = static_cast<uint8_t>(
                        std::min(255, base + 16));
                    break;
                case OreRarity::MYTHIC:
                    cr = static_cast<uint8_t>(
                        std::min(255, base + 30));
                    break;
                case OreRarity::LEGENDARY:
                    cr = static_cast<uint8_t>(
                        std::min(255, base + 28));
                    cg = static_cast<uint8_t>(
                        std::min(255, base + 18));
                    cb = static_cast<uint8_t>(
                        std::max(0, base - 4)); // warme “goud”-sterren
                    break;
                default:
                    break;
            }
        } else {
            const std::array<int, 3> bias =
                normalZoneStarRgbBias(state.currentLevel);
            cr = static_cast<uint8_t>(std::clamp(base + bias[0], 0, 255));
            cg = static_cast<uint8_t>(std::clamp(base + bias[1], 0, 255));
            cb = static_cast<uint8_t>(std::clamp(base + bias[2], 0, 255));
        }
        const uint8_t alpha = static_cast<uint8_t>(
            std::min(255, 165 + static_cast<int>(95.f * we)));
        star.setFillColor(sf::Color(cr, cg, cb, alpha));
        target.draw(star);
    }
}

void MiningScreen::drawZoneBackground(sf::RenderTarget& target,
                                      const GameState&  state,
                                      float             animTime) const {
    if (!state.isBonusZone) {
        // Gewone zone: diepte + zone-tint (hoeken donkerder, midden open),
        // licht pulserend zodat de ruimte niet statisch aanvoelt.
        const sf::Color base = normalMiningBackdrop(state.currentLevel);
        const float     cx   = m_x + m_w * 0.5f;
        const float     cy   = m_y + m_h * 0.5f;
        const float breathe =
            0.5f + 0.5f * std::sin(animTime * 0.28f);
        const auto edgeA = static_cast<std::uint8_t>(58 + breathe * 48);

        auto darkCh = [&](std::uint8_t u) {
            return static_cast<std::uint8_t>(
                std::clamp(static_cast<int>(u) * 13 / 25, 0, 255));
        };
        const sf::Color corner(darkCh(base.r), darkCh(base.g), darkCh(base.b),
                             edgeA);

        sf::VertexArray va(sf::PrimitiveType::TriangleFan, 6);
        va[0].position = { cx, cy };
        va[0].color    = sf::Color(base.r, base.g, base.b, 0);
        va[1].position = { m_x, m_y };
        va[1].color    = corner;
        va[2].position = { m_x + m_w, m_y };
        va[2].color    = corner;
        va[3].position = { m_x + m_w, m_y + m_h };
        va[3].color    = corner;
        va[4].position = { m_x, m_y + m_h };
        va[4].color    = corner;
        va[5].position = { m_x, m_y };
        va[5].color    = corner;
        target.draw(va);
        return;
    }
    sf::RectangleShape tint(sf::Vector2f{ m_w, m_h });
    tint.setPosition({ m_x, m_y });
    // Iets sterkere tint dan vroeger: anders leek de bonus-zone visueel
    // bijna gelijk aan een normale zone (alleen HUD-tekst viel op).
    sf::Color fill(255, 240, 160, 68);
    switch (state.bonusZoneRarity) {
        case OreRarity::COMMON:
            fill = sf::Color(255, 240, 160, 68);
            break;
        case OreRarity::UNCOMMON:
            fill = sf::Color(255, 230, 100, 96);
            break;
        case OreRarity::RARE:
            fill = sf::Color(255, 210, 70, 122);
            break;
        case OreRarity::EPIC:
            fill = sf::Color(255, 160, 60, 148);
            break;
        case OreRarity::MYTHIC:
            fill = sf::Color(255, 120, 40, 168);
            break;
        case OreRarity::LEGENDARY:
            fill = sf::Color(255, 210, 90, 188);
            break;
        default:
            break;
    }
    tint.setFillColor(fill);
    target.draw(tint);
}

void MiningScreen::drawWarpFlashOverlay(sf::RenderTarget& target,
                                        float remain) const {
    if (remain <= 0.f)
        return;
    const float   t = std::min(1.f, remain / WARP_FLASH_DURATION_SEC);
    const uint8_t a = static_cast<uint8_t>(
        std::min(250.f, 255.f * std::pow(t, 1.15f)));
    sf::RectangleShape flash(sf::Vector2f{ m_w, m_h });
    flash.setPosition({ m_x, m_y });
    flash.setFillColor(sf::Color(255, 250, 245, a));
    target.draw(flash);
}

// ─────────────────────────────────────────────────────────────
//  drawCollector
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawCollector(sf::RenderTarget& target) const {
    // Outer ring
    sf::CircleShape outer(18.f);
    outer.setOrigin({ 18.f, 18.f });
    outer.setPosition(m_collectorPos);
    outer.setFillColor(sf::Color(20, 30, 60, 200));
    outer.setOutlineColor(sf::Color(80, 140, 255, 200));
    outer.setOutlineThickness(3.f);
    target.draw(outer);

    // Inner core
    sf::CircleShape inner(9.f);
    inner.setOrigin({ 9.f, 9.f });
    inner.setPosition(m_collectorPos);
    inner.setFillColor(sf::Color(100, 160, 255, 230));
    target.draw(inner);

    // Centre dot
    sf::CircleShape dot(3.5f);
    dot.setOrigin({ 3.5f, 3.5f });
    dot.setPosition(m_collectorPos);
    dot.setFillColor(sf::Color(220, 235, 255));
    target.draw(dot);
}

// ─────────────────────────────────────────────────────────────
//  drawCollectRing
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawCollectRing(sf::RenderTarget& target,
                                    const GameState&  state) const {
    float r = state.autoCollectRadius();

    sf::CircleShape ring(r);
    ring.setOrigin({ r, r });
    ring.setPosition(m_collectorPos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(80, 140, 255, 35));
    ring.setOutlineThickness(1.5f);
    target.draw(ring);
}

// ─────────────────────────────────────────────────────────────
//  drawHUD
// ─────────────────────────────────────────────────────────────
void MiningScreen::drawHUD(sf::RenderTarget& target,
                             const GameState&  state,
                             float             warpCharge) const {
    if (!m_font)
        return;

    // ── Warp UI ───────────────────────────────────────────────
    if (state.warpDriveUnlocked()) {
        float barW = m_w * 0.25f;                        // 25% van schermbreed
        float barH = m_h * 0.014f;                       // ~1.4% van schermhoog
        float barX = m_x + m_w * 0.5f - barW * 0.5f;
        float barY = m_y + m_h - barH - m_h * 0.02f;
        unsigned fontSize = static_cast<unsigned>(
            std::max(10.f, m_h * 0.016f));

        // Achtergrond
        sf::RectangleShape bg(sf::Vector2f(barW, barH));
        bg.setPosition(sf::Vector2f(barX, barY));
        bg.setFillColor(sf::Color(20, 20, 40, 200));
        bg.setOutlineColor(sf::Color(80, 140, 255, 160));
        bg.setOutlineThickness(1.f);
        target.draw(bg);

        // Charge fill
        if (warpCharge > 0.f) {
            sf::RectangleShape fill(sf::Vector2f(
                barW * warpCharge, barH));
            fill.setPosition(sf::Vector2f(barX, barY));
            uint8_t g = static_cast<uint8_t>(120 + 135 * warpCharge);
            fill.setFillColor(sf::Color(60, g, 255, 220));
            target.draw(fill);
        }

        // Label
        sf::Text lbl(*m_font);
        lbl.setCharacterSize(fontSize);
        lbl.setFillColor(sf::Color(160, 200, 255));
        if (!state.canWarp())
            lbl.setString("Warp: " +
                std::to_string(static_cast<int>(state.oreThisLevel))
                + " / " +
                std::to_string(state.oreWarpRequirement()) +
                " ore");
        else if (warpCharge <= 0.f)
            lbl.setString("Warp ready - hold Space");
        else
            lbl.setString("Warping...");

        float lw = lbl.getLocalBounds().size.x;
        lbl.setPosition(sf::Vector2f(
            barX + (barW - lw) * 0.5f,
            barY - barH - fontSize * 1.2f));
        target.draw(lbl);
    }

    // Zone label — linksboven
    sf::Text zoneLabel(*m_font);
    zoneLabel.setString(state.levelLabel());
    zoneLabel.setCharacterSize(15);
    zoneLabel.setStyle(sf::Text::Bold);
    zoneLabel.setFillColor(sf::Color(180, 210, 255));
    zoneLabel.setPosition({ m_x + 8.f, m_y + 8.f });
    target.draw(zoneLabel);

    if (state.isBonusZone) {
        static const char* const rNames[] = {
            "COMMON", "UNCOMMON", "RARE", "EPIC", "MYTHIC", "LEGENDARY",
        };
        static const sf::Color rCols[] = {
            sf::Color(220, 220, 220),
            sf::Color(80, 200, 80),
            sf::Color(70, 130, 255),
            sf::Color(185, 60, 255),
            sf::Color(220, 50, 50),
            sf::Color(255, 170, 0),
        };
        const int ri =
            std::clamp(static_cast<int>(state.bonusZoneRarity), 0, 5);
        const std::string ban =
            strFromNullableUtf8(rNames[ri]) + " BONUS ZONE";
        sf::Text banT(*m_font);
        banT.setString(ban);
        banT.setCharacterSize(18);
        banT.setStyle(sf::Text::Bold);
        banT.setFillColor(rCols[ri]);
        banT.setOutlineColor(sf::Color(0, 0, 0, 180));
        banT.setOutlineThickness(2.f);
        const sf::FloatRect lb = banT.getLocalBounds();
        banT.setOrigin({ lb.position.x + lb.size.x * 0.5f, lb.position.y });
        banT.setPosition({ m_x + m_w * 0.5f, m_y + 12.f });
        target.draw(banT);
    }
    // HUD achtergrond
    sf::RectangleShape hudBg(sf::Vector2f{ 220.f, 96.f });
    hudBg.setPosition({ m_x + 5.f, m_y + 4.f });
    hudBg.setFillColor(sf::Color(4, 6, 16, 170));
    hudBg.setOutlineColor(sf::Color(40, 60, 100, 120));
    hudBg.setOutlineThickness(1.f);
    target.draw(hudBg);

    float hx    = m_x + 10.f;
    float hy    = m_y + 8.f;
    float lineH = 17.f;

    auto drawLine = [&](const std::string& s,
                         sf::Color col = sf::Color(200, 210, 240)) {
        sf::Text txt(*m_font);
        txt.setCharacterSize(13);
        txt.setString(s);
        txt.setFillColor(col);
        txt.setPosition({ hx, hy });
        target.draw(txt);
        hy += lineH;
    };

    drawLine("Asteroiden: " +
             std::to_string(m_asteroids.aliveCount()),
             sf::Color(180, 200, 255));
    drawLine("Kogels: " +
             std::to_string(m_bullets.aliveCount()),
             sf::Color(160, 230, 255));
    drawLine("Ore: " +
             std::to_string(m_ores.aliveCount()),
             sf::Color(200, 220, 140));
    drawLine("Particles: " +
             std::to_string(m_particles.alive()),
             sf::Color(160, 160, 180));
    drawLine("Turrets: " +
             std::to_string(m_turrets.activeCount()),
             sf::Color(255, 180, 100));


}
