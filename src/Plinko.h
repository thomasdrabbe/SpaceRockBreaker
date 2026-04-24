#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include "Constants.h"
#include "GameState.h"
#include "Particle.h"

// ─────────────────────────────────────────────────────────────
//  Plinko peg
// ─────────────────────────────────────────────────────────────
struct Peg {
    sf::Vector2f pos;
    float        hitFlash = 0.f;   // seconds of white flash remaining
};

// ─────────────────────────────────────────────────────────────
//  Plinko slot  (bottom bucket)
// ─────────────────────────────────────────────────────────────
struct PlinkoSlot {
    sf::Vector2f pos;       // centre-top of bucket
    float        width;
    float        multiplier;
    float        flashTimer = 0.f;   // lights up when ball lands
    sf::Color    color;
};

// ─────────────────────────────────────────────────────────────
//  Plinko ball  (physics object)
// ─────────────────────────────────────────────────────────────
struct PlinkoBall {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float        radius   = PLINKO_BALL_RADIUS;
    bool         alive    = false;
    bool         scored   = false;   // already awarded credits
    double       oreValue = 0.0;     // credits this ball carries
};

// ─────────────────────────────────────────────────────────────
//  PlinkoBoard  — self-contained Plinko mini-game
// ─────────────────────────────────────────────────────────────
class PlinkoBoard {
public:
    PlinkoBoard();

    /// Call once / when rows or board size changes
    void build(int          rows,
               float        boardX,   // top-left of board area
               float        boardY,
               float        boardW,
               float        boardH,
               float        multBonus,
               float        plinkoLuck);

    /// Drop one ball carrying oreValue credits
    /// Returns false if ball pool is full
    bool dropBall(double oreValue, float dropX = -1.f);

    /// Auto-drop if autoPlinko is enabled
    void updateAuto(float dt, double& oreStock,
                    float autoInterval);

    /// Physics step + scoring
    /// creditsOut : accumulated credits awarded this frame
    void update(float dt, double& creditsOut,
                float creditMult, int bulkMult,
                ParticleSystem& particles);

    void draw(sf::RenderTarget& target,
              sf::Font&         font) const;

    // Accessors
    int   ballsAlive()  const;
    int   totalBalls()  const { return MAX_PLINKO_BALLS; }
    float boardLeft()   const { return m_boardX; }
    float boardTop()    const { return m_boardY; }
    float boardWidth()  const { return m_boardW; }
    float boardHeight() const { return m_boardH; }

private:
    // ── Board geometry ────────────────────────────────────
    float m_boardX = 0.f, m_boardY = 0.f;
    float m_boardW = 0.f, m_boardH = 0.f;
    int   m_rows   = PLINKO_MIN_ROWS;

    // ── Pegs & slots ──────────────────────────────────────
    std::vector<Peg>       m_pegs;
    std::vector<PlinkoSlot> m_slots;

    // ── Ball pool ─────────────────────────────────────────
    std::array<PlinkoBall, MAX_PLINKO_BALLS> m_balls;

    // ── Auto-drop timer ───────────────────────────────────
    float m_autoTimer = 0.f;

    // ── Helpers ───────────────────────────────────────────
    void  buildPegs();
    void  buildSlots(float multBonus, float plinkoLuck);
    void  resolvePegCollision(PlinkoBall& ball);
    void  resolveWallCollision(PlinkoBall& ball);
    int   findSlot(float x) const;

    PlinkoBall* claimBall();

    // Slot multiplier table (centre → edges = high → low, with luck skew)
    static float slotMult(int slotIdx, int totalSlots,
                          float bonus, float luck);

    static constexpr float WALL_BOUNCE    = 0.55f;
    static constexpr float PEG_BOUNCE     = 0.45f;
    static constexpr float BALL_FRICTION  = 0.992f;
    static constexpr float DROP_SPEED_MIN = 80.f;
    static constexpr float DROP_SPEED_MAX = 140.f;
    static constexpr float SLOT_HEIGHT    = 40.f;
    static constexpr float AUTO_INTERVAL  = 1.2f;   // seconds between auto-drops
};