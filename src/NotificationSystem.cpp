#include "NotificationSystem.h"
#include <algorithm>
#include <cstdint>

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

    constexpr float kW = 400.f;
    constexpr float kH = 52.f;
    constexpr float kPad = 16.f;
    constexpr float kGap = 8.f;
    constexpr float kBarW = 6.f;
    constexpr float kTextOffset = 20.f;
    constexpr float kTimerH = 2.f;
    constexpr float kTimerMaxW = 394.f;
    constexpr unsigned kTextSize = 14u;

    const float baseX = std::max(0.f, scrW - kPad - insetRight - kW);
    const float baseY = std::max(0.f, scrH - kPad - kH);

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
        const float y = baseY - static_cast<float>(slotFromBottom) * (kH + kGap);

        const std::uint8_t baseAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 220.f);
        const std::uint8_t accentAlpha =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 180.f);

        sf::RectangleShape bg(sf::Vector2f{ kW, kH });
        bg.setPosition({ x, y });
        bg.setFillColor(sf::Color(26, 28, 46, baseAlpha));
        target.draw(bg);

        sf::RectangleShape bar(sf::Vector2f{ kBarW, kH });
        bar.setPosition({ x, y });
        bar.setFillColor(sf::Color(e.color.r, e.color.g, e.color.b, baseAlpha));
        target.draw(bar);

        sf::Text txt(font);
        txt.setCharacterSize(kTextSize);
        txt.setString(e.message);
        txt.setStyle(sf::Text::Bold);
        txt.setFillColor(sf::Color(
            255, 255, 255,
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 255.f)));
        const sf::FloatRect lb = txt.getLocalBounds();
        txt.setPosition({
            x + kBarW + kTextOffset - lb.position.x,
            y + (kH * 0.5f) - (lb.position.y + lb.size.y * 0.5f) });
        target.draw(txt);

        const float visibleTime =
            std::clamp(e.elapsed - fi, 0.f, e.holdSec);
        const float timerRatio = e.holdSec > 0.0001f
            ? std::clamp((e.holdSec - visibleTime) / e.holdSec, 0.f, 1.f)
            : 0.f;
        sf::RectangleShape tbar(sf::Vector2f{ kTimerMaxW * timerRatio, kTimerH });
        tbar.setPosition({ x + kBarW, y + kH - kTimerH });
        tbar.setFillColor(sf::Color(
            e.color.r, e.color.g, e.color.b, accentAlpha));
        target.draw(tbar);

        slotFromBottom++;
    }
}
