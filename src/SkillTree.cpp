#include "SkillTree.h"
#include "GameState.h"
#include "Utils.h"
#include <algorithm>
#include <sstream>

const std::vector<UpgradeNodeDef> SKILL_TREE_NODES = {
    { UpgradeID::GUN_DAMAGE,    0, 0 },
    { UpgradeID::ORE_VALUE,     0, 2 },
    { UpgradeID::PLINKO_BALLS,  0, 5 },

    { UpgradeID::FIRE_RATE,     1, 0, UpgradeID::GUN_DAMAGE,    1 },
    { UpgradeID::SPLIT_SHOT,    2, 0, UpgradeID::FIRE_RATE,     3 },
    { UpgradeID::CRIT_CHANCE,   3, 0, UpgradeID::SPLIT_SHOT,    1 },
    { UpgradeID::CRIT_MULT,     4, 0, UpgradeID::CRIT_CHANCE,   3 },
    // Targeting (spec kolom 4–5 rij 1; rij 0 = geen overlap met turret-nodes)
    { UpgradeID::TARGET_PRIORITY, 4, 0, UpgradeID::TURRET_COUNT,  2 },
    { UpgradeID::SEEKING_BULLETS, 5, 0, UpgradeID::TARGET_PRIORITY, 2 },
    { UpgradeID::TURRET_COUNT,  3, 1, UpgradeID::SPLIT_SHOT,    1 },
    { UpgradeID::TURRET_DAMAGE, 4, 1, UpgradeID::TURRET_COUNT,  1 },
    { UpgradeID::TURRET_FIRE_RATE, 5, 1, UpgradeID::TURRET_DAMAGE, 2 },
    { UpgradeID::TURRET_FUEL_DRAIN, 6, 1, UpgradeID::TURRET_FIRE_RATE, 2 },
    { UpgradeID::SATELLITE,       7, 1, UpgradeID::TURRET_FUEL_DRAIN, 2 },
    { UpgradeID::BULLET_RANGE,  4, 2, UpgradeID::TURRET_COUNT,  3 },

    { UpgradeID::AUTO_COLLECT_RADIUS, 1, 2, UpgradeID::ORE_VALUE, 1 },
    { UpgradeID::ORE_LUCK,      2, 2, UpgradeID::AUTO_COLLECT_RADIUS, 3 },
    { UpgradeID::ORE_ON_KILL,   3, 2, UpgradeID::ORE_LUCK,      2 },
    { UpgradeID::ASTEROID_HP,   2, 3, UpgradeID::ORE_VALUE,     3 },
    { UpgradeID::EXPLOSIVE_ASTEROIDS, 5, 6, UpgradeID::ASTEROID_HP, 3 },
    { UpgradeID::CHAIN_REACTION, 6, 6, UpgradeID::EXPLOSIVE_ASTEROIDS, 3 },
    { UpgradeID::WARP_DRIVE,    1, 3, UpgradeID::ORE_VALUE,     1 },
    { UpgradeID::SHIP_SPEED,    0, 3, UpgradeID::WARP_DRIVE,    1 },
    { UpgradeID::SPEED_EFFICIENCY, 0, 5, UpgradeID::SHIP_SPEED,  3 },
    { UpgradeID::AUTO_WARP,       0, 4, UpgradeID::WARP_DRIVE,    5 },
    { UpgradeID::WARP_ORE_BONUS,  1, 4, UpgradeID::WARP_DRIVE,    3 },
    { UpgradeID::FUEL_WARP_REFILL, 2, 4, UpgradeID::WARP_DRIVE, 1 },

    { UpgradeID::UNLOCK_BRONZE,  2, 2, UpgradeID::ORE_VALUE,      3 },
    { UpgradeID::UNLOCK_SILVER,  4, 2, UpgradeID::UNLOCK_BRONZE,  1 },
    { UpgradeID::UNLOCK_GOLD,    5, 2, UpgradeID::UNLOCK_SILVER,  1 },
    { UpgradeID::UNLOCK_DIAMOND, 6, 2, UpgradeID::UNLOCK_GOLD,    1 },
    { UpgradeID::UNLOCK_PLATINUM,7, 2, UpgradeID::UNLOCK_DIAMOND, 1 },
    { UpgradeID::UNLOCK_TITANIUM,8, 2, UpgradeID::UNLOCK_PLATINUM,1 },
    { UpgradeID::UNLOCK_IRIDIUM, 9, 2, UpgradeID::UNLOCK_TITANIUM,1 },

    { UpgradeID::AUTO_PLINKO,    3, 4, UpgradeID::PLINKO_BALLS,    5 },
    { UpgradeID::AUTO_SELL_THRESHOLD, 4, 4, UpgradeID::AUTO_PLINKO, 3 },
    { UpgradeID::PLINKO_ROWS,    2, 5, UpgradeID::PLINKO_BALLS,    3 },

    { UpgradeID::CREDIT_MULT,    3, 3, UpgradeID::PLINKO_BALLS,    1 },
    { UpgradeID::BULK_PROCESS,   4, 3, UpgradeID::CREDIT_MULT,    3 },

    { UpgradeID::METEOR_DAMAGE,  5, 0, UpgradeID::SPLIT_SHOT,     1 },
    { UpgradeID::METEOR_SIZE,    6, 0, UpgradeID::METEOR_DAMAGE,  3 },

    { UpgradeID::FUEL_CAPACITY,  5, 5, UpgradeID::WARP_DRIVE,     1 },
    { UpgradeID::FUEL_EFFICIENCY, 6, 5, UpgradeID::FUEL_CAPACITY, 1 },
    { UpgradeID::FUEL_ON_KILL,   4, 5, UpgradeID::FUEL_CAPACITY, 1 },
    { UpgradeID::SHIELD_HP,       0, 6, UpgradeID::FUEL_ON_KILL,  1 },
    { UpgradeID::SHIELD_RECHARGE,  1, 6, UpgradeID::SHIELD_HP,     2 },
    { UpgradeID::SHIELD_DELAY,    2, 6, UpgradeID::SHIELD_RECHARGE, 2 },
    { UpgradeID::SHIELD_MULTI_HIT, 3, 6, UpgradeID::SHIELD_DELAY, 3 },
    { UpgradeID::FUEL_ON_PICKUP, 4, 6, UpgradeID::AUTO_COLLECT_RADIUS, 1 },
};

SkillTreeSection skillTreeSectionOf(UpgradeID id) {
    switch (id) {
    case UpgradeID::ORE_VALUE:
    case UpgradeID::AUTO_COLLECT_RADIUS:
    case UpgradeID::ORE_LUCK:
    case UpgradeID::ORE_ON_KILL:
    case UpgradeID::ASTEROID_HP:
    case UpgradeID::EXPLOSIVE_ASTEROIDS:
    case UpgradeID::CHAIN_REACTION:
    case UpgradeID::UNLOCK_BRONZE:
    case UpgradeID::UNLOCK_SILVER:
    case UpgradeID::UNLOCK_GOLD:
    case UpgradeID::UNLOCK_DIAMOND:
    case UpgradeID::UNLOCK_PLATINUM:
    case UpgradeID::UNLOCK_TITANIUM:
    case UpgradeID::UNLOCK_IRIDIUM:
        return SkillTreeSection::ASTEROIDS;

    case UpgradeID::PLINKO_BALLS:
    case UpgradeID::PLINKO_ROWS:
    case UpgradeID::AUTO_PLINKO:
    case UpgradeID::AUTO_SELL_THRESHOLD:
    case UpgradeID::CREDIT_MULT:
    case UpgradeID::BULK_PROCESS:
        return SkillTreeSection::PLINKO;

    case UpgradeID::METEOR_DAMAGE:
    case UpgradeID::METEOR_SIZE:
        return SkillTreeSection::MISC;

    default:
        return SkillTreeSection::SHIP;
    }
}

namespace {

const char* sectionTabLabel(SkillTreeSection s) {
    switch (s) {
    case SkillTreeSection::SHIP:      return "Ship";
    case SkillTreeSection::ASTEROIDS: return "Asteroids";
    case SkillTreeSection::PLINKO:    return "Plinko";
    case SkillTreeSection::MISC:      return "Keukenlaatje";
    default:                          return "?";
    }
}

const UpgradeNodeDef* findNodeById(UpgradeID id) {
    for (const auto& n : SKILL_TREE_NODES) {
        if (n.id == id)
            return &n;
    }
    return nullptr;
}

} // namespace

void SkillTreeScreen::init(sf::Font& font,
                            float panelX, float panelY,
                            float panelW, float panelH,
                            float uiScale) {
    m_font    = &font;
    m_x       = panelX;
    m_y       = panelY;
    m_w       = panelW;
    m_h       = panelH;
    m_uiScale = std::max(0.75f, uiScale);
    resetScroll();
}

void SkillTreeScreen::resetScroll() {
    m_scrollX = 0.f;
    m_scrollY = 0.f;
    m_dragAxis = 0;
    m_dragGrabOffset = 0.f;
    updateScrollLimits();
}

bool SkillTreeScreen::nodeInActiveSection(const UpgradeNodeDef& node) const {
    return skillTreeSectionOf(node.id) == m_activeSection;
}

float SkillTreeScreen::sectionTabBarHeight() const {
    return std::round(36.f * m_uiScale);
}

float SkillTreeScreen::contentTop() const {
    return m_y + sectionTabBarHeight();
}

float SkillTreeScreen::contentHeight() const {
    return std::max(0.f, m_h - sectionTabBarHeight());
}

int SkillTreeScreen::sectionTabAt(sf::Vector2f pos) const {
    if (pos.y < m_y || pos.y >= contentTop())
        return -1;
    const float tabW = m_w / static_cast<float>(SkillTreeSection::COUNT);
    const int   idx  = static_cast<int>((pos.x - m_x) / tabW);
    if (idx < 0 || idx >= static_cast<int>(SkillTreeSection::COUNT))
        return -1;
    return idx;
}

void SkillTreeScreen::drawSectionTabs(sf::RenderTarget& target) const {
    const float tabH = sectionTabBarHeight();
    const float tabW =
        m_w / static_cast<float>(SkillTreeSection::COUNT);
    const unsigned fs = fontSz(12);

    for (int i = 0; i < static_cast<int>(SkillTreeSection::COUNT); ++i) {
        const auto    sec     = static_cast<SkillTreeSection>(i);
        const bool    active  = (sec == m_activeSection);
        const float   tx      = m_x + tabW * static_cast<float>(i);
        sf::RectangleShape tab({ tabW, tabH });
        tab.setPosition({ tx, m_y });
        tab.setFillColor(active ? sf::Color(35, 48, 82, 255)
                                : sf::Color(18, 22, 38, 255));
        tab.setOutlineColor(active ? sf::Color(100, 160, 255, 220)
                                   : sf::Color(45, 55, 85, 180));
        tab.setOutlineThickness(active ? 2.f : 1.f);
        target.draw(tab);

        sf::Text lbl(*m_font);
        lbl.setString(sectionTabLabel(sec));
        lbl.setCharacterSize(fs);
        lbl.setStyle(active ? sf::Text::Bold : sf::Text::Regular);
        lbl.setFillColor(active ? sf::Color(200, 220, 255)
                                : sf::Color(110, 120, 150));
        const auto lb = lbl.getLocalBounds();
        lbl.setPosition({
            tx + (tabW - lb.size.x) * 0.5f - lb.position.x,
            m_y + (tabH - lb.size.y) * 0.5f - lb.position.y });
        target.draw(lbl);
    }

    sf::RectangleShape sep(sf::Vector2f{ m_w, 1.f });
    sep.setPosition({ m_x, contentTop() - 1.f });
    sep.setFillColor(sf::Color(55, 70, 110, 200));
    target.draw(sep);
}

void SkillTreeScreen::updateScrollLimits() const {
    int maxGX = 0;
    int maxGY = 0;
    bool any = false;
    for (const auto& n : SKILL_TREE_NODES) {
        if (!nodeInActiveSection(n))
            continue;
        any = true;
        maxGX = std::max(maxGX, n.gridX);
        maxGY = std::max(maxGY, n.gridY);
    }
    if (!any) {
        m_contentW = viewportW();
        m_contentH = viewportH();
        m_minScrollX = 0.f;
        m_minScrollY = 0.f;
        return;
    }
    m_contentW =
        panelPad() + static_cast<float>(maxGX) * gridStepX() + nodeW() + panelPad();
    m_contentH =
        panelPad() + static_cast<float>(maxGY) * gridStepY() + nodeH() + panelPad();
    m_minScrollX = std::min(0.f, viewportW() - m_contentW);
    m_minScrollY = std::min(0.f, viewportH() - m_contentH);
    m_scrollX = std::clamp(m_scrollX, m_minScrollX, 0.f);
    m_scrollY = std::clamp(m_scrollY, m_minScrollY, 0.f);
}

float SkillTreeScreen::scrollBarThickness() const {
    return std::round(14.f * m_uiScale);
}

float SkillTreeScreen::viewportW() const {
    const float bar = scrollBarThickness();
    return std::max(0.f, m_w - (scrollNeededY() ? bar : 0.f));
}

float SkillTreeScreen::viewportH() const {
    const float bar = scrollBarThickness();
    return std::max(0.f, contentHeight() - (scrollNeededX() ? bar : 0.f));
}

bool SkillTreeScreen::scrollNeededX() const {
    const float bar = scrollBarThickness();
    return m_contentW > m_w - bar + 0.5f;
}

bool SkillTreeScreen::scrollNeededY() const {
    const float bar = scrollBarThickness();
    return m_contentH > contentHeight() - bar + 0.5f;
}

sf::FloatRect SkillTreeScreen::hTrackBounds() const {
    if (!scrollNeededX())
        return {};
    const float bar = scrollBarThickness();
    const float vBar = scrollNeededY() ? bar : 0.f;
    return { { m_x, contentTop() + contentHeight() - bar },
             { m_w - vBar, bar } };
}

sf::FloatRect SkillTreeScreen::vTrackBounds() const {
    if (!scrollNeededY())
        return {};
    const float bar = scrollBarThickness();
    const float hBar = scrollNeededX() ? bar : 0.f;
    return { { m_x + m_w - bar, contentTop() },
             { bar, contentHeight() - hBar } };
}

sf::FloatRect SkillTreeScreen::hThumbBounds() const {
    const sf::FloatRect track = hTrackBounds();
    if (track.size.x <= 0.f)
        return {};
    const float vw = viewportW();
    const float minThumb = std::round(28.f * m_uiScale);
    const float thumbW =
        std::clamp(track.size.x * (vw / m_contentW), minThumb, track.size.x);
    const float travel = std::max(0.f, track.size.x - thumbW);
    const float t =
        (m_minScrollX < 0.f) ? (m_scrollX / m_minScrollX) : 0.f;
    return { { track.position.x + travel * t, track.position.y },
             { thumbW, track.size.y } };
}

sf::FloatRect SkillTreeScreen::vThumbBounds() const {
    const sf::FloatRect track = vTrackBounds();
    if (track.size.y <= 0.f)
        return {};
    const float vh = viewportH();
    const float minThumb = std::round(28.f * m_uiScale);
    const float thumbH =
        std::clamp(track.size.y * (vh / m_contentH), minThumb, track.size.y);
    const float travel = std::max(0.f, track.size.y - thumbH);
    const float t =
        (m_minScrollY < 0.f) ? (m_scrollY / m_minScrollY) : 0.f;
    return { { track.position.x, track.position.y + travel * t },
             { track.size.x, thumbH } };
}

void SkillTreeScreen::setScrollFromHThumbCenter(float thumbCenterX) {
    const sf::FloatRect track = hTrackBounds();
    const sf::FloatRect thumb = hThumbBounds();
    if (track.size.x <= 0.f)
        return;
    const float travel = std::max(0.f, track.size.x - thumb.size.x);
    const float t = (travel > 0.f)
        ? std::clamp((thumbCenterX - track.position.x) / travel, 0.f, 1.f)
        : 0.f;
    m_scrollX = m_minScrollX * t;
}

void SkillTreeScreen::setScrollFromVThumbCenter(float thumbCenterY) {
    const sf::FloatRect track = vTrackBounds();
    const sf::FloatRect thumb = vThumbBounds();
    if (track.size.y <= 0.f)
        return;
    const float travel = std::max(0.f, track.size.y - thumb.size.y);
    const float t = (travel > 0.f)
        ? std::clamp((thumbCenterY - track.position.y) / travel, 0.f, 1.f)
        : 0.f;
    m_scrollY = m_minScrollY * t;
}

void SkillTreeScreen::drawScrollBars(sf::RenderTarget& target) const {
    const sf::Color trackFill(22, 28, 48, 240);
    const sf::Color trackEdge(55, 70, 110, 200);
    const sf::Color thumbFill(70, 110, 190, 230);
    const sf::Color thumbEdge(130, 180, 255, 220);

    auto drawTrack = [&](const sf::FloatRect& r) {
        sf::RectangleShape t(r.size);
        t.setPosition(r.position);
        t.setFillColor(trackFill);
        t.setOutlineColor(trackEdge);
        t.setOutlineThickness(1.f);
        target.draw(t);
    };
    auto drawThumb = [&](const sf::FloatRect& r) {
        sf::RectangleShape th(r.size);
        th.setPosition(r.position);
        th.setFillColor(thumbFill);
        th.setOutlineColor(thumbEdge);
        th.setOutlineThickness(1.f);
        target.draw(th);
    };

    if (scrollNeededX()) {
        drawTrack(hTrackBounds());
        drawThumb(hThumbBounds());
    }
    if (scrollNeededY()) {
        drawTrack(vTrackBounds());
        drawThumb(vThumbBounds());
    }
}

sf::Vector2f SkillTreeScreen::nodeScreenPos(const UpgradeNodeDef& node) const {
    return {
        m_x + panelPad() + static_cast<float>(node.gridX) * gridStepX() + m_scrollX,
        contentTop() + panelPad()
            + static_cast<float>(node.gridY) * gridStepY() + m_scrollY,
    };
}

sf::FloatRect SkillTreeScreen::nodeRect(const UpgradeNodeDef& node) const {
    const auto pos = nodeScreenPos(node);
    return { pos, { nodeW(), nodeH() } };
}

void SkillTreeScreen::draw(sf::RenderTarget& target,
                            const GameState&  state,
                            bool              seeThroughMiningBackdrop) const {
    updateScrollLimits();

    sf::RectangleShape bg({ m_w, m_h });
    bg.setPosition({ m_x, m_y });
    bg.setFillColor(hubBackdropTint(sf::Color(10, 12, 24, 255),
                                    seeThroughMiningBackdrop));
    target.draw(bg);

    m_hoveredNode = nullptr;

    drawSectionTabs(target);
    drawConnections(target, state);

    for (const auto& node : SKILL_TREE_NODES) {
        if (!nodeInActiveSection(node))
            continue;
        if (!state.isNodeVisible(node))
            continue;
        drawNode(target, node, state, seeThroughMiningBackdrop);
    }

    if (m_hoveredNode)
        drawTooltip(target, *m_hoveredNode, state, nodeScreenPos(*m_hoveredNode));

    drawScrollBars(target);
}

void SkillTreeScreen::drawConnections(sf::RenderTarget& target,
                                       const GameState&  state) const {
    for (const auto& node : SKILL_TREE_NODES) {
        if (!nodeInActiveSection(node))
            continue;
        if (!state.isNodeVisible(node))
            continue;
        if (node.requireId == UpgradeID::UPGRADE_COUNT)
            continue;

        const UpgradeNodeDef* parent = findNodeById(node.requireId);
        if (!parent || !nodeInActiveSection(*parent))
            continue;
        if (!state.isNodeVisible(*parent))
            continue;

        sf::Vector2f from = nodeScreenPos(*parent);
        from.x += nodeW() * 0.5f;
        from.y += nodeH() * 0.5f;

        sf::Vector2f to = nodeScreenPos(node);
        to.x += nodeW() * 0.5f;
        to.y += nodeH() * 0.5f;

        const bool unlocked = state.isNodeUnlocked(node);
        const sf::Color lineColor = unlocked
            ? sf::Color(80, 160, 80, 200)
            : sf::Color(60, 60, 80, 140);

        sf::Vertex line[2];
        line[0].position = from;
        line[0].color    = lineColor;
        line[1].position = to;
        line[1].color    = lineColor;
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void SkillTreeScreen::drawNode(sf::RenderTarget&     target,
                                const UpgradeNodeDef& node,
                                const GameState&      state,
                                bool seeThroughMiningBackdrop) const {
    const auto rect = nodeRect(node);
    const auto pos  = nodeScreenPos(node);

    if (rect.position.x + rect.size.x < m_x || rect.position.x > m_x + m_w)
        return;
    if (rect.position.y + rect.size.y < contentTop()
        || rect.position.y > contentTop() + contentHeight())
        return;

    const bool unlocked = state.isNodeUnlocked(node);
    const bool bought   = state.levelOf(node.id) > 0;
    const auto& def =
        GameState::upgradeCatalog[static_cast<std::size_t>(
            static_cast<int>(node.id))];
    const int  maxLv = state.effectiveMaxLevel(node.id);
    const bool maxed =
        maxLv > 0 && state.levelOf(node.id) >= maxLv;
    const bool zoneLocked =
        node.id >= UpgradeID::UNLOCK_BRONZE
        && node.id <= UpgradeID::UNLOCK_IRIDIUM
        && !state.isOreTierUnlockAvailable(node.id);
    const bool affordable = !zoneLocked && state.canBuy(node.id);
    const bool hovered    = rect.contains(m_mousePos);
    if (hovered)
        m_hoveredNode = &node;

    sf::Color bgColor;
    if (!unlocked)
        bgColor = sf::Color(20, 22, 35, 220);
    else if (maxed)
        bgColor = sf::Color(20, 80, 20, 230);
    else if (bought)
        bgColor = sf::Color(15, 40, 70, 230);
    else if (affordable)
        bgColor = sf::Color(20, 25, 50, 230);
    else
        bgColor = sf::Color(18, 20, 38, 230);

    sf::Color borderColor;
    if (!unlocked)
        borderColor = sf::Color(50, 50, 70);
    else if (maxed)
        borderColor = sf::Color(60, 200, 60);
    else if (hovered && affordable)
        borderColor = sf::Color(120, 180, 255);
    else if (affordable)
        borderColor = sf::Color(80, 140, 255);
    else
        borderColor = sf::Color(60, 80, 120);

    const float padIn = 8.f * m_uiScale;
    const float line1 = 8.f * m_uiScale;
    const float line2 = 28.f * m_uiScale;
    const float line3 = 46.f * m_uiScale;

    sf::RectangleShape bg({ nodeW(), nodeH() });
    bg.setPosition(pos);
    bg.setFillColor(hubBackdropTint(bgColor, seeThroughMiningBackdrop));
    bg.setOutlineColor(borderColor);
    bg.setOutlineThickness((hovered ? 2.5f : 2.f) * m_uiScale);
    target.draw(bg);

    sf::Text name(*m_font);
    name.setString(def.name);
    name.setCharacterSize(fontSz(13));
    name.setFillColor(unlocked ? sf::Color(220, 230, 255)
                               : sf::Color(80, 80, 100));
    name.setPosition({ pos.x + padIn, pos.y + line1 });
    target.draw(name);

    const int lv = state.levelOf(node.id);
    if (lv > 0) {
        sf::Text lvText(*m_font);
        lvText.setString("Lv " + std::to_string(lv));
        lvText.setCharacterSize(fontSz(12));
        lvText.setFillColor(sf::Color(120, 200, 120));
        lvText.setPosition({ pos.x + padIn, pos.y + line2 });
        target.draw(lvText);
    }

    if (node.id == UpgradeID::AUTO_PLINKO && state.autoPlinkoUnlockedByBoss()
        && lv == 0) {
        sf::Text bossLabel(*m_font);
        bossLabel.setString("Boss Reward!");
        bossLabel.setCharacterSize(fontSz(12));
        bossLabel.setStyle(sf::Text::Bold);
        bossLabel.setFillColor(sf::Color(255, 170, 0));
        bossLabel.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(bossLabel);
        return;
    }

    if (unlocked && zoneLocked) {
        sf::Text zoneText(*m_font);
        zoneText.setString(
            "Zone " + std::to_string(state.oreTierUnlockRequiredZone(node.id))
            + " vereist");
        zoneText.setCharacterSize(fontSz(11));
        zoneText.setFillColor(sf::Color(255, 160, 90));
        zoneText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(zoneText);
    } else if (unlocked && !maxed) {
        sf::Text priceText(*m_font);
        priceText.setString("$" + formatBig(state.costOf(node.id)));
        priceText.setCharacterSize(fontSz(12));
        priceText.setFillColor(affordable ? sf::Color(255, 215, 50)
                                          : sf::Color(120, 80, 80));
        priceText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(priceText);
    }

    if (!unlocked) {
        sf::Text lockText(*m_font);
        lockText.setString("[LOCKED]");
        lockText.setCharacterSize(fontSz(11));
        lockText.setFillColor(sf::Color(80, 80, 100));
        lockText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(lockText);

        if (const UpgradeNodeDef* req = findNodeById(node.requireId)) {
            const auto& reqDef =
                GameState::upgradeCatalog[static_cast<std::size_t>(
                    static_cast<int>(req->id))];
            std::string needs = "Needs: " + reqDef.name;
            if (node.requireLevel > 1)
                needs += " lv" + std::to_string(node.requireLevel);
            if (!nodeInActiveSection(*req))
                needs += " ("
                    + std::string(sectionTabLabel(
                        skillTreeSectionOf(req->id)))
                    + ")";
            sf::Text reqText(*m_font);
            reqText.setString(needs);
            reqText.setCharacterSize(fontSz(10));
            reqText.setFillColor(sf::Color(100, 100, 130));
            reqText.setPosition({ pos.x + padIn, pos.y + nodeH() - 16.f * m_uiScale });
            target.draw(reqText);
        }
    }

    if (maxed) {
        sf::Text maxText(*m_font);
        maxText.setString("MAXED");
        maxText.setCharacterSize(fontSz(12));
        maxText.setStyle(sf::Text::Bold);
        maxText.setFillColor(sf::Color(80, 220, 80));
        maxText.setPosition({ pos.x + padIn, pos.y + line3 });
        target.draw(maxText);
    }
}

void SkillTreeScreen::drawTooltip(sf::RenderTarget&     target,
                                   const UpgradeNodeDef& node,
                                   const GameState&      state,
                                   sf::Vector2f          nodePos) const {
    const auto& def =
        GameState::upgradeCatalog[static_cast<std::size_t>(
            static_cast<int>(node.id))];

    const float tw = 220.f * m_uiScale;
    const float th = 96.f * m_uiScale;
    const float tipPad = 8.f * m_uiScale;
    float tx = nodePos.x + nodeW() + tipPad;
    float ty = nodePos.y;
    if (tx + tw > m_x + m_w)
        tx = nodePos.x - tw - tipPad;
    if (ty + th > m_y + m_h)
        ty = m_y + m_h - th - 4.f * m_uiScale;

    sf::RectangleShape bg({ tw, th });
    bg.setPosition({ tx, ty });
    bg.setFillColor(sf::Color(10, 12, 30, 240));
    bg.setOutlineColor(sf::Color(80, 100, 180));
    bg.setOutlineThickness(std::max(1.f, 1.f * m_uiScale));
    target.draw(bg);

    sf::Text title(*m_font);
    title.setString(def.name);
    title.setCharacterSize(fontSz(15));
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color(200, 220, 255));
    title.setPosition({ tx + tipPad, ty + tipPad });
    target.draw(title);

    sf::Text desc(*m_font);
    desc.setString(def.description);
    desc.setCharacterSize(fontSz(13));
    desc.setFillColor(sf::Color(160, 170, 200));
    desc.setPosition({ tx + tipPad, ty + 30.f * m_uiScale });
    target.draw(desc);

    const int lv = state.levelOf(node.id);
    std::string lvStr = "Level: " + std::to_string(lv);
    const int maxLv = state.effectiveMaxLevel(node.id);
    if (maxLv > 0)
        lvStr += " / " + std::to_string(maxLv);
    sf::Text lvInfo(*m_font);
    lvInfo.setString(lvStr);
    lvInfo.setCharacterSize(fontSz(13));
    lvInfo.setFillColor(sf::Color(120, 200, 120));
    lvInfo.setPosition({ tx + tipPad, ty + 52.f * m_uiScale });
    target.draw(lvInfo);

    sf::Text costInfo(*m_font);
    costInfo.setString("Cost: $" + formatBig(state.costOf(node.id)));
    costInfo.setCharacterSize(fontSz(13));
    costInfo.setFillColor(sf::Color(255, 215, 50));
    costInfo.setPosition({ tx + tipPad, ty + 72.f * m_uiScale });
    target.draw(costInfo);
}

void SkillTreeScreen::setTutorialHighlight(UpgradeID id) {
    m_tutorialHighlight = id;
}

void SkillTreeScreen::setActiveSection(SkillTreeSection section) {
    m_activeSection = section;
    resetScroll();
}

bool SkillTreeScreen::handleClick(sf::Vector2f pos, GameState& state,
                                  bool shiftHeld) {
    const int tab = sectionTabAt(pos);
    if (tab >= 0) {
        const auto next = static_cast<SkillTreeSection>(tab);
        if (next != m_activeSection) {
            m_activeSection = next;
            resetScroll();
        }
        return false;
    }

    for (const auto& node : SKILL_TREE_NODES) {
        if (!nodeInActiveSection(node))
            continue;
        if (!state.isNodeVisible(node))
            continue;
        if (!state.isNodeUnlocked(node))
            continue;
        if (!nodeRect(node).contains(pos))
            continue;

        const int maxLv = state.effectiveMaxLevel(node.id);
        const bool maxed =
            maxLv > 0 && state.levelOf(node.id) >= maxLv;
        if (shiftHeld && maxed && state.canCapBreak(node.id)) {
            return state.buyCapBreak(node.id);
        }
        if (!state.canBuy(node.id))
            continue;
        state.buy(node.id);
        return true;
    }
    return false;
}

void SkillTreeScreen::handleScroll(float delta, sf::Vector2f /*pos*/,
                                   bool shiftHeld) {
    updateScrollLimits();
    const float step = 48.f * m_uiScale;
    if (shiftHeld && scrollNeededX()) {
        m_scrollX += delta * step;
        m_scrollX = std::clamp(m_scrollX, m_minScrollX, 0.f);
    } else {
        m_scrollY += delta * step;
        m_scrollY = std::clamp(m_scrollY, m_minScrollY, 0.f);
    }
}

bool SkillTreeScreen::handlePointerDown(sf::Vector2f pos) {
    if (sectionTabAt(pos) >= 0)
        return false;

    updateScrollLimits();
    m_dragAxis = 0;

    if (scrollNeededX()) {
        const sf::FloatRect thumb = hThumbBounds();
        const sf::FloatRect track = hTrackBounds();
        if (thumb.contains(pos)) {
            m_dragAxis = 1;
            m_dragGrabOffset = pos.x - (thumb.position.x + thumb.size.x * 0.5f);
            return true;
        }
        if (track.contains(pos)) {
            setScrollFromHThumbCenter(pos.x - m_dragGrabOffset);
            m_dragAxis = 1;
            m_dragGrabOffset = 0.f;
            return true;
        }
    }
    if (scrollNeededY()) {
        const sf::FloatRect thumb = vThumbBounds();
        const sf::FloatRect track = vTrackBounds();
        if (thumb.contains(pos)) {
            m_dragAxis = 2;
            m_dragGrabOffset = pos.y - (thumb.position.y + thumb.size.y * 0.5f);
            return true;
        }
        if (track.contains(pos)) {
            setScrollFromVThumbCenter(pos.y - m_dragGrabOffset);
            m_dragAxis = 2;
            m_dragGrabOffset = 0.f;
            return true;
        }
    }
    return false;
}

void SkillTreeScreen::handlePointerMove(sf::Vector2f pos) {
    if (m_dragAxis == 1)
        setScrollFromHThumbCenter(pos.x - m_dragGrabOffset);
    else if (m_dragAxis == 2)
        setScrollFromVThumbCenter(pos.y - m_dragGrabOffset);
}

void SkillTreeScreen::handlePointerUp() {
    m_dragAxis = 0;
    m_dragGrabOffset = 0.f;
}
