#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <string>

struct MenuPausaButton {
    std::string label;
    sf::Color accent = sf::Color::White;
    bool visible = true;
};

class MenuPausa {
public:
    explicit MenuPausa(const sf::Font& font);

    void configure(const std::string& title,
                   const std::string& subtitle,
                   const std::array<MenuPausaButton, 3>& buttons,
                   const sf::Vector2f& panelSize = {440.f, 278.f});

    int buttonAt(const sf::Vector2f& point, const sf::Vector2u& windowSize) const;
    void draw(sf::RenderWindow& window) const;

private:
    sf::FloatRect panelRect(const sf::Vector2u& windowSize) const;
    static void centerTextInRect(sf::Text& text, const sf::FloatRect& rect, float yOffset = 0.f);

    const sf::Font& font_;
    std::string title_;
    std::string subtitle_;
    std::array<MenuPausaButton, 3> buttons_{};
    sf::Vector2f panelSize_;
};
