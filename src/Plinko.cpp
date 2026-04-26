#include "Plinko.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

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
                         float scale) {        // ← nieuw
    m_boardX = boardX;
    m_boardY = boardY;
    m_boardW = boardW;
    m_boardH = boardH;
    m_scale  = scale;  // ← opslaan
    m_rows   = clamp(rows, PLINKO_MIN_ROWS, PLINKO_MAX_ROWS);

    buildPegs();
    buildSlots(multBonus, plinkoLuck);
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
void PlinkoBoard::resolvePegCollision(PlinkoBall& ball) {
    for (auto& peg : m_pegs) {
        sf::Vector2f diff = ball.pos - peg.pos;
        float        dist = length(diff);
        float        minD = ball.radius + PLINKO_PEG_RADIUS;

        if (dist < minD && dist > 0.001f) {
            sf::Vector2f normal = diff / dist;
            ball.pos = peg.pos + normal * (minD + 0.5f);

            float vDotN = dot(ball.vel, normal);
            ball.vel   -= normal * (1.f + PEG_BOUNCE) * vDotN;
            ball.vel.x += randFloat(-25.f, 25.f);

            peg.hitFlash = 0.12f;
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

        resolvePegCollision(ball);
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

                particles.emitExplosion(ball.pos, 18.f,
                                         slot.color, 12);
            }
        }

        if (ball.pos.y > m_boardY + m_boardH + 20.f)
            ball.alive = false;
    }
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
    sf::CircleShape pegShape(PLINKO_PEG_RADIUS);
    pegShape.setOrigin({ PLINKO_PEG_RADIUS, PLINKO_PEG_RADIUS });

    for (const auto& peg : m_pegs) {
        bool    flash = peg.hitFlash > 0.f;
        uint8_t r     = flash ? 255 : 140;
        uint8_t g     = flash ? 255 : 160;
        uint8_t b     = flash ? 255 : 200;

        pegShape.setPosition(peg.pos);
        pegShape.setFillColor(sf::Color(r, g, b));
        pegShape.setOutlineColor(sf::Color(200, 220, 255, 80));
        pegShape.setOutlineThickness(1.f);
        target.draw(pegShape);
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
