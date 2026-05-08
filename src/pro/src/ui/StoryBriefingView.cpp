#include "ui/StoryBriefingView.h"

#include <algorithm>
#include <sstream>

StoryBriefingView::StoryBriefingView(const sf::Font& font)
    : font_(font) {
}

sf::FloatRect StoryBriefingView::panelRect(const sf::Vector2u& windowSize) {
    const float panelWidth = std::min(660.f, static_cast<float>(windowSize.x) - 120.f);
    return {44.f, 108.f, panelWidth, 356.f};
}

void StoryBriefingView::draw(sf::RenderWindow& window,
                             const StoryBeat& beat,
                             bool awaitingAcknowledgement) const {
    sf::RectangleShape backdrop(
        {static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)});
    backdrop.setPosition(0.f, 0.f);
    backdrop.setFillColor(sf::Color(4, 8, 14, 170));
    window.draw(backdrop);

    const sf::FloatRect panelRectValue = panelRect(window.getSize());
    const float panelWidth = panelRectValue.width;
    const float panelHeight = panelRectValue.height;
    const sf::Vector2f panelPos(panelRectValue.left, panelRectValue.top);
    const float textWidth = panelWidth - 76.f;
    const std::vector<std::string> wrappedTitle = wrapTextLines(beat.title, 24, textWidth);

    sf::RectangleShape shadow({panelWidth, panelHeight});
    shadow.setPosition(panelPos.x + 8.f, panelPos.y + 8.f);
    shadow.setFillColor(sf::Color(2, 6, 12, 110));
    window.draw(shadow);

    sf::RectangleShape panel({panelWidth, panelHeight});
    panel.setPosition(panelPos);
    panel.setFillColor(sf::Color(6, 14, 24, 228));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(72, 124, 168, 220));
    window.draw(panel);

    sf::RectangleShape accentBar({panelWidth, 8.f});
    accentBar.setPosition(panelPos);
    accentBar.setFillColor(beat.accent);
    window.draw(accentBar);

    sf::RectangleShape tag({178.f, 24.f});
    tag.setPosition(panelPos.x + 18.f, panelPos.y + 22.f);
    tag.setFillColor(sf::Color(14, 28, 43, 255));
    tag.setOutlineThickness(1.5f);
    tag.setOutlineColor(beat.accent);
    window.draw(tag);

    sf::Text phaseLabel;
    phaseLabel.setFont(font_);
    phaseLabel.setString(beat.phaseLabel);
    phaseLabel.setCharacterSize(13);
    phaseLabel.setLetterSpacing(1.08f);
    phaseLabel.setFillColor(beat.accent);
    phaseLabel.setPosition(panelPos.x + 28.f, panelPos.y + 25.f);
    window.draw(phaseLabel);

    float titleY = panelPos.y + 56.f;
    for (const std::string& line : wrappedTitle) {
        sf::Text title;
        title.setFont(font_);
        title.setString(line);
        title.setCharacterSize(24);
        title.setFillColor(sf::Color::White);
        title.setPosition(panelPos.x + 20.f, titleY);
        window.draw(title);
        titleY += 28.f;
    }

    sf::RectangleShape divider({panelWidth - 40.f, 1.5f});
    divider.setPosition(panelPos.x + 20.f, titleY + 6.f);
    divider.setFillColor(sf::Color(beat.accent.r, beat.accent.g, beat.accent.b, 150));
    window.draw(divider);

    float lineY = titleY + 22.f;
    for (const std::string& line : beat.lines) {
        const std::vector<std::string> wrappedBody = wrapTextLines(line, 17, textWidth - 12.f);
        sf::CircleShape bullet(3.5f);
        bullet.setPosition(panelPos.x + 24.f, lineY + 10.f);
        bullet.setFillColor(beat.accent);
        window.draw(bullet);

        float wrappedLineY = lineY;
        for (const std::string& wrappedLine : wrappedBody) {
            sf::Text text;
            text.setFont(font_);
            text.setString(wrappedLine);
            text.setCharacterSize(17);
            text.setFillColor(sf::Color(222, 235, 245));
            text.setPosition(panelPos.x + 40.f, wrappedLineY);
            window.draw(text);
            wrappedLineY += 22.f;
        }

        lineY = wrappedLineY + 8.f;
    }

    sf::Text footer;
    footer.setFont(font_);
    footer.setString(!beat.footer.empty()
                         ? beat.footer
                         : (awaitingAcknowledgement
                                ? "Haz clic en esta transmision para continuar cuando termines de leer"
                                : "Preparacion en curso // Despliega defensas y fija el carril con clic derecho"));
    footer.setCharacterSize(14);
    footer.setFillColor(sf::Color(170, 198, 220));
    footer.setPosition(panelPos.x + 20.f, panelPos.y + panelHeight - 30.f);
    window.draw(footer);
}

std::vector<std::string> StoryBriefingView::wrapTextLines(const std::string& source,
                                                          unsigned int characterSize,
                                                          float maxWidth) const {
    std::vector<std::string> wrappedLines;
    std::istringstream stream(source);
    std::string word;
    std::string currentLine;
    sf::Text measure;
    measure.setFont(font_);
    measure.setCharacterSize(characterSize);

    while (stream >> word) {
        const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
        measure.setString(candidate);
        if (!currentLine.empty() && measure.getLocalBounds().width > maxWidth) {
            wrappedLines.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = candidate;
        }
    }

    if (!currentLine.empty()) {
        wrappedLines.push_back(currentLine);
    }

    if (wrappedLines.empty()) {
        wrappedLines.push_back(source);
    }

    return wrappedLines;
}
