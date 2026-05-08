#include "ui/MenuPausa.h"

#include <string>
#include <vector>

namespace {

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

MenuPausa::MenuPausa(const sf::Font& font)
    : font_(font),
      panelSize_(440.f, 278.f) {
}

void MenuPausa::configure(const std::string& title,
                          const std::string& subtitle,
                          const std::array<MenuPausaButton, 3>& buttons,
                          const sf::Vector2f& panelSize) {
    title_ = title;
    subtitle_ = subtitle;
    buttons_ = buttons;
    panelSize_ = panelSize;
}

int MenuPausa::buttonAt(const sf::Vector2f& point, const sf::Vector2u& windowSize) const {
    const sf::FloatRect panel = panelRect(windowSize);
    float y = panel.top + 96.f;

    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        if (!buttons_[i].visible) {
            continue;
        }

        const sf::FloatRect rect(panel.left + 40.f, y, panel.width - 80.f, 42.f);
        if (rect.contains(point)) {
            return static_cast<int>(i);
        }
        y += 56.f;
    }

    return -1;
}

void MenuPausa::draw(sf::RenderWindow& window) const {
    const sf::Vector2u windowSize = window.getSize();
    const sf::FloatRect panel = panelRect(windowSize);

    sf::RectangleShape overlay(
        sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    overlay.setFillColor(sf::Color(4, 8, 14, 190));
    window.draw(overlay);

    sf::RectangleShape panelShape({panel.width, panel.height});
    panelShape.setPosition(panel.left, panel.top);
    panelShape.setFillColor(sf::Color(9, 18, 29, 238));
    panelShape.setOutlineThickness(3.f);
    panelShape.setOutlineColor(sf::Color(78, 140, 188, 230));
    window.draw(panelShape);

    sf::Text title;
    title.setFont(font_);
    title.setString(title_);
    title.setCharacterSize(30);
    title.setFillColor(sf::Color::White);
    centerTextInRect(title, {panel.left, panel.top + 18.f, panel.width, 36.f});
    window.draw(title);

    if (!subtitle_.empty()) {
        sf::Text subtitle;
        subtitle.setFont(font_);
        subtitle.setString(wrapText(font_, subtitle_, 18, panel.width - 52.f));
        subtitle.setCharacterSize(18);
        subtitle.setFillColor(sf::Color(196, 216, 228));
        centerTextInRect(subtitle, {panel.left + 24.f, panel.top + 52.f, panel.width - 48.f, 42.f});
        window.draw(subtitle);
    }

    float y = panel.top + 96.f;
    for (const auto& button : buttons_) {
        if (!button.visible) {
            continue;
        }

        sf::RectangleShape rect({panel.width - 80.f, 42.f});
        rect.setPosition(panel.left + 40.f, y);
        rect.setFillColor(sf::Color(18, 31, 46, 235));
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(button.accent);
        window.draw(rect);

        sf::Text text;
        text.setFont(font_);
        text.setString(button.label);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        centerTextInRect(text, {rect.getPosition().x, rect.getPosition().y, rect.getSize().x, rect.getSize().y});
        window.draw(text);

        y += 56.f;
    }
}

sf::FloatRect MenuPausa::panelRect(const sf::Vector2u& windowSize) const {
    return {static_cast<float>(windowSize.x) * 0.5f - panelSize_.x * 0.5f,
            static_cast<float>(windowSize.y) * 0.5f - panelSize_.y * 0.5f,
            panelSize_.x,
            panelSize_.y};
}

void MenuPausa::centerTextInRect(sf::Text& text, const sf::FloatRect& rect, float yOffset) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition(rect.left + (rect.width - bounds.width) * 0.5f - bounds.left,
                     rect.top + (rect.height - bounds.height) * 0.5f - bounds.top + yOffset);
}
