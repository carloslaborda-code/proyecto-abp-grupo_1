#pragma once

#include <array>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

struct StoryBeat {
    std::string phaseLabel;
    std::string title;
    std::array<std::string, 4> lines;
    std::string footer;
    sf::Color accent = sf::Color::White;
};

class StoryBriefingView {
public:
    explicit StoryBriefingView(const sf::Font& font);

    static sf::FloatRect panelRect(const sf::Vector2u& windowSize);
    void draw(sf::RenderWindow& window, const StoryBeat& beat, bool awaitingAcknowledgement) const;

private:
    std::vector<std::string> wrapTextLines(const std::string& source,
                                           unsigned int characterSize,
                                           float maxWidth) const;

    const sf::Font& font_;
};
