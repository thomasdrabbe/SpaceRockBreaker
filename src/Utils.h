#pragma once
#include <cmath>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>


// ─────────────────────────────────────────────────────────────
//  Random number generator
// ─────────────────────────────────────────────────────────────
inline std::mt19937& rng() {
    static std::mt19937 gen{ std::random_device{}() };
    return gen;
}

inline int randInt(int lo, int hi) {
    return std::uniform_int_distribution<int>{ lo, hi }(rng());
}

inline float randFloat(float lo, float hi) {
    return std::uniform_real_distribution<float>{ lo, hi }(rng());
}

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

template<typename T>
inline T clamp(T val, T lo, T hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * clamp(t, 0.f, 1.f);
}

// ─────────────────────────────────────────────────────────────
//  Number formatting
// ─────────────────────────────────────────────────────────────
inline std::string formatBig(double val) {
    const double T = 1e12, B = 1e9, M = 1e6, K = 1e3;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    if      (val >= T) { ss << val / T << "T"; }
    else if (val >= B) { ss << val / B << "B"; }
    else if (val >= M) { ss << val / M << "M"; }
    else if (val >= K) { ss << val / K << "K"; }
    else               { ss << std::setprecision(0) << val; }
    return ss.str();
}

inline std::string formatCost(double cost) {
    return formatBig(cost);
}

inline std::string pct(float val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << val * 100.f << "%";
    return ss.str();
}

// ─────────────────────────────────────────────────────────────
//  Upgrade cost scaling
// ─────────────────────────────────────────────────────────────
inline double upgradeCost(double base, double mult, int level) {
    return base * std::pow(mult, static_cast<double>(level));
}

// ─────────────────────────────────────────────────────────────
//  Angle helpers
// ─────────────────────────────────────────────────────────────
constexpr float PI = 3.14159265358979f;

inline float toRad(float deg) { return deg * PI / 180.f; }
inline float toDeg(float rad) { return rad * 180.f / PI; }

inline float angleTo(sf::Vector2f a, sf::Vector2f b) {
    return std::atan2(b.y - a.y, b.x - a.x);
}

// ─────────────────────────────────────────────────────────────
//  SFML 3 helpers  — vervangt veelgebruikte SFML 2 patronen
// ─────────────────────────────────────────────────────────────

/// Maak een sf::FloatRect aan (SFML 3 gebruikt {pos, size})
inline sf::FloatRect makeRect(float x, float y, float w, float h) {
    return sf::FloatRect({ x, y }, { w, h });
}

/// rect.left  → rect.position.x  (shortcuts voor leesbaarheid)
inline float rectLeft  (const sf::FloatRect& r) { return r.position.x; }
inline float rectTop   (const sf::FloatRect& r) { return r.position.y; }
inline float rectWidth (const sf::FloatRect& r) { return r.size.x; }
inline float rectHeight(const sf::FloatRect& r) { return r.size.y; }

/// Zet muispixels om naar dezelfde 2D-coördinaten als drawing (default UI-view),
/// onafhankelijk van een resterende custom View op het venster (bv. mining-tab).
inline sf::Vector2f mapPixelToUi(const sf::RenderWindow& w, sf::Vector2i px) {
    const sf::Vector2u sz = w.getSize();
    const float        fw = sz.x > 0 ? static_cast<float>(sz.x) : 1.f;
    const float        fh = sz.y > 0 ? static_cast<float>(sz.y) : 1.f;
    const sf::View     ui(sf::FloatRect({ 0.f, 0.f }, { fw, fh }));
    return w.mapPixelToCoords(px, ui);
}

// ─────────────────────────────────────────────────────────────
//  Asset paden — altijd eerst naast de .exe (Release/Debug),
//  daarna cwd / ../ (ontwikkelen vanuit IDE).
// ─────────────────────────────────────────────────────────────
inline std::string applicationDirectory() {
#if defined(_WIN32)
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return {};
    std::string s(buf, buf + n);
    const auto slash = s.find_last_of("\\/");
    if (slash == std::string::npos)
        return {};
    s.resize(slash);
    return s;
#else
    (void)0;
    return {};
#endif
}

inline std::string resolveAssetPath(const std::string& rel) {
    namespace fs = std::filesystem;
    auto isFile = [](const std::string& p) {
        std::error_code ec;
        return fs::is_regular_file(fs::path(p), ec);
    };
    if (isFile(rel))
        return rel;
    if (isFile("../" + rel))
        return "../" + rel;
    if (isFile("../../" + rel))
        return "../../" + rel;
    const std::string app = applicationDirectory();
    if (!app.empty()) {
        const std::string p1 = app + "/" + rel;
        if (isFile(p1))
            return p1;
        const std::string p2 = app + "/../" + rel;
        if (isFile(p2))
            return p2;
    }
    return rel;
}
