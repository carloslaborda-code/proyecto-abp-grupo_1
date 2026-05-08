#include "ui/HUD.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

std::array<const char*, 3> fontCandidates() {
    return {
        "resources/proto_ui/fuente_opciones.otf",
        "../resources/proto_ui/fuente_opciones.otf",
        "src/pro/BinaryAssault/resources/proto_ui/fuente_opciones.otf"
    };
}

struct DefenseCard {
    DefenseType type;
    const char* name;
    const char* detail;
    int cost;
    sf::Color color;
};

struct CardLayout {
    float panelX = 0.f;
    float panelY = 0.f;
    float cardWidth = 0.f;
    float cardHeight = 0.f;
    float gap = 0.f;
};

const std::array<const char*, 4>& absorptionNames() {
    static const std::array<const char*, 4> names{{
        "Pulso",
        "Turbo",
        "Blindaje",
        "Estatica",
    }};
    return names;
}

const std::array<DefenseCard, 4>& cards() {
    static const std::array<DefenseCard, 4> kCards{{
        {DefenseType::Firewall, "Firewall", "Disparo continuo", 44, sf::Color(255, 196, 87)},
        {DefenseType::EMP, "EMP", "Anula el carril", 76, sf::Color(120, 220, 255)},
        {DefenseType::AuxiliaryServer, "Servidor", "Genera energia", 70, sf::Color(160, 255, 170)},
        {DefenseType::SlowNode, "Slow Node", "Presion constante", 56, sf::Color(200, 170, 255)},
    }};
    return kCards;
}

CardLayout cardLayoutForSize(const sf::Vector2f& size) {
    const float bottomPanelX = 18.f;
    const float bottomPanelHeight = 108.f;
    const float bottomPanelY = size.y - bottomPanelHeight - 14.f;
    const float bottomPanelWidth = size.x - 36.f;
    const float gap = 12.f;
    const float usableWidth = bottomPanelWidth - 28.f - gap * 3.f;

    CardLayout layout;
    layout.panelX = bottomPanelX + 14.f;
    layout.panelY = bottomPanelY + 34.f;
    layout.cardWidth = usableWidth / 4.f;
    layout.cardHeight = 58.f;
    layout.gap = gap;
    return layout;
}

sf::Text makeText(const sf::Font& font,
                  const sf::String& text,
                  unsigned int size,
                  const sf::Color& color) {
    sf::Text drawable;
    drawable.setFont(font);
    drawable.setString(text);
    drawable.setCharacterSize(size);
    drawable.setFillColor(color);
    return drawable;
}

void setTextTopRight(sf::Text& text, const sf::Vector2f& position) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition(position.x - bounds.width - bounds.left, position.y);
}

void centerTextInRect(sf::Text& text, const sf::FloatRect& rect, float yOffset = 0.f) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition(rect.left + (rect.width - bounds.width) * 0.5f - bounds.left,
                     rect.top + (rect.height - bounds.height) * 0.5f - bounds.top + yOffset);
}

std::vector<std::string> splitWordsPreserveParagraphs(const std::string& text) {
    std::vector<std::string> words;
    std::string current;
    for (const char ch : text) {
        if (ch == '\n') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            words.push_back("\n");
        } else if (ch == ' ') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        words.push_back(current);
    }
    return words;
}

std::string wrapText(const sf::Font& font, const std::string& text, unsigned int size, float maxWidth) {
    sf::Text measure;
    measure.setFont(font);
    measure.setCharacterSize(size);

    std::string wrapped;
    std::string line;
    for (const auto& token : splitWordsPreserveParagraphs(text)) {
        if (token == "\n") {
            if (!wrapped.empty()) {
                wrapped.push_back('\n');
            }
            wrapped += line;
            wrapped.push_back('\n');
            line.clear();
            continue;
        }

        const std::string candidate = line.empty() ? token : line + " " + token;
        measure.setString(candidate);
        if (!line.empty() && measure.getLocalBounds().width > maxWidth) {
            if (!wrapped.empty() && wrapped.back() != '\n') {
                wrapped.push_back('\n');
            }
            wrapped += line;
            line = token;
        } else {
            line = candidate;
        }
    }

    if (!line.empty()) {
        if (!wrapped.empty() && wrapped.back() != '\n') {
            wrapped.push_back('\n');
        }
        wrapped += line;
    }

    return wrapped;
}

}  // namespace

HUD::HUD()
    : fontLoaded(loadFont()) {
}

HUDActionLayout HUD::actionLayout(const sf::Vector2u& windowSize, const sf::FloatRect& worldViewport) {
    HUDActionLayout layout;
    if (windowSize.x == 0 || windowSize.y == 0) {
        return layout;
    }

    const float sizeX = static_cast<float>(windowSize.x);
    const float sizeY = static_cast<float>(windowSize.y);
    const float topPanelY = 14.f;
    const float topPanelHeight = 88.f;
    const float bottomPanelHeight = 108.f;
    const float bottomPanelY = sizeY - bottomPanelHeight - 14.f;
    const float panelTop = topPanelY + topPanelHeight + 12.f;
    const float panelBottom = bottomPanelY - 12.f;
    const float worldRight = (worldViewport.left + worldViewport.width) * sizeX;
    const float rightMarginWidth = std::max(0.f, sizeX - worldRight);
    const float preferredPanelWidth = std::max(126.f, rightMarginWidth - 14.f);
    const float panelWidth = std::min(166.f, preferredPanelWidth);
    const float panelX = sizeX - panelWidth - 8.f;

    layout.valid = panelBottom > panelTop && panelWidth > 110.f;
    if (!layout.valid) {
        return layout;
    }

    layout.panelRect = {panelX, panelTop, panelWidth, panelBottom - panelTop};
    layout.repairRect = {panelX + 10.f, panelTop + 52.f, panelWidth - 20.f, 58.f};
    layout.boostRect = {panelX + 10.f, panelTop + 124.f, panelWidth - 20.f, 58.f};

    float abilityY = panelTop + 236.f;
    for (std::size_t i = 0; i < layout.absorptionRects.size(); ++i) {
        layout.absorptionRects[i] = {panelX + 10.f, abilityY, panelWidth - 20.f, 48.f};
        abilityY += 56.f;
    }

    return layout;
}

void HUD::handleResize(const sf::RenderWindow& window) {
    hudView = window.getDefaultView();
}

void HUD::draw(sf::RenderWindow& window, const HUDState& state) {
    handleResize(window);
    const sf::View previousView = window.getView();
    window.setView(hudView);

    const sf::Vector2f size = hudView.getSize();
    const float topPanelX = 18.f;
    const float topPanelY = 14.f;
    const float topPanelHeight = 88.f;
    const float topPanelWidth = size.x - 36.f;
    const float bottomPanelX = 18.f;
    const float bottomPanelHeight = 108.f;
    const float bottomPanelY = size.y - bottomPanelHeight - 14.f;
    const float bottomPanelWidth = size.x - 36.f;

    drawPanel(window, {topPanelX, topPanelY, topPanelWidth, topPanelHeight}, sf::Color(8, 16, 27, 214));
    drawPanel(window, {bottomPanelX, bottomPanelY, bottomPanelWidth, bottomPanelHeight}, sf::Color(8, 16, 27, 228));

    sf::RectangleShape divider({topPanelWidth - 28.f, 1.5f});
    divider.setPosition(topPanelX + 14.f, topPanelY + 40.f);
    divider.setFillColor(sf::Color(78, 140, 188, 180));
    window.draw(divider);

    if (fontLoaded) {
        sf::Text title = makeText(font, "BINARY ASSAULT", 24, sf::Color(230, 245, 255));
        title.setPosition(topPanelX + 16.f, topPanelY + 10.f);
        window.draw(title);

        sf::Text modeText = makeText(font,
                                     state.modeLabel + " | " + state.stageLabel,
                                     16,
                                     sf::Color(188, 220, 245));
        modeText.setPosition(topPanelX + 250.f, topPanelY + 16.f);
        window.draw(modeText);

        if (state.godMode) {
            sf::Text godModeText = makeText(font, "MODO DIOS", 16, sf::Color(255, 130, 130));
            godModeText.setPosition(topPanelX + topPanelWidth * 0.60f, topPanelY + 16.f);
            window.draw(godModeText);
        }

        sf::Text scoreText = makeText(font,
                                      "PTS " + std::to_string(state.score),
                                      18,
                                      sf::Color(255, 214, 109));
        setTextTopRight(scoreText, {size.x - 18.f, topPanelY + 14.f});
        window.draw(scoreText);

        drawText(window,
                 "E " + std::to_string(state.credits),
                 19,
                 {topPanelX + 18.f, topPanelY + 52.f},
                 sf::Color(255, 214, 109));

        drawText(window,
                 state.waveStatusLabel.empty()
                     ? "W " + std::to_string(state.wave) + "/" + std::to_string(state.totalWaves)
                     : state.waveStatusLabel,
                 19,
                 {topPanelX + 130.f, topPanelY + 52.f},
                 sf::Color(124, 214, 255));

        drawText(window,
                 "EN " + std::to_string(state.enemiesAlive),
                 19,
                 {topPanelX + 255.f, topPanelY + 52.f},
                 sf::Color(196, 216, 228));

        const std::string laneLabel =
            "C " + std::to_string(state.selectedLane + 1) + "/" + std::to_string(state.selectedLaneCount);
        const std::string defenseLabel = state.selectedDefensePlaced
                                             ? "Ocupada"
                                             : "Libre";
        drawText(window,
                 laneLabel + " | " + defenseLabel,
                 16,
                 {bottomPanelX + 18.f, bottomPanelY + 10.f},
                 sf::Color(224, 236, 244));

        const bool defenseLimitReached = state.maxDefenses > 0 && state.activeDefenses >= state.maxDefenses;
        drawText(window,
                 "Defensas " + std::to_string(state.activeDefenses) + "/" + std::to_string(state.maxDefenses),
                 16,
                 {bottomPanelX + 218.f, bottomPanelY + 10.f},
                 defenseLimitReached ? sf::Color(255, 196, 87) : sf::Color(188, 220, 245));
    }

    const float coreRatio = state.maxCoreStability > 0
                                ? static_cast<float>(state.coreStability) / static_cast<float>(state.maxCoreStability)
                                : 0.f;
    sf::RectangleShape coreBarBg({180.f, 10.f});
    coreBarBg.setPosition(size.x - 214.f, topPanelY + 56.f);
    coreBarBg.setFillColor(sf::Color(35, 48, 64));
    window.draw(coreBarBg);

    sf::RectangleShape coreBar({180.f * std::max(0.f, coreRatio), 10.f});
    coreBar.setPosition(size.x - 214.f, topPanelY + 56.f);
    coreBar.setFillColor(state.gameOver ? sf::Color(200, 55, 55) : sf::Color(85, 215, 130));
    window.draw(coreBar);

    const HUDActionLayout rightPanel = actionLayout(window.getSize(), previousView.getViewport());
    if (rightPanel.valid) {
        drawPanel(window, rightPanel.panelRect, sf::Color(8, 16, 27, 224));

        if (fontLoaded) {
            sf::Text systemsTitle = makeText(font, "SISTEMA", 19, sf::Color(230, 245, 255));
            centerTextInRect(systemsTitle,
                             {rightPanel.panelRect.left,
                              rightPanel.panelRect.top + 10.f,
                              rightPanel.panelRect.width,
                              24.f});
            window.draw(systemsTitle);

            sf::Text laneText =
                makeText(font,
                         "Carril " + std::to_string(state.selectedLane + 1),
                         14,
                         sf::Color(188, 220, 245));
            centerTextInRect(laneText,
                             {rightPanel.panelRect.left,
                              rightPanel.panelRect.top + 28.f,
                              rightPanel.panelRect.width,
                              18.f});
            window.draw(laneText);

            auto drawActionBox = [&](const sf::FloatRect& rect,
                                     const std::string& titleText,
                                     const std::string& detailText,
                                     bool available,
                                     const sf::Color& accent) {
                sf::RectangleShape box({rect.width, rect.height});
                box.setPosition(rect.left, rect.top);
                box.setFillColor(available ? sf::Color(18, 31, 46, 242) : sf::Color(15, 24, 36, 228));
                box.setOutlineThickness(2.f);
                box.setOutlineColor(available ? accent : sf::Color(72, 103, 132, 210));
                window.draw(box);

                sf::Text title = makeText(font, titleText, 16, sf::Color::White);
                centerTextInRect(title, {rect.left, rect.top + 6.f, rect.width, 18.f});
                window.draw(title);

                sf::Text detail = makeText(font, detailText, 13, available ? accent : sf::Color(188, 210, 230));
                centerTextInRect(detail, {rect.left + 6.f, rect.top + 26.f, rect.width - 12.f, 16.f});
                window.draw(detail);
            };

            drawActionBox(rightPanel.repairRect,
                          "Reparar",
                          state.repairBonusCharges > 0
                              ? "Carga gratis x" + std::to_string(state.repairBonusCharges)
                              : (state.repairAvailable ? std::to_string(state.repairCost) + " energia"
                                                       : "No disponible"),
                          state.repairAvailable,
                          sf::Color(125, 235, 150));

            const std::string boostDetail = !state.laneBoostAvailable
                                                ? std::to_string(state.laneBoostSeconds) + " s restantes"
                                                : (state.laneBoostBonusCharges > 0
                                                       ? "Carga gratis x" + std::to_string(state.laneBoostBonusCharges)
                                                       : "Lista por " + std::to_string(state.laneBoostCost) + " energia");
            drawActionBox(rightPanel.boostRect,
                          "Mejora",
                          boostDetail,
                          state.laneBoostAvailable,
                          sf::Color(120, 220, 255));

            sf::Text abilityHeader = makeText(font, "REABSORCION", 16, sf::Color(230, 245, 255));
            centerTextInRect(abilityHeader,
                             {rightPanel.panelRect.left,
                              rightPanel.panelRect.top + 202.f,
                              rightPanel.panelRect.width,
                              20.f});
            window.draw(abilityHeader);

            for (std::size_t i = 0; i < rightPanel.absorptionRects.size(); ++i) {
                const bool available = state.absorptionCharges[i] > 0;
                const sf::Color accent = available ? sf::Color(255, 196, 87) : sf::Color(72, 103, 132, 210);
                sf::RectangleShape box({rightPanel.absorptionRects[i].width, rightPanel.absorptionRects[i].height});
                box.setPosition(rightPanel.absorptionRects[i].left, rightPanel.absorptionRects[i].top);
                box.setFillColor(available ? sf::Color(18, 31, 46, 242) : sf::Color(15, 24, 36, 228));
                box.setOutlineThickness(2.f);
                box.setOutlineColor(accent);
                window.draw(box);

                sf::Text keyText = makeText(font, absorptionNames()[i], 14, sf::Color::White);
                keyText.setPosition(rightPanel.absorptionRects[i].left + 8.f,
                                    rightPanel.absorptionRects[i].top + 6.f);
                window.draw(keyText);

                const std::string detailText = available
                                                   ? "Carga x" + std::to_string(state.absorptionCharges[i])
                                                   : "Faltan " + std::to_string(state.absorptionRemaining[i]);
                sf::Text detail = makeText(font,
                                           detailText,
                                           12,
                                           available ? sf::Color(255, 214, 109) : sf::Color(188, 210, 230));
                detail.setPosition(rightPanel.absorptionRects[i].left + 8.f,
                                   rightPanel.absorptionRects[i].top + 26.f);
                window.draw(detail);
            }
        }
    }

    const CardLayout layout = cardLayoutForSize(size);

    for (std::size_t i = 0; i < cards().size(); ++i) {
        const auto& card = cards()[i];
        const float x = layout.panelX + i * (layout.cardWidth + layout.gap);
        const bool selected = card.type == state.selectedDefenseType;

        sf::RectangleShape rect({layout.cardWidth, layout.cardHeight});
        rect.setPosition(x, layout.panelY);
        rect.setFillColor(selected ? sf::Color(24, 45, 64, 245) : sf::Color(15, 27, 41, 235));
        rect.setOutlineThickness(selected ? 3.f : 2.f);
        rect.setOutlineColor(selected ? card.color : sf::Color(72, 103, 132, 210));
        window.draw(rect);

        sf::RectangleShape accent({layout.cardWidth, 6.f});
        accent.setPosition(x, layout.panelY);
        accent.setFillColor(card.color);
        window.draw(accent);

        if (fontLoaded) {
            drawText(window, card.name, 18, {x + 10.f, layout.panelY + 11.f}, sf::Color::White);
            drawText(window, card.detail, 14, {x + 10.f, layout.panelY + 33.f}, sf::Color(196, 216, 228));
            const int displayedCost = state.godMode ? 0 : card.cost;
            drawText(window,
                     std::to_string(displayedCost) + " E",
                     17,
                     {x + layout.cardWidth - 62.f, layout.panelY + 10.f},
                     state.godMode ? sf::Color(255, 130, 130) : card.color);
        }
    }

    if (fontLoaded && !state.bannerTitle.empty()) {
        sf::Color accent = sf::Color(120, 220, 255);
        if (state.victory) {
            accent = sf::Color(125, 235, 150);
        } else if (state.gameOver) {
            accent = sf::Color(255, 130, 130);
        }
        const sf::FloatRect resultPanel(size.x * 0.5f - 330.f, size.y * 0.5f - 108.f, 660.f, 216.f);
        drawPanel(window, resultPanel, sf::Color(5, 10, 18, 230));

        sf::Text title = makeText(font, state.bannerTitle, 30, accent);
        centerTextInRect(title, {resultPanel.left, resultPanel.top + 18.f, resultPanel.width, 44.f});
        window.draw(title);

        sf::Text subtitle = makeText(font,
                                     wrapText(font, state.bannerSubtitle, 20, resultPanel.width - 52.f),
                                     20,
                                     sf::Color::White);
        centerTextInRect(subtitle,
                         {resultPanel.left + 24.f, resultPanel.top + 70.f, resultPanel.width - 48.f, 64.f});
        window.draw(subtitle);

        const std::string progressLine =
            "Reabsorcion restante: " + std::to_string(state.absorptionRemaining[0]) + "/" +
            std::to_string(state.absorptionRemaining[1]) + "/" +
            std::to_string(state.absorptionRemaining[2]) + "/" +
            std::to_string(state.absorptionRemaining[3]);
        sf::Text footer = makeText(font,
                                   wrapText(font, progressLine, 17, resultPanel.width - 52.f),
                                   17,
                                   sf::Color(188, 210, 230));
        centerTextInRect(footer,
                         {resultPanel.left + 24.f, resultPanel.top + 144.f, resultPanel.width - 48.f, 42.f});
        window.draw(footer);
    }

    window.setView(previousView);
}

bool HUD::loadFont() {
    for (const char* path : fontCandidates()) {
        if (font.loadFromFile(path)) {
            return true;
        }
    }

    return false;
}

void HUD::drawPanel(sf::RenderWindow& window, const sf::FloatRect& rect, const sf::Color& fill) const {
    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition(rect.left, rect.top);
    shape.setFillColor(fill);
    shape.setOutlineThickness(2.f);
    shape.setOutlineColor(sf::Color(70, 132, 180, 210));
    window.draw(shape);
}

void HUD::drawText(sf::RenderWindow& window,
                   const sf::String& text,
                   unsigned int size,
                   const sf::Vector2f& position,
                   const sf::Color& color) const {
    sf::Text drawable;
    drawable.setFont(font);
    drawable.setString(text);
    drawable.setCharacterSize(size);
    drawable.setFillColor(color);
    drawable.setPosition(position);
    window.draw(drawable);
}
