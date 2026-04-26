#include "Shop.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

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
    { "[M] Mining", ShopCategory::MINING,
      { UpgradeID::ORE_VALUE,          UpgradeID::AUTO_COLLECT_RADIUS,
        UpgradeID::ORE_LUCK,           UpgradeID::ASTEROID_HP,
        UpgradeID::WARP_DRIVE } },
    { "[P] Plinko", ShopCategory::PLINKO,
    { UpgradeID::PLINKO_MULT,
      UpgradeID::PLINKO_BALLS, UpgradeID::PLINKO_LUCK,
      UpgradeID::AUTO_PLINKO } },
    { "[E] Economy", ShopCategory::ECONOMY,
      { UpgradeID::CREDIT_MULT,  UpgradeID::BULK_PROCESS,
        UpgradeID::AUTO_PLINKO } },
    { "[O] Ore Tiers", ShopCategory::ORE_TIERS,
      { UpgradeID::UNLOCK_BRONZE,
      UpgradeID::UNLOCK_SILVER,
      UpgradeID::UNLOCK_GOLD,
      UpgradeID::UNLOCK_DIAMOND,
      UpgradeID::UNLOCK_PLATINUM,
      UpgradeID::UNLOCK_TITANIUM,
      UpgradeID::UNLOCK_IRIDIUM } },
};

const sf::Color CAT_COLORS[] = {
    sf::Color(255, 100,  80),
    sf::Color( 80, 200, 120),
    sf::Color(160, 100, 255),
    sf::Color(255, 200,  60),
    sf::Color(120, 180, 255),
};

} // anonymous namespace

// ═════════════════════════════════════════════════════════════
//  Constructor / init
// ═════════════════════════════════════════════════════════════
Shop::Shop() {}

void Shop::init(sf::Font& font,
                float panelX, float panelY,
                float panelW, float panelH,
                float scale) {
    m_font  = &font;
    m_x     = panelX;
    m_y     = panelY;
    m_w     = panelW;
    m_h     = panelH;
    m_scale = scale;

    // Schaalbare layout-waarden
    m_tabH       = std::round(44.f  * m_scale);
    m_cardH      = std::round(100.f * m_scale);
    m_cardMargin = std::round(10.f  * m_scale);
    m_cardPad    = std::round(14.f  * m_scale);
    m_scrollBarW = std::round(8.f   * m_scale);
}

// ═════════════════════════════════════════════════════════════
//  tabBounds
// ═════════════════════════════════════════════════════════════
sf::FloatRect Shop::tabBounds(int idx) const {
    int   count = static_cast<int>(ShopCategory::CATEGORY_COUNT);
    float tabW  = (m_w - m_scrollBarW) / static_cast<float>(count);
    return sf::FloatRect(
        { m_x + idx * tabW, m_y },
        { tabW, m_tabH });
}

// ═════════════════════════════════════════════════════════════
//  buildCards
// ═════════════════════════════════════════════════════════════
void Shop::buildCards(const GameState& state) {
    m_cards.clear();

    int catIdx = static_cast<int>(m_activeCategory);
    const auto& ids = CATEGORIES[catIdx].ids;

    float cardW = m_w - m_scrollBarW - m_cardMargin * 2.f;
    float y     = m_y + m_tabH + m_cardMargin - m_scroll;

    for (auto id : ids) {
        UpgradeCard card;
        card.id         = id;
        card.bounds     = sf::FloatRect(
            { m_x + m_cardMargin, y },
            { cardW, m_cardH });
        card.affordable = state.canBuy(id);
        m_cards.push_back(card);
        y += m_cardH + m_cardMargin;
    }

    float totalH   = static_cast<float>(ids.size()) * (m_cardH + m_cardMargin);
    float visibleH = m_h - m_tabH;
    m_maxScroll    = std::max(0.f, totalH - visibleH);
    m_scroll       = clamp(m_scroll, 0.f, m_maxScroll);
}

// ═════════════════════════════════════════════════════════════
//  handleEvent
// ═════════════════════════════════════════════════════════════
bool Shop::handleEvent(const sf::Event& event, GameState& state) {
    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp(
                static_cast<float>(e->position.x),
                static_cast<float>(e->position.y));

            for (int i = 0; i < static_cast<int>(ShopCategory::CATEGORY_COUNT); i++) {
                if (tabBounds(i).contains(mp)) {
                    m_activeCategory = static_cast<ShopCategory>(i);
                    m_scroll         = 0.f;
                    buildCards(state);
                    return false;
                }
            }

            for (auto& card : m_cards) {
                if (card.bounds.contains(mp) && card.affordable) {
                    state.buy(card.id);
                    buildCards(state);
                    return true;
                }
            }
        }
    }

    if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        scrollBy(-e->delta * 30.f);
        buildCards(state);
    }

    return false;
}

// ═════════════════════════════════════════════════════════════
//  update
// ═════════════════════════════════════════════════════════════
void Shop::update(sf::Vector2f mousePos, const GameState& state) {
    buildCards(state);
    for (auto& card : m_cards)
        card.hovered = card.bounds.contains(mousePos);
}

void Shop::scrollBy(float delta) {
    m_scroll = clamp(m_scroll + delta, 0.f, m_maxScroll);
}

// ═════════════════════════════════════════════════════════════
//  draw
// ═════════════════════════════════════════════════════════════
void Shop::draw(sf::RenderTarget& target, const GameState& state) const {
    drawBackground(target);
    drawCategoryTabs(target, state);

    // Viewport clipping — gebruik werkelijke target-grootte
    auto  tSize = target.getSize();
    float tW    = static_cast<float>(tSize.x);
    float tH    = static_cast<float>(tSize.y);

    sf::View oldView = target.getView();

    float vpX = m_x / tW;
    float vpY = (m_y + m_tabH) / tH;
    float vpW = m_w / tW;
    float vpH = (m_h - m_tabH) / tH;

    sf::View cardView(sf::FloatRect(
        { m_x, m_y + m_tabH },
        { m_w, m_h - m_tabH }));
    cardView.setViewport(sf::FloatRect({ vpX, vpY }, { vpW, vpH }));
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
    int      count   = static_cast<int>(ShopCategory::CATEGORY_COUNT);
    unsigned tabFont = static_cast<unsigned>(std::round(14.f * m_scale));

    for (int i = 0; i < count; i++) {
        auto  bounds = tabBounds(i);
        bool  active = (m_activeCategory == static_cast<ShopCategory>(i));
        auto& accent = CAT_COLORS[i];

        sf::RectangleShape bg(sf::Vector2f{
            bounds.size.x, bounds.size.y });
        bg.setPosition(bounds.position);
        bg.setFillColor(active
            ? sf::Color(accent.r/5, accent.g/5, accent.b/5, 255)
            : sf::Color(16, 18, 32, 255));
        bg.setOutlineColor(active
            ? sf::Color(accent.r, accent.g, accent.b, 200)
            : sf::Color(40, 48, 80, 150));
        bg.setOutlineThickness(1.f);
        target.draw(bg);

        if (active) {
            sf::RectangleShape bar(sf::Vector2f{
                bounds.size.x - 4.f, 3.f });
            bar.setPosition({
                bounds.position.x + 2.f,
                bounds.position.y + bounds.size.y - 3.f });
            bar.setFillColor(accent);
            target.draw(bar);
        }

        sf::Text lbl(*m_font);
        lbl.setCharacterSize(tabFont);
        lbl.setString(CATEGORIES[i].label);
        lbl.setFillColor(active ? accent : sf::Color(130, 140, 170));
        if (active) lbl.setStyle(sf::Text::Bold);
        lbl.setPosition({
            bounds.position.x + bounds.size.x * 0.5f
                - lbl.getLocalBounds().size.x * 0.5f,
            bounds.position.y + m_tabH * 0.5f
                - static_cast<float>(tabFont) * 0.5f });
        target.draw(lbl);
    }

    // Scheidingslijn onder tabs
    sf::RectangleShape sep(sf::Vector2f{ m_w, 1.f });
    sep.setPosition({ m_x, m_y + m_tabH });
    sep.setFillColor(sf::Color(50, 60, 100, 200));
    target.draw(sep);
}

// ─────────────────────────────────────────────────────────────
//  drawCard
// ─────────────────────────────────────────────────────────────
void Shop::drawCard(sf::RenderTarget&  target,
                     const UpgradeCard& card,
                     const GameState&   state) const {
    int catIdx  = static_cast<int>(m_activeCategory);
    auto& accent = CAT_COLORS[catIdx];

    bool hov = card.hovered;
    bool aff = card.affordable;

    // Achtergrond
    sf::RectangleShape bg(sf::Vector2f{
        card.bounds.size.x, card.bounds.size.y });
    bg.setPosition(card.bounds.position);
    bg.setFillColor(hov && aff
        ? sf::Color(accent.r/6, accent.g/6, accent.b/6, 240)
        : sf::Color(18, 20, 36, 230));
    bg.setOutlineColor(aff
        ? sf::Color(accent.r, accent.g, accent.b,
                    hov ? 220 : 120)
        : sf::Color(40, 44, 70, 100));
    bg.setOutlineThickness(hov && aff ? 2.f : 1.f);
    target.draw(bg);

    unsigned fName = static_cast<unsigned>(std::round(15.f * m_scale));
    unsigned fDesc = static_cast<unsigned>(std::round(12.f * m_scale));
    unsigned fCost = static_cast<unsigned>(std::round(13.f * m_scale));
    unsigned fLvl  = static_cast<unsigned>(std::round(12.f * m_scale));

    float tx = card.bounds.position.x + m_cardPad;
    float ty = card.bounds.position.y + m_cardPad;

    int         idx = static_cast<int>(card.id);
    const auto& def = GameState::upgradeCatalog[idx];
    int         lv  = state.upgradeLevels[idx];

    // Naam
    sf::Text name(*m_font);
    name.setCharacterSize(fName);
    name.setString(def.name);
    name.setFillColor(aff ? sf::Color(230, 230, 255)
                          : sf::Color(110, 110, 140));
    name.setStyle(sf::Text::Bold);
    name.setPosition({ tx, ty });
    target.draw(name);

    // Level badge
    std::string lvStr = "Lv " + std::to_string(lv);
    sf::Text lvTxt(*m_font);
    lvTxt.setCharacterSize(fLvl);
    lvTxt.setString(lvStr);
    lvTxt.setFillColor(sf::Color(accent.r, accent.g, accent.b,
                                  aff ? 220 : 120));
    lvTxt.setPosition({
        card.bounds.position.x + card.bounds.size.x
            - m_cardPad - lvTxt.getLocalBounds().size.x - 4.f,
        ty + 2.f });
    target.draw(lvTxt);

    ty += fName + std::round(6.f * m_scale);

    // Beschrijving
    sf::Text desc(*m_font);
    desc.setCharacterSize(fDesc);
    desc.setString(def.description);
    desc.setFillColor(sf::Color(140, 150, 180));
    desc.setPosition({ tx, ty });
    target.draw(desc);

    ty += fDesc + std::round(6.f * m_scale);

    // Effect
    std::string fx = formatEffect(card.id, state);
    if (!fx.empty()) {
        sf::Text fxTxt(*m_font);
        fxTxt.setCharacterSize(fDesc);
        fxTxt.setString(fx);
        fxTxt.setFillColor(sf::Color(
            accent.r, accent.g, accent.b, aff ? 200 : 100));
        fxTxt.setPosition({ tx, ty });
        target.draw(fxTxt);
    }

    // Kosten (rechtsonder)
    double cost = state.costOf(card.id);
    std::ostringstream cs;
    cs << "$ " << std::fixed << std::setprecision(0) << cost;
    sf::Text costTxt(*m_font);
    costTxt.setCharacterSize(fCost);
    costTxt.setString(cs.str());
    costTxt.setStyle(sf::Text::Bold);
    costTxt.setFillColor(aff ? sf::Color(255, 215, 70)
                             : sf::Color(100, 90, 70));
    costTxt.setPosition({
        card.bounds.position.x + card.bounds.size.x
            - m_cardPad - costTxt.getLocalBounds().size.x - 4.f,
        card.bounds.position.y + card.bounds.size.y
            - m_cardPad - fCost });
    target.draw(costTxt);
}

// ─────────────────────────────────────────────────────────────
//  drawScrollBar
// ─────────────────────────────────────────────────────────────
void Shop::drawScrollBar(sf::RenderTarget& target) const {
    if (m_maxScroll <= 0.f) return;

    float visibleH = m_h - m_tabH;
    float totalH   = visibleH + m_maxScroll;
    float barH     = std::max(30.f, visibleH * (visibleH / totalH));
    float barY     = m_y + m_tabH +
                     (m_scroll / m_maxScroll) * (visibleH - barH);
    float barX     = m_x + m_w - m_scrollBarW;

    sf::RectangleShape track(sf::Vector2f{ m_scrollBarW, visibleH });
    track.setPosition({ barX, m_y + m_tabH });
    track.setFillColor(sf::Color(20, 24, 44));
    target.draw(track);

    sf::RectangleShape bar(sf::Vector2f{ m_scrollBarW - 2.f, barH });
    bar.setPosition({ barX + 1.f, barY });
    bar.setFillColor(sf::Color(80, 90, 140, 200));
    target.draw(bar);
}

// ═════════════════════════════════════════════════════════════
//  formatEffect
// ═════════════════════════════════════════════════════════════
std::string Shop::formatEffect(UpgradeID id,
                                 const GameState& state) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    int lv = state.upgradeLevels[static_cast<int>(id)];

    switch (id) {
        case UpgradeID::GUN_DAMAGE:
            ss << "Damage: " << state.gunDamage(); break;
        case UpgradeID::FIRE_RATE:
            ss << "Fire/sec: " << state.fireRatePerSec(); break;
        case UpgradeID::TURRET_COUNT:
            ss << "Turrets: " << state.turretCount(); break;
        case UpgradeID::CRIT_CHANCE:
            ss << "Crit: " << state.critChance() * 100.f << "%"; break;
        case UpgradeID::CRIT_MULT:
            ss << "Crit mult: " << state.critMult() << "x"; break;
        case UpgradeID::SPLIT_SHOT:
            ss << "Bullets: " << state.splitShot(); break;
        case UpgradeID::ORE_VALUE:
            ss << "Ore value: " << state.oreValueMult() << "x"; break;
        case UpgradeID::AUTO_COLLECT_RADIUS:
            ss << "Radius: +" << lv * 30 << "px"; break;
        case UpgradeID::ORE_LUCK:
            ss << "Luck: " << lv * 5 << "%"; break;
        case UpgradeID::ASTEROID_HP:
            ss << "HP: -" << lv * 10 << "%"; break;
        case UpgradeID::PLINKO_ROWS:
            ss << "Rows: " << state.plinkoRows(); break;
        case UpgradeID::PLINKO_MULT:
            ss << "Mult bonus: +" << lv * 10 << "%"; break;
        case UpgradeID::PLINKO_BALLS:
            ss << "Max balls: " << state.maxPlinkoBalls(); break;
        case UpgradeID::PLINKO_LUCK:
            ss << "High-slot luck: +" << lv * 5 << "%"; break;
        case UpgradeID::CREDIT_MULT:
            ss << "Credits: " << state.creditMult() << "x"; break;
        case UpgradeID::BULK_PROCESS:
            ss << "Bulk: " << state.bulkProcess() << "x"; break;
        case UpgradeID::AUTO_PLINKO:
            ss << (lv > 0 ? "Actief" : "Inactief"); break;
        case UpgradeID::WARP_DRIVE:
            ss << (state.levelOf(id) > 0 ? "Unlocked" : "Locked"); break;
        case UpgradeID::UNLOCK_BRONZE:
        case UpgradeID::UNLOCK_SILVER:
        case UpgradeID::UNLOCK_GOLD:
        case UpgradeID::UNLOCK_DIAMOND:
        case UpgradeID::UNLOCK_PLATINUM:
        case UpgradeID::UNLOCK_TITANIUM:
        case UpgradeID::UNLOCK_IRIDIUM:
            ss << (state.levelOf(id) > 0 ? "Unlocked" : "Locked");
            break;
        default: return "";
    }
    return ss.str();
}
