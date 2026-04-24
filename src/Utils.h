#pragma once
#include <cmath>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <SFML/System/Vector2.hpp>

// ─────────────────────────────────────────────────────────────
//  Random number generator  (single global instance)
// ─────────────────────────────────────────────────────────────
inline std::mt19937& rng() {
    static std::mt19937 gen{ std::random_device{}() };
    return gen;
}

/// Integer in [lo, hi]
inline int randInt(int lo, int hi) {
    return std::uniform_int_distribution<int>{ lo, hi }(rng());
}

/// Float in [lo, hi)
inline float randFloat(float lo, float hi) {
    return std::uniform_real_distribution<float>{ lo, hi }(rng());
}

/// True with probability p  (0.0 – 1.0)
inline bool chance(float p) {
    return randFloat(0.f, 1.f) < p;
}

// ─────────────────────────────────────────────────────────────
//  Math helpers
// ─────────────────────────────────────────────────────────────
inline float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline sf::Vector2f normalize(sf::Vector2f v) {
    float len = length(v);
    if (len < 1e-6f) return { 0.f, 0.f };
    return { v.x / len, v.y / len };
}

inline float dot(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}

inline float distanceSq(sf::Vector2f a, sf::Vector2f b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

inline float distance(sf::Vector2f a, sf::Vector2f b) {
    return std::sqrt(distanceSq(a, b));
}

/// Clamp a value between lo and hi
template<typename T>
inline T clamp(T val, T lo, T hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}

/// Linear interpolation
inline float lerp(float a, float b, float t) {
    return a + (b - a) * clamp(t, 0.f, 1.f);
}

// ─────────────────────────────────────────────────────────────
//  Number formatting helpers
// ─────────────────────────────────────────────────────────────

/// "1.23M", "456K", "789" etc.
inline std::string formatBig(double val) {
    const double T  = 1e12, B = 1e9, M = 1e6, K = 1e3;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    if      (val >= T) { ss << val / T << "T"; }
    else if (val >= B) { ss << val / B << "B"; }
    else if (val >= M) { ss << val / M << "M"; }
    else if (val >= K) { ss << val / K << "K"; }
    else               { ss << std::setprecision(0) << val; }
    return ss.str();
}

/// Price tag:  "123" or "1.50K" etc.
inline std::string formatCost(double cost) {
    return formatBig(cost);
}

/// Percentage with one decimal:  "12.5%"
inline std::string pct(float val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << val * 100.f << "%";
    return ss.str();
}

// ─────────────────────────────────────────────────────────────
//  Upgrade cost scaling  (base * multiplier ^ level)
// ─────────────────────────────────────────────────────────────
inline double upgradeCost(double base, double mult, int level) {
    return base * std::pow(mult, static_cast<double>(level));
}

// ─────────────────────────────────────────────────────────────
//  Angle helpers (degrees ↔ radians)
// ─────────────────────────────────────────────────────────────
constexpr float PI = 3.14159265358979f;

inline float toRad(float deg) { return deg * PI / 180.f; }
inline float toDeg(float rad) { return rad * 180.f / PI; }

/// Angle (radians) from point a toward point b
inline float angleTo(sf::Vector2f a, sf::Vector2f b) {
    return std::atan2(b.y - a.y, b.x - a.x);
}