#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Menu {
public:
    Menu(sf::RenderWindow& ventana, sf::Font& fuente);
    virtual ~Menu() = default;

    std::size_t getIndiceSeleccionado() const;
    void dibujar();
    void actualizar(float dt);
    void animarSeleccion();
    std::size_t seleccionarOpcionEn(const sf::Vector2f& punto);
    bool tieneOpcionEn(const sf::Vector2f& punto) const;

protected:
    sf::Text crearOpcion(
        const sf::String& contenido,
        unsigned int tamano,
        sf::Vector2f posicion
    );

    void actualizarColores();
    void establecerOpciones(const std::vector<std::string>& textos, unsigned int tamano);
    void posicionarCentradoEnPanel(
        sf::Vector2f centroPanel,
        float anchoPanel,
        float altoPanel,
        float separacionVertical
    );

    sf::RenderWindow& ventana_;
    sf::Font& fuente_;
    std::vector<sf::Text> opciones_;
    std::size_t indiceSeleccionado_;

private:
    bool animandoSeleccion_ = false;
    float tiempoAnimacionSeleccion_ = 0.f;
    float duracionAnimacionSeleccion_ = 0.12f;

    float escalaHover_ = 1.0f;
    float escalaPresionado_ = 0.88f;

    sf::Color colorNormal_ = sf::Color::White;
    sf::Color colorHover_ = sf::Color(255, 221, 0);
};
