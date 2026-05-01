#include "Plinko.h"
#include "SoundHub.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace pegc {

constexpr float kBonus[static_cast<int>(OreRarity::RARITY_COUNT)] = {
    0.f, 0.5f, 1.f, 2.f, 4.f, 8.f,
};

const sf::Color kFill[static_cast<int>(OreRarity::RARITY_COUNT)] = {
    sf::Color(140, 160, 200), // Common
    sf::Color( 80, 200,  80), // Uncommon
    sf::Color( 70, 130, 255), // Rare
    sf::Color(185,  60, 255), // Epic
    sf::Color(220,  50,  50), // Mythic
    sf::Color(255, 170,   0), // Legendary
};

} // namespace pegc

sf::Color PlinkoPegRarity::fillColor(OreRarity r) {
    const int i = std::clamp(static_cast<int>(r), 0,
        static_cast<int>(OreRarity::RARITY_COUNT) - 1);
    return pegc::kFill[i];
}

float PlinkoPegRarity::bonusMult(OreRarity r) {
    const int i = std::clamp(static_cast<int>(r), 0,
        static_cast<int>(OreRarity::RARITY_COUNT) - 1);
    return pegc::kBonus[i];
}

namespace {

std::string fmtPegCreditGain(double v) {
    std::ostringstream s;
    s << "+$";
    if (v >= 1e12) {
        s << std::fixed << std::setprecision(2) << v / 1e12 << "T";
        return s.str();
    }
    if (v >= 1e9) {
        s << std::fixed << std::setprecision(2) << v / 1e9 << "B";
        return s.str();
    }
    if (v >= 1e6) {
        s << std::fixed << std::setprecision(2) << v / 1e6 << "M";
        return s.str();
    }
    if (v >= 1e3) {
        s << std::fixed << std::setprecision(1) << v / 1e3 << "K";
        return s.str();
    }
    if (v >= 100.0)
        s << static_cast<long long>(std::llround(v));
    else
        s << std::fixed << std::setprecision(1) << v;
    return s.str();
}

OreRarity rollPegRarityTier() {
    static constexpr float w[5] = { 50.f, 30.f, 15.f, 4.f, 1.f };
    float                  total = 0.f;
    for (float x : w) total += x;
    const float r = randFloat(0.f, total);
    float       cum = 0.f;
    for (int i = 0; i < 5; i++) {
        cum += w[i];
        if (r < cum)
            return static_cast<OreRarity>(i + 1);
    }
    return OreRarity::LEGENDARY;
}

} // namespace

// ═════════════════════════════════════════════════════════════
//  Constructor
// ═════════════════════════════════════════════════════════════
PlinkoBoard::PlinkoBoard() {
    for (auto& b : m_balls) b.alive = false;
}

// ═════════════════════════════════════════════════════════════
//  build
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::build(int   rows,
                         float boardX, float boardY,
                         float boardW, float boardH,
                         float multBonus, float plinkoLuck,
                         float scale,
                         float pegHitRadius,
                         float pegBounceMult) {
    m_boardX = boardX;
    m_boardY = boardY;
    m_boardW = boardW;
    m_boardH = boardH;
    m_scale  = scale;

    const int newRows = clamp(rows, PLINKO_MIN_ROWS, PLINKO_MAX_ROWS);
    if (m_lastBuildRows >= 0 && newRows != m_lastBuildRows) {
        m_pegRarityByCell.clear();
        m_syncedGoldenPegChestLevel = -1;
    }
    m_rows = newRows;

    m_pegCreditPopups.clear();

    m_pegHitRadius  = std::max(3.f, pegHitRadius);
    m_pegDrawRadius = m_pegHitRadius;
    m_pegBounceMult = std::max(0.5f, pegBounceMult);

    buildPegs();
    buildSlots(multBonus, plinkoLuck);

    m_lastBuildRows = m_rows;
}

// ─────────────────────────────────────────────────────────────
//  buildPegs
// ─────────────────────────────────────────────────────────────
void PlinkoBoard::buildPegs() {
    m_pegs.clear();

    int   cols    = 32;                          // ← was 7
    float usableH = m_boardH - SLOT_HEIGHT - 20.f;
    float colStep = m_boardW / static_cast<float>(cols + 1);
    float rowStep = usableH  / static_cast<float>(m_rows + 1);

    for (int row = 0; row < m_rows; row++) {
        float y = m_boardY + rowStep * (row + 1);

        bool  oddRow    = (row % 2 == 1);
        int   pegsInRow = oddRow ? cols - 1 : cols;
        float offset    = oddRow ? colStep * 0.5f : 0.f;

        for (int col = 0; col < pegsInRow; col++) {
            Peg peg;
            peg.pos      = { m_boardX + offset + colStep * (col + 1), y };
            peg.hitFlash = 0.f;
            peg.cellRow  = row;
            peg.cellCol  = col;
            const auto key = std::make_pair(row, col);
            const auto it  = m_pegRarityByCell.find(key);
            peg.pegRarity    = (it != m_pegRarityByCell.end())
                                   ? it->second
                                   : OreRarity::COMMON;
            m_pegs.push_back(peg);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  slotMult
// ─────────────────────────────────────────────────────────────
float PlinkoBoard::slotMult(int slotIdx, int totalSlots,
                              float bonus, float luck) {
    float centre = (totalSlots - 1) * 0.5f;
    float dist   = std::abs(slotIdx - centre) / centre;
    float base   = 3.f * std::pow(1.f - dist, 2.2f) + 0.8f;
    base += luck * 3.f;
    return base * bonus;
}

// ─────────────────────────────────────────────────────────────
//  buildSlots
// ─────────────────────────────────────────────────────────────
void PlinkoBoard::buildSlots(float multBonus, float plinkoLuck) {
    m_slots.clear();

    constexpr int NUM_SLOTS = 13;
    float slotW = m_boardW / static_cast<float>(NUM_SLOTS);
    float slotY = m_boardY + m_boardH - SLOT_HEIGHT;

    for (int i = 0; i < NUM_SLOTS; i++) {
        PlinkoSlot slot;
        slot.pos   = sf::Vector2f(m_boardX + i * slotW, slotY);
        slot.width = slotW;
        slot.multiplier = slotMult(i, NUM_SLOTS, multBonus, plinkoLuck);

        // Kleur op basis van multiplier waarde
        float t = static_cast<float>(i) / (NUM_SLOTS - 1);
        float m = std::abs(t - 0.5f) * 2.f;  // 0 = midden, 1 = rand
        slot.color = sf::Color(
            static_cast<uint8_t>(50  + 200 * m),
            static_cast<uint8_t>(200 - 150 * m),
            static_cast<uint8_t>(100 + 100 * m));

        m_slots.push_back(slot);
    }
}

// ═════════════════════════════════════════════════════════════
//  claimBall
// ═════════════════════════════════════════════════════════════
PlinkoBall* PlinkoBoard::claimBall() {
    for (auto& b : m_balls)
        if (!b.alive) return &b;
    return nullptr;
}

int PlinkoBoard::ballsAlive() const {
    int n = 0;
    for (const auto& b : m_balls) if (b.alive) n++;
    return n;
}

// ═════════════════════════════════════════════════════════════
//  dropBall
// ═════════════════════════════════════════════════════════════
bool PlinkoBoard::dropBall(double oreValue, float dropX) {
    PlinkoBall* b = claimBall();
    if (!b) return false;

    float x = (dropX < 0.f)
               ? randFloat(m_boardX + 20.f,
                            m_boardX + m_boardW - 20.f)
               : dropX;

    b->pos      = { x, m_boardY + 5.f };
    b->vel      = { randFloat(-30.f, 30.f),
                    randFloat(DROP_SPEED_MIN, DROP_SPEED_MAX) };
    b->oreValue = oreValue;
    b->alive    = true;
    b->scored   = false;
    gSfx.play(Sfx::PlinkoDrop);
    return true;
}

// ═════════════════════════════════════════════════════════════
//  updateAuto
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::updateAuto(float dt, double& oreStock,
                               float autoInterval) {
    m_autoTimer -= dt;
    if (m_autoTimer > 0.f || oreStock < 1.0) return;

    m_autoTimer = autoInterval;

    oreStock -= 1.0;
    if (oreStock < 0.0) oreStock = 0.0;

    dropBall(1.0);
}

// ═════════════════════════════════════════════════════════════
//  resolvePegCollision
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::resolvePegCollision(PlinkoBall& ball,
                                       double&     creditsOut,
                                       float       creditMult) {
    for (auto& peg : m_pegs) {
        sf::Vector2f diff = ball.pos - peg.pos;
        float        dist = length(diff);
        float        minD = ball.radius + m_pegHitRadius;

        if (dist < minD && dist > 0.001f) {
            sf::Vector2f normal = diff / dist;
            ball.pos = peg.pos + normal * (minD + 0.5f);

            float vDotN = dot(ball.vel, normal);
            float rest  = 1.f + PEG_BOUNCE * m_pegBounceMult;
            ball.vel   -= normal * rest * vDotN;
            ball.vel.x += randFloat(-25.f, 25.f);

            peg.hitFlash = 0.12f;

            const int ri = static_cast<int>(peg.pegRarity);
            if (peg.pegRarity > OreRarity::COMMON && ri >= 0
                && ri < static_cast<int>(OreRarity::RARITY_COUNT)) {
                const float mult = pegc::kBonus[ri];
                const double gain =
                    static_cast<double>(ball.oreValue)
                    * static_cast<double>(mult)
                    * static_cast<double>(creditMult);
                creditsOut += gain;
                pushPegCreditPopup(peg.pos, gain);
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════
//  resolveWallCollision
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::resolveWallCollision(PlinkoBall& ball) {
    float left  = m_boardX + ball.radius;
    float right = m_boardX + m_boardW - ball.radius;

    if (ball.pos.x < left) {
        ball.pos.x = left;
        ball.vel.x = std::abs(ball.vel.x) * WALL_BOUNCE;
    }
    if (ball.pos.x > right) {
        ball.pos.x = right;
        ball.vel.x = -std::abs(ball.vel.x) * WALL_BOUNCE;
    }
}

// ─────────────────────────────────────────────────────────────
//  findSlot
// ─────────────────────────────────────────────────────────────
int PlinkoBoard::findSlot(float x) const {
    for (int i = 0; i < static_cast<int>(m_slots.size()); i++) {
        float left  = m_slots[i].pos.x;
        float right = left + m_slots[i].width;
        if (x >= left && x < right) return i;
    }
    return (x < m_boardX + m_boardW * 0.5f)
           ? 0
           : static_cast<int>(m_slots.size()) - 1;
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::update(float dt, double& creditsOut,
                          float creditMult, int bulkMult,
                          ParticleSystem& particles) {
    float slotTopY = m_boardY + m_boardH - SLOT_HEIGHT;

    for (auto& peg : m_pegs)
        if (peg.hitFlash > 0.f) peg.hitFlash -= dt;

    for (auto& s : m_slots)
        if (s.flashTimer > 0.f) s.flashTimer -= dt;

    for (auto& ball : m_balls) {
        if (!ball.alive) continue;

        // Physics
        ball.vel.y += PLINKO_GRAVITY * dt;
        ball.vel   *= BALL_FRICTION;
        ball.pos   += ball.vel * dt;

        resolvePegCollision(ball, creditsOut, creditMult);
        resolveWallCollision(ball);

        // Scoring
        if (!ball.scored && ball.pos.y >= slotTopY) {
            ball.scored  = true;
            int slotIdx  = findSlot(ball.pos.x);

            if (slotIdx >= 0 &&
                slotIdx < static_cast<int>(m_slots.size())) {
                auto& slot = m_slots[slotIdx];

                double earned = ball.oreValue
                    * static_cast<double>(slot.multiplier)
                    * static_cast<double>(creditMult)
                    * static_cast<double>(bulkMult);

                creditsOut      += earned;
                slot.flashTimer  = 0.6f;
                gSfx.play(Sfx::PlinkoScore);

                particles.emitExplosion(ball.pos, 18.f,
                                         slot.color, 12);
            }
        }

        if (ball.pos.y > m_boardY + m_boardH + 20.f)
            ball.alive = false;
    }

    updatePegCreditPopups(dt);
}

void PlinkoBoard::pushPegCreditPopup(sf::Vector2f pegCenter,
                                      double       credits) {
    if (credits <= 0.0)
        return;

    PegCreditPopup p;
    p.pos  = pegCenter
           + sf::Vector2f(0.f, -m_pegDrawRadius - std::round(6.f * m_scale));
    p.text = fmtPegCreditGain(credits);
    p.age  = 0.f;

    constexpr std::size_t kMaxPopups = 48;
    if (m_pegCreditPopups.size() >= kMaxPopups)
        m_pegCreditPopups.erase(m_pegCreditPopups.begin());

    m_pegCreditPopups.push_back(std::move(p));
}

void PlinkoBoard::updatePegCreditPopups(float dt) {
    constexpr float kLife = 0.85f;
    for (auto& p : m_pegCreditPopups) {
        p.age += dt;
        p.pos.y -= std::round(38.f * m_scale) * dt;
    }
    m_pegCreditPopups.erase(
        std::remove_if(m_pegCreditPopups.begin(), m_pegCreditPopups.end(),
                       [kLife](const PegCreditPopup& p) {
                           return p.age >= kLife;
                       }),
        m_pegCreditPopups.end());
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void PlinkoBoard::draw(sf::RenderTarget& target,
                        sf::Font&         font) const {
    // ── Board background ──────────────────────────────────
    sf::RectangleShape bg(sf::Vector2f{ m_boardW, m_boardH });
    bg.setPosition({ m_boardX, m_boardY });
    bg.setFillColor(sf::Color(10, 12, 25, 220));
    bg.setOutlineColor(sf::Color(60, 80, 140, 180));
    bg.setOutlineThickness(2.f);
    target.draw(bg);

    // ── Pegs ──────────────────────────────────────────────
    sf::CircleShape pegShape(m_pegDrawRadius);
    pegShape.setOrigin({ m_pegDrawRadius, m_pegDrawRadius });

    for (const auto& peg : m_pegs) {
        const int pr = std::clamp(static_cast<int>(peg.pegRarity), 0,
            static_cast<int>(OreRarity::RARITY_COUNT) - 1);
        sf::Color base = pegc::kFill[pr];
        const float ft =
            peg.hitFlash > 0.f ? std::min(1.f, peg.hitFlash / 0.12f) : 0.f;
        auto brighten = [](uint8_t v, float t) {
            return static_cast<uint8_t>(
                std::min(255, static_cast<int>(v + (255 - v) * t * 0.9f)));
        };
        pegShape.setPosition(peg.pos);
        pegShape.setFillColor(
            sf::Color(brighten(base.r, ft), brighten(base.g, ft),
                      brighten(base.b, ft)));
        pegShape.setOutlineColor(sf::Color(
            std::min(255, base.r + 40), std::min(255, base.g + 40),
            std::min(255, base.b + 50), 90));
        pegShape.setOutlineThickness(1.f);
        target.draw(pegShape);
    }

    // ── Bonus-peg credit floaters ─────────────────────────
    {
        constexpr float kLife = 0.85f;
        const unsigned  fs =
            static_cast<unsigned>(std::round(11.f * m_scale));
        for (const auto& p : m_pegCreditPopups) {
            const float   t  = clamp(p.age / kLife, 0.f, 1.f);
            const uint8_t a  = static_cast<uint8_t>(255.f * (1.f - t));
            const uint8_t aO = static_cast<uint8_t>(200.f * (1.f - t));

            sf::Text label(font);
            label.setString(p.text);
            label.setCharacterSize(fs);
            label.setStyle(sf::Text::Bold);
            label.setFillColor(sf::Color(255, 235, 130, a));
            label.setOutlineColor(sf::Color(50, 40, 20, aO));
            label.setOutlineThickness(1.f);

            const auto lb    = label.getLocalBounds();
            const float textW = lb.size.x;
            label.setPosition({
                p.pos.x - textW * 0.5f - lb.position.x,
                p.pos.y - lb.position.y });
            target.draw(label);
        }
    }

    // ── Slots ─────────────────────────────────────────────
    for (const auto& slot : m_slots) {
        float   flash = clamp(slot.flashTimer / 0.6f, 0.f, 1.f);
        uint8_t alpha = static_cast<uint8_t>(160 + 95 * flash);

        sf::RectangleShape bucket(
            sf::Vector2f{ slot.width - 2.f, SLOT_HEIGHT });
        bucket.setPosition({
            slot.pos.x + 1.f, slot.pos.y });
        bucket.setFillColor(sf::Color(
            static_cast<uint8_t>(
                slot.color.r * 0.4f + flash * slot.color.r * 0.6f),
            static_cast<uint8_t>(
                slot.color.g * 0.4f + flash * slot.color.g * 0.6f),
            static_cast<uint8_t>(
                slot.color.b * 0.4f + flash * slot.color.b * 0.6f),
            alpha));
        bucket.setOutlineColor(sf::Color(
            slot.color.r, slot.color.g, slot.color.b, 180));
        bucket.setOutlineThickness(1.f);
        target.draw(bucket);

        // Multiplier label
        // Multiplier label
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1)
        << slot.multiplier << "x";

        sf::Text label(font);
        label.setString(ss.str());
        label.setCharacterSize(
            static_cast<unsigned>(std::round(12.f * m_scale)));  // ← was 11
        label.setStyle(sf::Text::Bold);
        label.setFillColor(sf::Color(255, 255, 255,
            static_cast<uint8_t>(200 + 55 * flash)));

        float lw = label.getLocalBounds().size.x;
        float lh = label.getLocalBounds().size.y;
        label.setPosition({
            slot.pos.x + (slot.width - lw) * 0.5f,
            slot.pos.y + (SLOT_HEIGHT - lh) * 0.5f - 2.f });  // ← ook verticaal gecentreerd
        target.draw(label);
    }

    // ── Balls ─────────────────────────────────────────────
    sf::CircleShape ballShape;

    for (const auto& ball : m_balls) {
        if (!ball.alive) continue;

        // Glow
        float glowR = ball.radius + 5.f;
        ballShape.setRadius(glowR);
        ballShape.setOrigin({ glowR, glowR });
        ballShape.setPosition(ball.pos);
        ballShape.setFillColor(sf::Color(180, 120, 255, 50));
        target.draw(ballShape);

        // Core
        ballShape.setRadius(ball.radius);
        ballShape.setOrigin({ ball.radius, ball.radius });
        ballShape.setPosition(ball.pos);
        ballShape.setFillColor(sf::Color(200, 150, 255));
        ballShape.setOutlineColor(sf::Color(255, 220, 255, 160));
        ballShape.setOutlineThickness(1.5f);
        target.draw(ballShape);

        // Specular
        float specR = ball.radius * 0.35f;
        ballShape.setRadius(specR);
        ballShape.setOrigin({ specR, specR });
        ballShape.setPosition({
            ball.pos.x - ball.radius * 0.28f,
            ball.pos.y - ball.radius * 0.28f });
        ballShape.setFillColor(sf::Color(255, 255, 255, 180));
        ballShape.setOutlineThickness(0.f);
        target.draw(ballShape);
    }
}

// ═════════════════════════════════════════════════════════════
//  Peg rarity (Golden Pegs chest) — persistent per (rij, kolom)
// ═════════════════════════════════════════════════════════════

void PlinkoBoard::resetGoldenPegRarityState() {
    m_pegRarityByCell.clear();
    m_syncedGoldenPegChestLevel = -1;
    m_lastBuildRows             = -1;
    m_pegCreditPopups.clear();
    for (auto& p : m_pegs)
        p.pegRarity = OreRarity::COMMON;
}

void PlinkoBoard::applyPegRarityRolls(int rollCount) {
    if (m_pegs.empty() || rollCount <= 0)
        return;

    const int nPegs = static_cast<int>(m_pegs.size());
    for (int i = 0; i < rollCount; ++i) {
        Peg& peg = m_pegs[randInt(0, nPegs - 1)];
        if (peg.cellRow < 0)
            continue;

        const OreRarity rolled = rollPegRarityTier();
        const int       cur    = static_cast<int>(peg.pegRarity);
        const int       rv     = static_cast<int>(rolled);
        if (rv > cur)
            peg.pegRarity = rolled;

        m_pegRarityByCell[{ peg.cellRow, peg.cellCol }] = peg.pegRarity;
    }
}

void PlinkoBoard::syncGoldenPegChestRarities(int goldenPegChestLevel) {
    if (goldenPegChestLevel < 0)
        goldenPegChestLevel = 0;
    const int targetRolls = goldenPegChestLevel * 3;

    if (m_pegs.empty()) {
        m_syncedGoldenPegChestLevel = goldenPegChestLevel;
        return;
    }

    if (m_syncedGoldenPegChestLevel < 0) {
        applyPegRarityRolls(targetRolls);
        m_syncedGoldenPegChestLevel = goldenPegChestLevel;
        return;
    }

    if (goldenPegChestLevel > m_syncedGoldenPegChestLevel) {
        const int d = goldenPegChestLevel - m_syncedGoldenPegChestLevel;
        applyPegRarityRolls(d * 3);
        m_syncedGoldenPegChestLevel = goldenPegChestLevel;
    } else if (goldenPegChestLevel < m_syncedGoldenPegChestLevel) {
        m_pegRarityByCell.clear();
        for (auto& p : m_pegs)
            p.pegRarity = OreRarity::COMMON;
        m_syncedGoldenPegChestLevel = -1;
        applyPegRarityRolls(targetRolls);
        m_syncedGoldenPegChestLevel = goldenPegChestLevel;
    }
}
