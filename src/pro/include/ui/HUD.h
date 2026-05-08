#pragma once

#include <array>
#include <string>

#include <SFML/Graphics.hpp>

#include "entities/Defense.h"

struct HUDState {
    int credits = 0;
    int coreStability = 0;
    int maxCoreStability = 0;
    int selectedLane = 0;
    int selectedLaneCount = 0;
    bool selectedDefensePlaced = false;
    DefenseType selectedDefenseType = DefenseType::Firewall;
    int activeDefenses = 0;
    int maxDefenses = 0;
    int wave = 1;
    int totalWaves = 1;
    std::string waveStatusLabel;
    int enemiesAlive = 0;
    int score = 0;
    std::string modeLabel;
    std::string stageLabel;
    bool repairAvailable = false;
    int repairCost = 0;
    int repairBonusCharges = 0;
    bool laneBoostAvailable = false;
    int laneBoostCost = 0;
    int laneBoostBonusCharges = 0;
    int laneBoostSeconds = 0;
    std::array<int, 4> absorptionCharges{};
    std::array<int, 4> absorptionRemaining{};
    std::string bannerTitle;
    std::string bannerSubtitle;
    bool gameOver = false;
    bool victory = false;
    bool godMode = false;
};

struct HUDActionLayout {
    bool valid = false;
    sf::FloatRect panelRect;
    sf::FloatRect repairRect;
    sf::FloatRect boostRect;
    std::array<sf::FloatRect, 4> absorptionRects{};
};

class HUD {
public:
    HUD();

    static HUDActionLayout actionLayout(const sf::Vector2u& windowSize, const sf::FloatRect& worldViewport);

    void handleResize(const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window, const HUDState& state);

private:
    bool loadFont();
    void drawPanel(sf::RenderWindow& window, const sf::FloatRect& rect, const sf::Color& fill) const;
    void drawText(sf::RenderWindow& window,
                  const sf::String& text,
                  unsigned int size,
                  const sf::Vector2f& position,
                  const sf::Color& color) const;

    sf::View hudView;
    sf::Font font;
    bool fontLoaded;
};
