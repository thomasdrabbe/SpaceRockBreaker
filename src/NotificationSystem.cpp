#include "NotificationSystem.h"
#include <algorithm>
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
    while (!m_queue.empty()) {
        Entry& front = m_queue.front();
        front.elapsed += dt;
        if (front.elapsed >= front.totalDur())
            m_queue.pop_front();
        else
            break;
    }
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

namespace {

void drawPill(sf::RenderTarget& target,
              sf::Vector2f      pos,
              sf::Vector2f      size,
              sf::Color         fill,
              sf::Color         outline,
              float             outlineThick) {
    const float h = size.y;
    const float r = h * 0.5f;
    if (size.x < h) {
        sf::CircleShape c(r);
        c.setOrigin({ r, r });
        c.setPosition({ pos.x + size.x * 0.5f, pos.y + size.y * 0.5f });
        c.setFillColor(fill);
        c.setOutlineColor(outline);
        c.setOutlineThickness(outlineThick);
        target.draw(c);
        return;
    }
    sf::RectangleShape mid(sf::Vector2f{ size.x - h, h });
    mid.setPosition({ pos.x + r, pos.y });
    mid.setFillColor(fill);
    mid.setOutlineColor(sf::Color::Transparent);
    target.draw(mid);

    sf::CircleShape left(r);
    left.setOrigin({ r, r });
    left.setPosition({ pos.x + r, pos.y + r });
    left.setFillColor(fill);
    left.setOutlineColor(outline);
    left.setOutlineThickness(outlineThick);
    target.draw(left);

    sf::CircleShape right(r);
    right.setOrigin({ r, r });
    right.setPosition({ pos.x + size.x - r, pos.y + r });
    right.setFillColor(fill);
    right.setOutlineColor(outline);
    right.setOutlineThickness(outlineThick);
    target.draw(right);
}

} // namespace

void NotificationSystem::draw(sf::RenderTarget& target,
                              const sf::Font&     font,
                              float               insetRight,
                              float               insetTop) const {
    const auto px = target.getSize();
    const float scrW = px.x > 0 ? static_cast<float>(px.x) : 1.f;
    const float scrH = px.y > 0 ? static_cast<float>(px.y) : 1.f;

    const float marginH =
        std::clamp(scrW * 0.012f, 16.f, 34.f);
    const float marginTop =
        insetTop + std::clamp(scrH * 0.011f, 12.f, 22.f);
    const float contentW  = std::max(120.f, scrW - insetRight);
    const float pillH =
        std::clamp(54.f * (scrH / 1080.f), 46.f, 72.f);
    const float stackGap = std::clamp(12.f * (scrH / 1080.f), 10.f, 18.f);
    const unsigned charSize =
        static_cast<unsigned>(std::clamp(22.f * (scrH / 1080.f), 18.f, 28.f));

    int slot = 0;
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

        sf::Text measure(font);
        measure.setCharacterSize(charSize);
        measure.setStyle(sf::Text::Bold);
        measure.setString(e.message);
        const sf::FloatRect lb = measure.getLocalBounds();
        const float          textW = lb.size.x;
        const float          padX  =
            std::clamp(52.f * (scrW / 1920.f), 42.f, 72.f);
        const float maxPill =
            std::max(120.f, contentW - marginH * 2.f);
        const float pillW = std::min(maxPill, textW + padX);
        const float x     =
            marginH + std::max(0.f, (contentW - marginH * 2.f - pillW) * 0.5f);
        const float y =
            marginTop
            + static_cast<float>(slot) * (pillH + stackGap);

        const std::uint8_t fillA =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 220.f);
        const std::uint8_t outA =
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 200.f);
        const sf::Color fillCol(18, 20, 38, fillA);
        const sf::Color outCol(
            static_cast<std::uint8_t>(std::min(255, e.color.r + 40)),
            static_cast<std::uint8_t>(std::min(255, e.color.g + 40)),
            static_cast<std::uint8_t>(std::min(255, e.color.b + 40)),
            outA);

        drawPill(target, { x, y }, { pillW, pillH }, fillCol, outCol, 2.f);

        sf::Text txt(font);
        txt.setCharacterSize(charSize);
        txt.setString(e.message);
        txt.setStyle(sf::Text::Bold);
        txt.setFillColor(sf::Color(
            e.color.r, e.color.g, e.color.b,
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 255.f)));
        const float lbShiftX = lb.position.x;
        const float lbShiftY = lb.position.y;
        txt.setPosition({
            x + (pillW - textW) * 0.5f - lbShiftX,
            y + pillH * 0.5f - (lbShiftY + lb.size.y * 0.5f) });
        target.draw(txt);
        slot++;
    }
}
