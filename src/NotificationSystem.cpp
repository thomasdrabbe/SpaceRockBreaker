#include "NotificationSystem.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace {

constexpr float kVisualScale = 2.f;   // t.o.v. oude toast (540×52, font 26)

} // namespace

void NotificationSystem::push(const std::string& message,
                              sf::Color              color,
                              float                  duration,
                              int                    badgeTab) {
    Entry e;
    e.message  = message;
    e.color    = color;
    e.holdSec  = std::max(0.1f, duration);
    e.badgeTab = badgeTab;
    e.elapsed  = 0.f;
    m_queue.push_back(std::move(e));
    while (static_cast<int>(m_queue.size()) > kMaxVisible)
        m_queue.pop_front();
    if (badgeTab >= 0 && badgeTab < TAB_COUNT)
        m_tabBadges[static_cast<std::size_t>(badgeTab)] = true;
}

void NotificationSystem::update(float dt) {
    const float maxStep = 0.1f;
    dt = std::min(dt, maxStep);
    for (Entry& e : m_queue)
        e.elapsed += dt;
    while (!m_queue.empty() && m_queue.front().elapsed >= m_queue.front().totalDur())
        m_queue.pop_front();
}

bool NotificationSystem::hasBadgeFor(int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= TAB_COUNT)
        return false;
    return m_tabBadges[static_cast<std::size_t>(tabIndex)];
}

void NotificationSystem::clearBadge(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= TAB_COUNT)
        return;
    m_tabBadges[static_cast<std::size_t>(tabIndex)] = false;
}

void NotificationSystem::draw(sf::RenderTarget& target,
                              const sf::Font&     font,
                              float               topInset) const {
    const auto px    = target.getSize();
    const float scrW = px.x > 0 ? static_cast<float>(px.x) : 1.f;
    const float scrH = px.y > 0 ? static_cast<float>(px.y) : 1.f;
    const float uiScale =
        std::min(scrW / 1920.f, scrH / 1080.f) * kVisualScale;

    const unsigned textSize =
        static_cast<unsigned>(std::round(26.f * uiScale));
    const float notifW =
        std::min(std::round(540.f * uiScale), std::max(120.f, scrW - 24.f));
    const float barH   = std::max(8.f, std::round(10.f * uiScale));
    const float mainH  = std::round(52.f * uiScale);
    const float notifH = mainH + barH;
    const float gap    = std::round(10.f * uiScale);
    const float padX   = std::round(18.f * uiScale);

    const float x = std::max(12.f, (scrW - notifW) * 0.5f);
    float       y = std::max(4.f, topInset);

    int slot = 0;
    for (int i = static_cast<int>(m_queue.size()) - 1; i >= 0; --i) {
        const Entry& e = m_queue[static_cast<std::size_t>(i)];
        const float fi  = kFadeInSec;
        const float fo  = kFadeOutSec;
        const float tot = e.totalDur();
        float       a   = 1.f;
        if (e.elapsed < fi)
            a = std::clamp(e.elapsed / fi, 0.f, 1.f);
        else if (e.elapsed >= fi + e.holdSec)
            a = std::clamp((tot - e.elapsed) / fo, 0.f, 1.f);
        if (a <= 0.01f)
            continue;

        const float yy = y + static_cast<float>(slot) * (notifH + gap);

        const std::uint8_t fillAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 180.f);
        const std::uint8_t outlineAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 160.f);
        const std::uint8_t textAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 255.f);

        sf::RectangleShape bg(sf::Vector2f{ notifW, mainH });
        bg.setPosition({ x, yy });
        bg.setFillColor(sf::Color(12, 14, 28, fillAlpha));
        bg.setOutlineColor(sf::Color(
            e.color.r, e.color.g, e.color.b, outlineAlpha));
        bg.setOutlineThickness(2.f);
        target.draw(bg);

        sf::RectangleShape track(sf::Vector2f{ notifW, barH });
        track.setPosition({ x, yy + mainH });
        track.setFillColor(sf::Color(18, 22, 40,
            static_cast<std::uint8_t>(fillAlpha * 0.95f)));
        track.setOutlineColor(sf::Color(
            e.color.r, e.color.g, e.color.b,
            static_cast<std::uint8_t>(outlineAlpha * 0.55f)));
        track.setOutlineThickness(1.f);
        target.draw(track);

        const float remain = std::max(0.f, tot - e.elapsed);
        const float tRatio = tot > 1e-4f ? std::clamp(remain / tot, 0.f, 1.f) : 0.f;
        const float innerPad = 3.f;
        const float fillW =
            std::max(0.f, (notifW - 2.f * innerPad) * tRatio);
        if (fillW > 0.5f) {
            sf::RectangleShape fill(sf::Vector2f{
                fillW, std::max(2.f, barH - 2.f * innerPad) });
            fill.setPosition({ x + innerPad, yy + mainH + innerPad });
            fill.setFillColor(sf::Color(
                e.color.r, e.color.g, e.color.b,
                static_cast<std::uint8_t>(
                    static_cast<float>(textAlpha) * 0.85f)));
            target.draw(fill);
        }

        sf::Text txt(font);
        txt.setCharacterSize(textSize);
        txt.setString(e.message);
        txt.setStyle(sf::Text::Bold);
        txt.setFillColor(sf::Color(
            e.color.r, e.color.g, e.color.b, textAlpha));
        const sf::FloatRect lb = txt.getLocalBounds();
        txt.setOrigin({ std::round(lb.position.x),
                        std::round(lb.position.y + lb.size.y * 0.5f) });
        txt.setPosition({ std::round(x + padX), std::round(yy + mainH * 0.5f) });
        target.draw(txt);

        ++slot;
    }
}
