#include "ui/MainMenu.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "core/Config.h"

namespace {

namespace fs = std::filesystem;

std::array<const char*, 3> fontCandidates() {
    return {
        "resources/proto_ui/fuente_opciones.otf",
        "../resources/proto_ui/fuente_opciones.otf",
        "src/pro/BinaryAssault/resources/proto_ui/fuente_opciones.otf"
    };
}

std::array<const char*, 4> musicCandidates() {
    return {
        "resources/audio/menu_pause_theme.mp3",
        "../resources/audio/menu_pause_theme.mp3",
        "src/pro/BinaryAssault/resources/audio/menu_pause_theme.mp3",
        "/home/clm91/Descargas/blendertimer-event-of-doom-395427.mp3"
    };
}

std::array<const char*, 3> baseResourceDirs() {
    return {
        "resources/proto_ui",
        "../resources/proto_ui",
        "src/pro/BinaryAssault/resources/proto_ui"
    };
}

std::string resolveSequencePrefix(const std::string& relativePrefix) {
    for (const char* base : baseResourceDirs()) {
        const fs::path candidate = fs::path(base) / relativePrefix;
        const fs::path firstFrame = candidate.string() + "000.png";
        if (fs::exists(firstFrame)) {
            return candidate.string();
        }
    }

    return {};
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

sf::FloatRect centeredPanelRect(const sf::RenderWindow& window, const sf::Vector2f& size, float yFactor = 0.56f) {
    return {static_cast<float>(window.getSize().x) * 0.5f - size.x * 0.5f,
            static_cast<float>(window.getSize().y) * yFactor - size.y * 0.5f,
            size.x,
            size.y};
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

void drawPanel(sf::RenderWindow& window, const sf::FloatRect& rect, const sf::Color& fill, const sf::Color& outline) {
    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition(rect.left, rect.top);
    shape.setFillColor(fill);
    shape.setOutlineThickness(2.f);
    shape.setOutlineColor(outline);
    window.draw(shape);
}

}  // namespace

MainMenu::MainMenu(sf::RenderWindow& gameWindow)
    : window(gameWindow),
      fontLoaded(loadFont()),
      musicLoaded(loadMusic()),
      currentScreen(Screen::Splash),
      currentHowToSection(HowToSection::DefensesAndEnemies),
      menuPrincipal(gameWindow, font) {
    const std::string fondoPrefix = resolveSequencePrefix("menu_fondo/fondo_");
    if (!fondoPrefix.empty()) {
        fondoAnimado.cargarDesdeSecuencia(fondoPrefix, ".png", 479, 479.0f / 60.0f, true);
    }

    const std::string panelInferiorPrefix = resolveSequencePrefix("menu_panel_inferior/botonmenu_");
    if (!panelInferiorPrefix.empty()) {
        panelInferiorAnimado.cargarDesdeSecuencia(panelInferiorPrefix, ".png", 479, 479.0f / 60.0f, true);
    }

    const std::string panelTituloPrefix = resolveSequencePrefix("menu_panel_titulo/botontitulo_");
    if (!panelTituloPrefix.empty()) {
        panelTituloAnimado.cargarDesdeSecuencia(panelTituloPrefix, ".png", 479, 479.0f / 60.0f, true);
    }

    const std::string textoTituloPrefix = resolveSequencePrefix("menu_texto_titulo/textotitulo_");
    if (!textoTituloPrefix.empty()) {
        textoTituloAnimado.cargarDesdeSecuencia(textoTituloPrefix, ".png", 479, 479.0f / 60.0f, true);
    }

    if (musicLoaded) {
        menuMusic.setLoop(true);
        menuMusic.setVolume(Config::getInstance().masterVolume());
        menuMusic.play();
    }

    fondoAnimado.ajustarAlTamanoVentana(window);
    updateLayout();
    clock.restart();
}

bool MainMenu::run() {
    while (window.isOpen()) {
        bool shouldStart = false;
        bool shouldQuit = false;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return false;
            }

            if (event.type == sf::Event::Resized) {
                fondoAnimado.ajustarAlTamanoVentana(window);
                updateLayout();
            }

            if (currentScreen == Screen::Splash) {
                if (event.type == sf::Event::MouseButtonPressed &&
                    event.mouseButton.button == sf::Mouse::Left) {
                    currentScreen = Screen::Main;
                }
                continue;
            }

            if (event.type == sf::Event::MouseMoved) {
                const sf::Vector2f point(static_cast<float>(event.mouseMove.x),
                                         static_cast<float>(event.mouseMove.y));
                if (currentScreen == Screen::Main) {
                    menuPrincipal.seleccionarOpcionEn(point);
                }
                continue;
            }

            if (event.type != sf::Event::MouseButtonPressed ||
                event.mouseButton.button != sf::Mouse::Left) {
                continue;
            }

            const sf::Vector2f point(static_cast<float>(event.mouseButton.x),
                                     static_cast<float>(event.mouseButton.y));

            if (currentScreen == Screen::Settings) {
                const sf::FloatRect panel = centeredPanelRect(window, {650.f, 286.f});
                const sf::FloatRect resolutionMinus(panel.left + 26.f, panel.top + 96.f, 48.f, 44.f);
                const sf::FloatRect resolutionPlus(panel.left + panel.width - 74.f, panel.top + 96.f, 48.f, 44.f);
                const sf::FloatRect volumeMinus(panel.left + 26.f, panel.top + 164.f, 48.f, 44.f);
                const sf::FloatRect volumePlus(panel.left + panel.width - 74.f, panel.top + 164.f, 48.f, 44.f);
                const sf::FloatRect backRect(panel.left + 160.f, panel.top + 224.f, panel.width - 320.f, 42.f);

                Config& config = Config::getInstance();
                if (resolutionMinus.contains(point)) {
                    config.cycleWindowPreset(-1);
                    applyWindowConfig();
                } else if (resolutionPlus.contains(point)) {
                    config.cycleWindowPreset(1);
                    applyWindowConfig();
                } else if (volumeMinus.contains(point)) {
                    config.changeMasterVolume(-5.f);
                    if (musicLoaded) {
                        menuMusic.setVolume(config.masterVolume());
                    }
                } else if (volumePlus.contains(point)) {
                    config.changeMasterVolume(5.f);
                    if (musicLoaded) {
                        menuMusic.setVolume(config.masterVolume());
                    }
                } else if (backRect.contains(point)) {
                    currentScreen = Screen::Main;
                    updateLayout();
                }
                continue;
            }

            if (currentScreen == Screen::Credits) {
                const sf::FloatRect panel = centeredPanelRect(window, {800.f, 560.f}, 0.55f);
                const sf::FloatRect backRect(panel.left + 220.f, panel.top + panel.height - 62.f, panel.width - 440.f, 42.f);
                if (backRect.contains(point)) {
                    currentScreen = Screen::Main;
                    updateLayout();
                }
                continue;
            }

            if (currentScreen == Screen::HowToPlay) {
                const sf::FloatRect panel = centeredPanelRect(window, {980.f, 680.f}, 0.52f);
                const float tabTop = panel.top + 76.f;
                const float tabWidth = (panel.width - 68.f) / 4.f;
                for (int i = 0; i < 4; ++i) {
                    const sf::FloatRect tabRect(panel.left + 24.f + i * (tabWidth + 6.f), tabTop, tabWidth, 46.f);
                    if (tabRect.contains(point)) {
                        currentHowToSection = static_cast<HowToSection>(i);
                        continue;
                    }
                }
                const sf::FloatRect backRect(panel.left + 280.f, panel.top + panel.height - 62.f, panel.width - 560.f, 42.f);
                if (backRect.contains(point)) {
                    currentScreen = Screen::Main;
                    updateLayout();
                }
                continue;
            }

            if (currentScreen == Screen::Main) {
                if (menuPrincipal.tieneOpcionEn(point)) {
                    menuPrincipal.seleccionarOpcionEn(point);
                    menuPrincipal.animarSeleccion();
                    handleMainSelection(shouldStart, shouldQuit);
                    continue;
                }
            }
        }

        if (shouldStart) {
            if (musicLoaded) {
                menuMusic.stop();
            }
            return true;
        }

        if (shouldQuit) {
            if (musicLoaded) {
                menuMusic.stop();
            }
            window.close();
            return false;
        }

        const float dt = clock.restart().asSeconds();
        fondoAnimado.actualizar(dt);
        panelInferiorAnimado.actualizar(dt);
        panelTituloAnimado.actualizar(dt);
        textoTituloAnimado.actualizar(dt);
        menuPrincipal.actualizar(dt);

        render();
    }

    return false;
}

bool MainMenu::loadFont() {
    for (const char* path : fontCandidates()) {
        if (font.loadFromFile(path)) {
            return true;
        }
    }

    return false;
}

bool MainMenu::loadMusic() {
    for (const char* path : musicCandidates()) {
        if (menuMusic.openFromFile(path)) {
            return true;
        }
    }

    return false;
}

void MainMenu::updateLayout() {
    const sf::Vector2u winSize = window.getSize();

    const sf::Vector2u tamPanelInferior = panelInferiorAnimado.getTamanoFrame();
    const sf::Vector2u tamPanelTitulo = panelTituloAnimado.getTamanoFrame();
    const sf::Vector2u tamTextoTitulo = textoTituloAnimado.getTamanoFrame();

    const float panelInferiorWidth = tamPanelInferior.x > 0 ? static_cast<float>(tamPanelInferior.x) : 720.f;
    const float panelInferiorHeight = tamPanelInferior.y > 0 ? static_cast<float>(tamPanelInferior.y) : 230.f;
    const float panelTituloWidth = tamPanelTitulo.x > 0 ? static_cast<float>(tamPanelTitulo.x) : 520.f;
    const float textoTituloWidth = tamTextoTitulo.x > 0 ? static_cast<float>(tamTextoTitulo.x) : 360.f;

    const float xPanelTitulo =
        (static_cast<float>(winSize.x) - panelTituloWidth) / 2.f;
    const float yPanelTitulo = 60.f;
    panelTituloAnimado.setPosition(xPanelTitulo, yPanelTitulo);

    const float xTextoTitulo =
        (static_cast<float>(winSize.x) - textoTituloWidth) / 2.f;
    textoTituloAnimado.setPosition(xTextoTitulo, yPanelTitulo);

    const float xPanelInferior =
        (static_cast<float>(winSize.x) - panelInferiorWidth) / 2.f;
    const float yPanelInferior =
        static_cast<float>(winSize.y) - panelInferiorHeight - 55.f;
    panelInferiorAnimado.setPosition(xPanelInferior, yPanelInferior);

    const sf::Vector2f centroPanel(
        xPanelInferior + panelInferiorWidth * 0.50f,
        yPanelInferior + panelInferiorHeight / 2.f - 10.f);

    menuPrincipal.actualizarLayoutSobrePanel(
        centroPanel,
        panelInferiorWidth * 0.72f,
        panelInferiorHeight);
}

void MainMenu::handleMainSelection(bool& shouldStart, bool& shouldQuit) {
    Config& config = Config::getInstance();
    switch (menuPrincipal.getIndiceSeleccionado()) {
        case 0:
            config.setSelectedMode(GameMode::Story);
            shouldStart = true;
            break;
        case 1:
            config.setSelectedMode(GameMode::Challenge);
            shouldStart = true;
            break;
        case 2:
            config.setSelectedMode(GameMode::Infinite);
            shouldStart = true;
            break;
        case 3:
            currentHowToSection = HowToSection::DefensesAndEnemies;
            currentScreen = Screen::HowToPlay;
            break;
        case 4:
            currentScreen = Screen::Credits;
            break;
        case 5:
            shouldQuit = true;
            break;
        default:
            break;
    }
}

void MainMenu::applyWindowConfig() {
    Config& config = Config::getInstance();
    window.create(sf::VideoMode(config.windowWidth(), config.windowHeight()), config.windowTitle());
    window.setFramerateLimit(config.targetFps());
    fondoAnimado.ajustarAlTamanoVentana(window);
    updateLayout();
}

void MainMenu::render() {
    window.clear(sf::Color::Black);
    fondoAnimado.dibujar(window);
    if (currentScreen != Screen::Splash) {
        sf::RectangleShape dimmer({static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)});
        dimmer.setFillColor(sf::Color(3, 7, 12, 175));
        window.draw(dimmer);
    }
    panelTituloAnimado.dibujar(window);
    textoTituloAnimado.dibujar(window);

    if (currentScreen != Screen::Splash) {
        panelInferiorAnimado.dibujar(window);
    }

    drawTitleTexts();

    if (currentScreen == Screen::Splash) {
        drawSplashTexts();
    } else if (currentScreen == Screen::Main) {
        menuPrincipal.dibujar();
        drawFooterHint("Desliza el raton y haz clic para seleccionar");
    } else if (currentScreen == Screen::Settings) {
        drawSettingsScreen();
    } else if (currentScreen == Screen::Credits) {
        drawCreditsScreen();
    } else if (currentScreen == Screen::HowToPlay) {
        drawHowToScreen();
    }

    window.display();
}

void MainMenu::drawTitleTexts() {
    if (!fontLoaded || currentScreen == Screen::Splash) {
        return;
    }

    const sf::Vector2u winSize = window.getSize();
    const sf::Vector2u tamPanelInferior = panelInferiorAnimado.getTamanoFrame();
    const sf::Vector2u tamPanelTitulo = panelTituloAnimado.getTamanoFrame();
    const float panelInferiorHeight = tamPanelInferior.y > 0 ? static_cast<float>(tamPanelInferior.y) : 230.f;
    const float panelInferiorY = static_cast<float>(winSize.y) - panelInferiorHeight - 55.f;
    const float panelTituloY = 60.f;
    const float panelTituloHeight = tamPanelTitulo.y > 0 ? static_cast<float>(tamPanelTitulo.y) : 120.f;
    const float subtitleY = panelTituloY + panelTituloHeight +
                            (panelInferiorY - (panelTituloY + panelTituloHeight)) * 0.5f - 12.f;

    sf::Text subtitle = makeText(font, "Defiende el nucleo. Reconfigura el sistema. Colapsa la amenaza.", 20,
                                 sf::Color(213, 230, 242));
    const sf::FloatRect bounds = subtitle.getLocalBounds();
    subtitle.setPosition(static_cast<float>(winSize.x) * 0.5f - bounds.width * 0.5f,
                         subtitleY);
    window.draw(subtitle);
}

void MainMenu::drawSplashTexts() {
    if (!fontLoaded) {
        return;
    }

    sf::RectangleShape panel({620.f, 130.f});
    panel.setOrigin(panel.getSize() * 0.5f);
    panel.setPosition(static_cast<float>(window.getSize().x) * 0.5f,
                      static_cast<float>(window.getSize().y) * 0.75f);
    panel.setFillColor(sf::Color(6, 14, 22, 210));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(90, 160, 214, 220));
    window.draw(panel);

    sf::Text title = makeText(font, "HAZ CLIC PARA ENTRAR", 34, sf::Color::White);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(panel.getPosition().x - titleBounds.width * 0.5f,
                      panel.getPosition().y - 26.f);
    window.draw(title);

    sf::Text subtitle = makeText(font, "Kyro Team presenta", 22,
                                 sf::Color(180, 214, 240));
    sf::FloatRect subtitleBounds = subtitle.getLocalBounds();
    subtitle.setPosition(panel.getPosition().x - subtitleBounds.width * 0.5f,
                         panel.getPosition().y + 20.f);
    window.draw(subtitle);
}

void MainMenu::drawSettingsScreen() {
    if (!fontLoaded) {
        return;
    }

    const sf::FloatRect panel = centeredPanelRect(window, {650.f, 286.f});
    sf::RectangleShape rect({panel.width, panel.height});
    rect.setPosition(panel.left, panel.top);
    rect.setFillColor(sf::Color(7, 16, 28, 226));
    rect.setOutlineThickness(2.f);
    rect.setOutlineColor(sf::Color(90, 160, 214, 220));
    window.draw(rect);

    const Config& config = Config::getInstance();
    sf::Text title = makeText(font, "AJUSTES DEL SISTEMA", 28, sf::Color::White);
    title.setPosition(panel.left + 28.f, panel.top + 22.f);
    window.draw(title);

    const std::array<sf::FloatRect, 4> controls{{
        {panel.left + 26.f, panel.top + 96.f, 48.f, 44.f},
        {panel.left + panel.width - 74.f, panel.top + 96.f, 48.f, 44.f},
        {panel.left + 26.f, panel.top + 164.f, 48.f, 44.f},
        {panel.left + panel.width - 74.f, panel.top + 164.f, 48.f, 44.f},
    }};

    const std::array<const char*, 4> labels{{"-", "+", "-", "+"}};
    for (std::size_t i = 0; i < controls.size(); ++i) {
        sf::RectangleShape button({controls[i].width, controls[i].height});
        button.setPosition(controls[i].left, controls[i].top);
        button.setFillColor(sf::Color(18, 31, 46, 235));
        button.setOutlineThickness(2.f);
        button.setOutlineColor(sf::Color(120, 220, 255, 210));
        window.draw(button);

        sf::Text symbol = makeText(font, labels[i], 28, sf::Color::White);
        const sf::FloatRect bounds = symbol.getLocalBounds();
        symbol.setPosition(controls[i].left + controls[i].width * 0.5f - bounds.width * 0.5f,
                           controls[i].top + 2.f);
        window.draw(symbol);
    }

    sf::Text resolutionLabel = makeText(font, "Resolucion", 22, sf::Color(196, 222, 241));
    resolutionLabel.setPosition(panel.left + 98.f, panel.top + 98.f);
    window.draw(resolutionLabel);
    sf::Text resolutionValue = makeText(font, config.windowPresetLabel(), 24, sf::Color::White);
    resolutionValue.setPosition(panel.left + 280.f, panel.top + 96.f);
    window.draw(resolutionValue);

    sf::Text volumeLabel = makeText(font, "Volumen maestro", 22, sf::Color(196, 222, 241));
    volumeLabel.setPosition(panel.left + 98.f, panel.top + 166.f);
    window.draw(volumeLabel);
    sf::Text volumeValue =
        makeText(font, std::to_string(static_cast<int>(config.masterVolume())) + "%", 24, sf::Color::White);
    volumeValue.setPosition(panel.left + 280.f, panel.top + 164.f);
    window.draw(volumeValue);

    sf::FloatRect backRect(panel.left + 160.f, panel.top + 224.f, panel.width - 320.f, 42.f);
    sf::RectangleShape back({backRect.width, backRect.height});
    back.setPosition(backRect.left, backRect.top);
    back.setFillColor(sf::Color(18, 31, 46, 235));
    back.setOutlineThickness(2.f);
    back.setOutlineColor(sf::Color(255, 196, 87, 210));
    window.draw(back);
    sf::Text backText = makeText(font, "Volver", 22, sf::Color::White);
    const sf::FloatRect backBounds = backText.getLocalBounds();
    backText.setPosition(backRect.left + backRect.width * 0.5f - backBounds.width * 0.5f,
                         backRect.top + 7.f);
    window.draw(backText);

    drawFooterHint("Usa los botones laterales para cambiar valores");
}

void MainMenu::drawCreditsScreen() {
    if (!fontLoaded) {
        return;
    }

    const sf::FloatRect panel = centeredPanelRect(window, {800.f, 560.f}, 0.55f);
    drawPanel(window, panel, sf::Color(6, 13, 22, 242), sf::Color(120, 220, 255, 220));

    sf::Text title = makeText(font, "CREDITOS", 30, sf::Color::White);
    title.setPosition(panel.left + 28.f, panel.top + 22.f);
    window.draw(title);

    sf::Text project = makeText(font, "Binary Assault // Proyecto ABP", 20, sf::Color(205, 225, 240));
    project.setPosition(panel.left + 28.f, panel.top + 70.f);
    window.draw(project);

    sf::Text team = makeText(font, "Equipo: Kyro Team", 28, sf::Color(255, 196, 87));
    team.setPosition(panel.left + 28.f, panel.top + 102.f);
    window.draw(team);

    const std::string introText =
        "En una ciudad sostenida por un nucleo digital central, una oleada de robots fuera de control "
        "ha roto los protocolos de seguridad. Tu escuadron tecnico debe desplegar defensas, mantener "
        "la energia del sistema y contener la crisis antes de que la red colapse.";
    sf::Text intro = makeText(font, wrapText(font, introText, 18, panel.width - 56.f), 18, sf::Color(218, 232, 242));
    intro.setPosition(panel.left + 28.f, panel.top + 148.f);
    window.draw(intro);

    sf::Text membersTitle = makeText(font, "Integrantes", 24, sf::Color::White);
    membersTitle.setPosition(panel.left + 28.f, panel.top + 264.f);
    window.draw(membersTitle);

    const std::string membersText =
        "- Carlos Laborda Marinez\n"
        "- Fidel Mas Saez\n"
        "- Teresa Cediel Campillo\n"
        "- Niki Gonzalo Duenas Vazquez\n"
        "- Cesar David Parra Paternina\n"
        "- Pablo Castell ALmodovar";
    sf::Text members = makeText(font, membersText, 21, sf::Color(210, 226, 239));
    members.setPosition(panel.left + 36.f, panel.top + 304.f);
    window.draw(members);

    sf::FloatRect backRect(panel.left + 220.f, panel.top + panel.height - 62.f, panel.width - 440.f, 42.f);
    sf::RectangleShape back({backRect.width, backRect.height});
    back.setPosition(backRect.left, backRect.top);
    back.setFillColor(sf::Color(18, 31, 46, 235));
    back.setOutlineThickness(2.f);
    back.setOutlineColor(sf::Color(255, 196, 87, 210));
    window.draw(back);
    sf::Text backText = makeText(font, "Volver", 22, sf::Color::White);
    const sf::FloatRect backBounds = backText.getLocalBounds();
    backText.setPosition(backRect.left + backRect.width * 0.5f - backBounds.width * 0.5f,
                         backRect.top + 7.f);
    window.draw(backText);
}

void MainMenu::drawHowToScreen() {
    if (!fontLoaded) {
        return;
    }

    const sf::FloatRect panel = centeredPanelRect(window, {980.f, 680.f}, 0.52f);
    drawPanel(window, panel, sf::Color(6, 13, 22, 244), sf::Color(120, 220, 255, 220));

    sf::Text title = makeText(font, "COMO JUGAR", 30, sf::Color::White);
    title.setPosition(panel.left + 28.f, panel.top + 22.f);
    window.draw(title);

    const std::array<std::string, 4> tabLabels{{
        "Defensas y enemigos",
        "Reabsorcion",
        "Mejoras y reparacion",
        "Modos de juego",
    }};
    const float tabTop = panel.top + 76.f;
    const float tabWidth = (panel.width - 68.f) / 4.f;
    for (std::size_t i = 0; i < tabLabels.size(); ++i) {
        const bool active = static_cast<int>(currentHowToSection) == static_cast<int>(i);
        const sf::FloatRect tabRect(panel.left + 24.f + static_cast<float>(i) * (tabWidth + 6.f),
                                    tabTop,
                                    tabWidth,
                                    46.f);
        drawPanel(window,
                  tabRect,
                  active ? sf::Color(21, 38, 58, 244) : sf::Color(13, 23, 35, 232),
                  active ? sf::Color(255, 196, 87, 220) : sf::Color(78, 140, 188, 190));
        sf::Text tabText = makeText(font, wrapText(font, tabLabels[i], 17, tabRect.width - 16.f), 17, sf::Color::White);
        const sf::FloatRect tabBounds = tabText.getLocalBounds();
        tabText.setPosition(tabRect.left + (tabRect.width - tabBounds.width) * 0.5f - tabBounds.left,
                            tabRect.top + (tabRect.height - tabBounds.height) * 0.5f - tabBounds.top - 1.f);
        window.draw(tabText);
    }

    const sf::FloatRect contentRect(panel.left + 24.f, panel.top + 136.f, panel.width - 48.f, panel.height - 226.f);
    drawPanel(window, contentRect, sf::Color(10, 20, 32, 236), sf::Color(78, 140, 188, 180));

    std::string sectionTitle;
    std::string sectionBody;
    switch (currentHowToSection) {
        case HowToSection::DefensesAndEnemies:
            sectionTitle = "Defensas y enemigos";
            break;
        case HowToSection::Reabsorption:
            sectionTitle = "Reabsorcion";
            sectionBody =
                "Las reabsorciones se cargan derrotando enemigos y se activan desde el HUD.\n\n"
                "Pulso: dano directo en el carril actual.\n"
                "Turbo: mejora prolongada del carril activo.\n"
                "Blindaje: cura defensas y puede recuperar estabilidad del nucleo.\n"
                "Estatica: ralentizacion global temporal.\n\n"
                "Consejo: no gastes una carga por rutina. Guardarla para una oleada complicada suele dar mas valor.";
            break;
        case HowToSection::UpgradesAndRepair:
            sectionTitle = "Mejoras y reparacion";
            sectionBody =
                "La energia llega por tiempo, enemigos, servidores y creditos entre oleadas.\n\n"
                "Reparacion:\n"
                "Si el modo lo permite, puedes curar el nucleo desde el HUD gastando energia.\n\n"
                "Mejora de carril:\n"
                "Potencia disparo, proyectiles y produccion de energia del carril seleccionado.\n\n"
                "Consejo: repara cuando realmente necesites margen y usa la mejora donde vayas a concentrar la defensa.";
            break;
        case HowToSection::GameModes:
            sectionTitle = "Modos de juego";
            sectionBody =
                "Historia:\n"
                "Progresion por etapas con dificultad creciente y margen para aprender.\n\n"
                "Desafio:\n"
                "Modo duro y tecnico, con pocos errores permitidos y presion alta.\n\n"
                "Infinito:\n"
                "Sectores sucesivos con escalada continua de densidad, mezcla enemiga y economia.";
                
            break;
    }

    sf::Text sectionTitleText = makeText(font, sectionTitle, 24, sf::Color(255, 196, 87));
    sectionTitleText.setPosition(contentRect.left + 18.f, contentRect.top + 16.f);
    window.draw(sectionTitleText);

    if (currentHowToSection == HowToSection::DefensesAndEnemies) {
        const std::string introText =
            "Objetivo: protege el nucleo y supera las oleadas.\n"
            "Controles: clic izq. carta = seleccionar, clic izq. casilla = construir, clic der. hueco = elegir carril, clic der. defensa = vender.";
        sf::Text intro = makeText(font,
                                  wrapText(font, introText, 18, contentRect.width - 36.f),
                                  18,
                                  sf::Color(214, 229, 240));
        intro.setPosition(contentRect.left + 18.f, contentRect.top + 56.f);
        window.draw(intro);

        const sf::FloatRect leftBox(contentRect.left + 18.f, contentRect.top + 134.f, contentRect.width * 0.47f, 200.f);
        const sf::FloatRect rightBox(contentRect.left + contentRect.width * 0.51f, contentRect.top + 134.f, contentRect.width * 0.47f - 18.f, 200.f);
        drawPanel(window, leftBox, sf::Color(13, 24, 38, 232), sf::Color(78, 140, 188, 180));
        drawPanel(window, rightBox, sf::Color(13, 24, 38, 232), sf::Color(78, 140, 188, 180));

        sf::Text defenseTitle = makeText(font, "Defensas", 22, sf::Color(255, 196, 87));
        defenseTitle.setPosition(leftBox.left + 14.f, leftBox.top + 12.f);
        window.draw(defenseTitle);

        const std::string defenseBody =
            "Firewall: dano continuo.\n"
            "EMP: elimina en area y se consume.\n"
            "Servidor: genera energia.\n"
            "Slow Node: ralentiza enemigos.\n\n"
            "Venta: una defensa colocada puede venderse con clic derecho y devuelve la mitad de su coste.";
        sf::Text defenseText = makeText(font,
                                        wrapText(font, defenseBody, 18, leftBox.width - 28.f),
                                        18,
                                        sf::Color(214, 229, 240));
        defenseText.setPosition(leftBox.left + 14.f, leftBox.top + 48.f);
        window.draw(defenseText);

        sf::Text enemyTitle = makeText(font, "Enemigos", 22, sf::Color(255, 196, 87));
        enemyTitle.setPosition(rightBox.left + 14.f, rightBox.top + 12.f);
        window.draw(enemyTitle);

        const std::string enemyBody =
            "Standard: equilibrado.\n"
            "Fast: presiona por velocidad.\n"
            "Heavy: aguanta mucho dano.\n"
            "Adaptive: cambia de carril si detecta uno mas debil.";
        sf::Text enemyText = makeText(font,
                                      wrapText(font, enemyBody, 18, rightBox.width - 28.f),
                                      18,
                                      sf::Color(214, 229, 240));
        enemyText.setPosition(rightBox.left + 14.f, rightBox.top + 48.f);
        window.draw(enemyText);
    } else {
        sf::Text bodyText = makeText(font,
                                     wrapText(font, sectionBody, 20, contentRect.width - 36.f),
                                     20,
                                     sf::Color(214, 229, 240));
        bodyText.setPosition(contentRect.left + 18.f, contentRect.top + 58.f);
        window.draw(bodyText);
    }

    sf::FloatRect backRect(panel.left + 280.f, panel.top + panel.height - 62.f, panel.width - 560.f, 42.f);
    sf::RectangleShape back({backRect.width, backRect.height});
    back.setPosition(backRect.left, backRect.top);
    back.setFillColor(sf::Color(18, 31, 46, 235));
    back.setOutlineThickness(2.f);
    back.setOutlineColor(sf::Color(255, 196, 87, 210));
    window.draw(back);

    sf::Text backText = makeText(font, "Volver", 22, sf::Color::White);
    const sf::FloatRect backBounds = backText.getLocalBounds();
    backText.setPosition(backRect.left + backRect.width * 0.5f - backBounds.width * 0.5f,
                         backRect.top + 7.f);
    window.draw(backText);

}

void MainMenu::drawFooterHint(const sf::String& text, float yOffset) {
    if (!fontLoaded) {
        return;
    }

    sf::Text hint = makeText(font, text, 18, sf::Color(188, 210, 230));
    const sf::FloatRect bounds = hint.getLocalBounds();
    hint.setPosition(static_cast<float>(window.getSize().x) * 0.5f - bounds.width * 0.5f,
                     static_cast<float>(window.getSize().y) - 42.f - yOffset);
    window.draw(hint);
}
