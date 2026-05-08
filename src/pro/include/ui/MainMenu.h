#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include "ui/AnimacionFrames.hpp"
#include "ui/MenuPrincipal.hpp"

class MainMenu {
public:
    explicit MainMenu(sf::RenderWindow& gameWindow);

    bool run();

private:
    enum class Screen {
        Splash,
        Main,
        Settings,
        Credits,
        HowToPlay
    };

    enum class HowToSection {
        DefensesAndEnemies,
        Reabsorption,
        UpgradesAndRepair,
        GameModes
    };

    bool loadFont();
    bool loadMusic();
    void updateLayout();
    void handleMainSelection(bool& shouldStart, bool& shouldQuit);
    void applyWindowConfig();
    void render();
    void drawTitleTexts();
    void drawSplashTexts();
    void drawSettingsScreen();
    void drawCreditsScreen();
    void drawHowToScreen();
    void drawFooterHint(const sf::String& text, float yOffset = 0.f);

    sf::RenderWindow& window;
    sf::Font font;
    bool fontLoaded;
    sf::Music menuMusic;
    bool musicLoaded;
    Screen currentScreen;
    HowToSection currentHowToSection;
    MenuPrincipal menuPrincipal;
    AnimacionFrames fondoAnimado;
    AnimacionFrames panelInferiorAnimado;
    AnimacionFrames panelTituloAnimado;
    AnimacionFrames textoTituloAnimado;
    sf::Clock clock;
};
