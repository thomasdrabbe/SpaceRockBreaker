#include "Shop.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────
//  Category metadata
// ─────────────────────────────────────────────────────────────
namespace {

struct CatInfo {
    std::string            label;
    ShopCategory           cat;
    std::vector<UpgradeID> ids;
};

const std::vector<CatInfo> CATEGORIES = {
    { "[W] Weapons", ShopCategory::WEAPONS,
      { UpgradeID::GUN_DAMAGE,   UpgradeID::FIRE_RATE,
        UpgradeID::TURRET_COUNT, UpgradeID::CRIT_CHANCE,
        UpgradeID::CRIT_MULT,    UpgradeID::SPLIT_SHOT } },

    { "[M] Mining",  ShopCategory::MINING,
      { UpgradeID::ORE_VALUE,          UpgradeID::AUTO_COLLECT_RADIUS,
        UpgradeID::ORE_LUCK,           UpgradeID::ASTEROID_HP } },

    { "[P] Plinko",  ShopCategory::PLINKO,
      { UpgradeID::PLINKO_ROWS,  UpgradeID::PLINKO_MULT,
        UpgradeID::PLINKO_BALLS, UpgradeID::PLINKO_LUCK } },

    { "[E] Economy", ShopCategory::ECONOMY,
      { UpgradeID::CREDIT_MULT, UpgradeID::BULK_PROCESS,
        UpgradeID::AUTO_PLINKO } },
};

const sf::Color CAT_COLORS[] = {
    sf::Color(255, 100,  80),   // Weapons
    sf::Color( 80, 200, 120),   // Mining
    sf::Color(160, 100, 255),   // Plinko
    sf::Color(255, 200,  60),   // Economy
};

} // anonymous namespace

// ═════════════════════════════════════════════════════════════
//  Constructor / init
// ═════════════════════════════════════════════════════════════
Shop::Shop() {}

void Shop::init(sf::Font& font,
                float panelX, float panelY,
                float panelW, float panelH) {
    m_font = &font;
    m_x    = panelX;
    m_y    = panelY;
    m_w    = panelW;
    m_h    = panelH;
}

// ═════════════════════════════════════════════════════════════
//  buildCards
// ═════════════════════════════════════════════════════════════
void Shop::buildCards(const GameState& state) {
    m_cards.clear();

    const auto& cat   = CATEGORIES[static_cast<int>(m_activeCategory)];
    float contentX    = m_x + CARD_MARGIN;
    float cardW       = m_w - CARD_MARGIN * 2.f
                        - SCROLLBAR_W - 4.f;
    float startY      = m_y + TAB_H + CARD_MARGIN;

    int idx = 0;
    for (UpgradeID id : cat.ids) {
        UpgradeCard card;
        card.id         = id;
        card.bounds     = sf::FloatRect(
            { contentX,
              startY + idx * (CARD_H + CARD_MARGIN) - m_scroll },
            { cardW, CARD_H });
        card.affordable = state.canBuy(id);
        m_cards.push_back(card);
        idx++;
    }

    float totalH   = cat.ids.size() * (CARD_H + CARD_MARGIN);
    float visibleH = m_h - TAB_H - CARD_MARGIN;
    m_maxScroll    = std::max(0.f, totalH - visibleH);
    m_scroll       = clamp(m_scroll, 0.f, m_maxScroll);
}

// ═════════════════════════════════════════════════════════════
//  tabBounds
// ═════════════════════════════════════════════════════════════
sf::FloatRect Shop::tabBounds(int idx) const {
    float tabW = m_w / static_cast<float>(
        static_cast<int>(ShopCategory::CATEGORY_COUNT));
    return sf::FloatRect(
        { m_x + idx * tabW, m_y },
        { tabW, TAB_H });
}

// ═════════════════════════════════════════════════════════════
//  handleEvent  — SFML 3 event system
// ═════════════════════════════════════════════════════════════
bool Shop::handleEvent(const sf::Event& event,
                        GameState&       state) {

    if (const auto* e =
            event.getIf<sf::Event::MouseButtonPressed>()) {

        if (e->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp(static_cast<float>(e->position.x),
                            static_cast<float>(e->position.y));

            // Category tab click
            for (int i = 0; i < static_cast<int>(
                     ShopCategory::CATEGORY_COUNT); i++) {
                if (tabBounds(i).contains(mp)) {
                    m_activeCategory = static_cast<ShopCategory>(i);
                    m_scroll         = 0.f;
                    buildCards(state);
                    return false;
                }
            }

            // Card click
            for (auto& card : m_cards) {
                if (card.bounds.contains(mp) && card.affordable) {
                    state.buy(card.id);
                    buildCards(state);
                    return true;
                }
            }
        }
    }

    if (const auto* e =
            event.getIf<sf::Event::MouseWheelScrolled>()) {
        scrollBy(-e->delta * 30.f);
        buildCards(state);
    }

    return false;
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Shop::update(sf::Vector2f mousePos,
                   const GameState& state) {
    buildCards(state);
    for (auto& card : m_cards)
        card.hovered = card.bounds.contains(mousePos);
}

void Shop::scrollBy(float delta) {
    m_scroll = clamp(m_scroll + delta, 0.f, m_maxScroll);
}

// ═════════════════════════════════════════════════════════════
//  formatEffect
// ═════════════════════════════════════════════════════════════
std::string Shop::formatEffect(UpgradeID        id,
                                 const GameState& state) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);

    switch (id) {
        case UpgradeID::GUN_DAMAGE:
            ss << state.gunDamage() << " dmg"; break;
        case UpgradeID::FIRE_RATE:
            ss << state.fireRatePerSec() << "/s"; break;
        case UpgradeID::TURRET_COUNT:
            ss << state.turretCount() << " turrets"; break;
        case UpgradeID::CRIT_CHANCE:
            ss << pct(state.critChance()); break;
        case UpgradeID::CRIT_MULT:
            ss << state.critMult() << "x"; break;
        case UpgradeID::SPLIT_SHOT:
            ss << state.splitShot() << " bullets"; break;
        case UpgradeID::ORE_VALUE:
            ss << state.oreValueMult() << "x"; break;
        case UpgradeID::AUTO_COLLECT_RADIUS:
            ss << state.autoCollectRadius() << "px"; break;
        case UpgradeID::ORE_LUCK:
            ss << pct(state.oreLuckBonus()); break;
        case UpgradeID::ASTEROID_HP: {
            float hpMult = std::max(0.1f,
                1.f - state.levelOf(UpgradeID::ASTEROID_HP) * 0.1f);
            ss << std::setprecision(0)
               << hpMult * 100.f << "% HP"; break;
        }
        case UpgradeID::PLINKO_ROWS:
            ss << state.plinkoRows() << " rows"; break;
        case UpgradeID::PLINKO_MULT:
            ss << state.plinkoMultBonus() << "x"; break;
        case UpgradeID::PLINKO_BALLS:
            ss << state.maxPlinkoBalls() << " balls"; break;
        case UpgradeID::PLINKO_LUCK:
            ss << pct(state.plinkoLuck()); break;
        case UpgradeID::CREDIT_MULT:
            ss << state.creditMult() << "x"; break;
        case UpgradeID::BULK_PROCESS:
            ss << state.bulkProcess() << "x bulk"; break;
        case UpgradeID::AUTO_PLINKO:
            ss << (state.autoPlinkoEnabled() ? "ON" : "OFF"); break;
        default:
            ss << "Lv " << state.levelOf(id); break;
    }
    return ss.str();
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void Shop::draw(sf::RenderTarget& target,
                 const GameState&  state) const {
    drawBackground(target);
    drawCategoryTabs(target, state);

    // Clip kaarten via viewport
    sf::View oldView = target.getView();

    float vpX = m_x / WINDOW_WIDTH;
    float vpY = (m_y + TAB_H) / WINDOW_HEIGHT;
    float vpW = m_w / WINDOW_WIDTH;
    float vpH = (m_h - TAB_H) / WINDOW_HEIGHT;

    sf::View cardView(sf::FloatRect(
        { m_x, m_y + TAB_H },
        { m_w, m_h - TAB_H }));
    cardView.setViewport(sf::FloatRect(
        { vpX, vpY }, { vpW, vpH }));
    target.setView(cardView);

    for (const auto& card : m_cards)
        drawCard(target, card, state);

    target.setView(oldView);
    drawScrollBar(target);
}

// ─────────────────────────────────────────────────────────────
//  drawBackground
// ─────────────────────────────────────────────────────────────
void Shop::drawBackground(sf::RenderTarget& target) const {
    sf::RectangleShape bg(sf::Vector2f{ m_w, m_h });
    bg.setPosition({ m_x, m_y });
    bg.setFillColor(sf::Color(12, 14, 28, 245));
    bg.setOutlineColor(sf::Color(50, 60, 100, 180));
    bg.setOutlineThickness(1.f);
    target.draw(bg);
}

// ─────────────────────────────────────────────────────────────
//  drawCategoryTabs
// ─────────────────────────────────────────────────────────────
void Shop::drawCategoryTabs(sf::RenderTarget& target,
                              const GameState& /*state*/) const {
    int count = static_cast<int>(ShopCategory::CATEGORY_COUNT);

    for (int i = 0; i < count; i++) {
        auto  bounds = tabBounds(i);
        bool  active = (m_activeCategory ==
                        static_cast<ShopCategory>(i));
        auto& accent = CAT_COLORS[i];

        sf::RectangleShape tab(sf::Vector2f{
            bounds.size.x, bounds.size.y });
        tab.setPosition(bounds.position);
        tab.setFillColor(active
            ? sf::Color(accent.r / 4, accent.g / 4,
                        accent.b / 4, 240)
            : sf::Color(18, 20, 38, 200));
        tab.setOutlineColor(active
            ? sf::Color(accent.r, accent.g, accent.b, 200)
            : sf::Color(40, 50, 80, 120));
        tab.setOutlineThickness(1.f);
        target.draw(tab);

        if (active) {
            sf::RectangleShape bar(sf::Vector2f{
                bounds.size.x - 4.f, 3.f });
            bar.setPosition({
                bounds.position.x + 2.f,
                bounds.position.y + bounds.size.y - 3.f });
            bar.setFillColor(accent);
            target.draw(bar);
        }

        sf::Text label(*m_font);
        label.setString(CATEGORIES[i].label);
        label.setCharacterSize(13);
        label.setStyle(sf::Text::Bold);
        label.setFillColor(active
            ? sf::Color(accent.r, accent.g, accent.b)
            : sf::Color(160, 170, 200));

        float lw = label.getLocalBounds().size.x;
        float lh = label.getLocalBounds().size.y;
        label.setPosition({
            bounds.position.x + (bounds.size.x - lw) * 0.5f,
            bounds.position.y + (bounds.size.y - lh) * 0.5f - 4.f
        });
        target.draw(label);
    }
}

// ─────────────────────────────────────────────────────────────
//  drawCard
// ─────────────────────────────────────────────────────────────
void Shop::drawCard(sf::RenderTarget&  target,
                     const UpgradeCard& card,
                     const GameState&   state) const {

    const auto& def   = GameState::upgradeCatalog[
                            static_cast<int>(card.id)];
    int         level = state.levelOf(card.id);
    double      cost  = state.costOf(card.id);
    bool        maxed = (def.maxLevel > 0 &&
                         level >= def.maxLevel);
    auto&       accent= CAT_COLORS[
                            static_cast<int>(m_activeCategory)];

    // Card background
    sf::RectangleShape bg(sf::Vector2f{
        card.bounds.size.x, card.bounds.size.y });
    bg.setPosition(card.bounds.position);

    if (maxed)
        bg.setFillColor(sf::Color(30, 40, 20, 220));
    else if (card.hovered && card.affordable)
        bg.setFillColor(sf::Color(
            accent.r / 6 + 20,
            accent.g / 6 + 20,
            accent.b / 6 + 20, 240));
    else
        bg.setFillColor(sf::Color(20, 24, 44, 220));

    bg.setOutlineColor(card.hovered
        ? sf::Color(accent.r, accent.g, accent.b, 200)
        : sf::Color(40, 50, 80, 120));
    bg.setOutlineThickness(1.f);
    target.draw(bg);

    float tx = card.bounds.position.x + CARD_PAD;
    float ty = card.bounds.position.y + CARD_PAD;

    // Name + level
    sf::Text name(*m_font);
    name.setCharacterSize(14);
    name.setStyle(sf::Text::Bold);
    name.setFillColor(maxed
        ? sf::Color(120, 200, 80)
        : sf::Color(220, 230, 255));
    std::string nameStr = def.name;
    nameStr += maxed
        ? "  [MAX]"
        : "  Lv " + std::to_string(level);
    name.setString(nameStr);
    name.setPosition({ tx, ty });
    target.draw(name);

    // Current effect
    sf::Text effect(*m_font);
    effect.setCharacterSize(12);
    effect.setFillColor(sf::Color(
        accent.r, accent.g, accent.b, 200));
    effect.setString(formatEffect(card.id, state));
    effect.setPosition({ tx, ty + 18.f });
    target.draw(effect);

    // Description
    sf::Text desc(*m_font);
    desc.setCharacterSize(11);
    desc.setFillColor(sf::Color(140, 150, 180));
    desc.setString(def.description);
    desc.setPosition({ tx, ty + 34.f });
    target.draw(desc);

    // Cost button
    if (!maxed) {
        float btnW = 100.f, btnH = 28.f;
        float btnX = card.bounds.position.x
                     + card.bounds.size.x - btnW - CARD_PAD;
        float btnY = card.bounds.position.y
                     + card.bounds.size.y - btnH - CARD_PAD;

        sf::RectangleShape btn(sf::Vector2f{ btnW, btnH });
        btn.setPosition({ btnX, btnY });
        btn.setFillColor(card.affordable
            ? sf::Color(accent.r/3, accent.g/3,
                        accent.b/3, 220)
            : sf::Color(40, 30, 30, 200));
        btn.setOutlineColor(card.affordable
            ? sf::Color(accent.r, accent.g, accent.b, 180)
            : sf::Color(80, 50, 50, 120));
        btn.setOutlineThickness(1.f);
        target.draw(btn);

        sf::Text costLabel(*m_font);
        costLabel.setCharacterSize(12);
        costLabel.setStyle(sf::Text::Bold);
        costLabel.setFillColor(card.affordable
            ? sf::Color(255, 230, 120)
            : sf::Color(120, 80, 80));
        costLabel.setString("$ " + formatCost(cost));

        float cw = costLabel.getLocalBounds().size.x;
        float ch = costLabel.getLocalBounds().size.y;
        costLabel.setPosition({
            btnX + (btnW - cw) * 0.5f,
            btnY + (btnH - ch) * 0.5f - 2.f });
        target.draw(costLabel);
    }
}

// ─────────────────────────────────────────────────────────────
//  drawScrollBar
// ─────────────────────────────────────────────────────────────
void Shop::drawScrollBar(sf::RenderTarget& target) const {
    if (m_maxScroll <= 0.f) return;

    float trackX = m_x + m_w - SCROLLBAR_W - 2.f;
    float trackY = m_y + TAB_H + 4.f;
    float trackH = m_h - TAB_H - 8.f;

    sf::RectangleShape track(sf::Vector2f{ SCROLLBAR_W, trackH });
    track.setPosition({ trackX, trackY });
    track.setFillColor(sf::Color(30, 35, 60, 160));
    target.draw(track);

    float thumbRatio = std::min(1.f, trackH /
                                (trackH + m_maxScroll));
    float thumbH     = std::max(20.f, trackH * thumbRatio);
    float thumbY     = trackY + (m_scroll / m_maxScroll)
                       * (trackH - thumbH);

    sf::RectangleShape thumb(sf::Vector2f{
        SCROLLBAR_W, thumbH });
    thumb.setPosition({ trackX, thumbY });
    thumb.setFillColor(sf::Color(100, 120, 200, 200));
    thumb.setOutlineColor(sf::Color(160, 180, 255, 120));
    thumb.setOutlineThickness(1.f);
    target.draw(thumb);
}
