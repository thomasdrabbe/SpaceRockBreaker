#include "NotificationSystem.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

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
                              float               insetRight,
                              float               insetTop) const {
    (void)insetTop;
    const auto px   = target.getSize();
    const float scrW = px.x > 0 ? static_cast<float>(px.x) : 1.f;
    const float scrH = px.y > 0 ? static_cast<float>(px.y) : 1.f;
    const float uiScale = std::min(scrW / 1920.f, scrH / 1080.f);
    const unsigned textSize =
        static_cast<unsigned>(std::round(26.f * uiScale));
    const float notifW  = std::round(540.f * uiScale);
    const float notifH  = std::round(52.f * uiScale);
    const float gap     = std::round(10.f * uiScale);
    const float padX    = std::round(18.f * uiScale);

    const float baseX = std::max(0.f, scrW - insetRight - notifW - 10.f);
    const float baseY = std::max(0.f, scrH - std::round(58.f * uiScale));

    int slotFromBottom = 0;
    for (int i = static_cast<int>(m_queue.size()) - 1; i >= 0; --i) {
        const Entry& e = m_queue[static_cast<std::size_t>(i)];
        const float fi  = kFadeInSec;
        const float fo  = kFadeOutSec;
        const float tot = fi + e.holdSec + fo;
        float       a   = 1.f;
        if (e.elapsed < fi)
            a = std::clamp(e.elapsed / fi, 0.f, 1.f);
        else if (e.elapsed >= fi + e.holdSec)
            a = std::clamp((tot - e.elapsed) / fo, 0.f, 1.f);
        if (a <= 0.01f)
            continue;

        const float x = baseX;
        const float y = baseY - static_cast<float>(slotFromBottom)
                              * (notifH + gap);

        const std::uint8_t fillAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 180.f);
        const std::uint8_t outlineAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 160.f);

        sf::RectangleShape bg(sf::Vector2f{ notifW, notifH });
        bg.setPosition({ x, y });
        bg.setFillColor(sf::Color(12, 14, 28, fillAlpha));
        bg.setOutlineColor(sf::Color(
            e.color.r, e.color.g, e.color.b, outlineAlpha));
        bg.setOutlineThickness(2.f);
        target.draw(bg);

        sf::Text txt(font);
        txt.setCharacterSize(textSize);
        txt.setString(e.message);
        txt.setStyle(sf::Text::Bold);
        txt.setFillColor(sf::Color(
            e.color.r, e.color.g, e.color.b,
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 255.f)));
        txt.setPosition({
            x + padX,
            y + notifH * 0.5f - static_cast<float>(textSize) * 0.5f });
        target.draw(txt);

        slotFromBottom++;
    }
}
