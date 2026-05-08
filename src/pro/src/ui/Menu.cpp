#include "ui/Menu.hpp"

#include <algorithm>

Menu::Menu(sf::RenderWindow& ventana, sf::Font& fuente)
    : ventana_(ventana), fuente_(fuente), indiceSeleccionado_(0) {}

std::size_t Menu::getIndiceSeleccionado() const {
    return indiceSeleccionado_;
}

std::size_t Menu::seleccionarOpcionEn(const sf::Vector2f& punto) {
    for (std::size_t i = 0; i < opciones_.size(); ++i) {
        if (opciones_[i].getGlobalBounds().contains(punto)) {
            indiceSeleccionado_ = i;
            actualizarColores();
            return i;
        }
    }

    return opciones_.size();
}

bool Menu::tieneOpcionEn(const sf::Vector2f& punto) const {
    for (const auto& opcion : opciones_) {
        if (opcion.getGlobalBounds().contains(punto)) {
            return true;
        }
    }

    return false;
}

void Menu::dibujar() {
    for (const auto& texto : opciones_) {
        ventana_.draw(texto);
    }
}

void Menu::actualizar(float dt) {
    if (!animandoSeleccion_ || opciones_.empty()) return;

    tiempoAnimacionSeleccion_ += dt;

    float t = tiempoAnimacionSeleccion_ / duracionAnimacionSeleccion_;
    if (t >= 1.f) {
        t = 1.f;
        animandoSeleccion_ = false;
    }

    float escalaActual;
    if (t < 0.5f) {
        float subT = t / 0.5f;
        escalaActual = escalaHover_ + (escalaPresionado_ - escalaHover_) * subT;
    } else {
        float subT = (t - 0.5f) / 0.5f;
        escalaActual = escalaPresionado_ + (escalaHover_ - escalaPresionado_) * subT;
    }

    opciones_[indiceSeleccionado_].setScale(escalaActual, escalaActual);
}

void Menu::actualizarColores() {
    for (std::size_t i = 0; i < opciones_.size(); ++i) {
        if (i == indiceSeleccionado_) {
            opciones_[i].setFillColor(colorHover_);
            opciones_[i].setScale(escalaHover_, escalaHover_);
        } else {
            opciones_[i].setFillColor(colorNormal_);
            opciones_[i].setScale(1.f, 1.f);
        }
    }
}

sf::Text Menu::crearOpcion(
    const sf::String& contenido,
    unsigned int tamano,
    sf::Vector2f posicion
) {
    sf::Text texto(contenido, fuente_, tamano);
    texto.setPosition(posicion);
    texto.setFillColor(colorNormal_);
    return texto;
}

void Menu::establecerOpciones(const std::vector<std::string>& textos, unsigned int tamano) {
    opciones_.clear();

    for (const auto& texto : textos) {
        opciones_.push_back(crearOpcion(texto, tamano, {0.f, 0.f}));
    }

    indiceSeleccionado_ = 0;
    actualizarColores();
}

void Menu::posicionarCentradoEnPanel(
    sf::Vector2f centroPanel,
    float /*anchoPanel*/,
    float /*altoPanel*/,
    float separacionVertical
) {
    if (opciones_.empty()) return;

    float alturaTotal = 0.f;

    for (std::size_t i = 0; i < opciones_.size(); ++i) {
        sf::FloatRect bounds = opciones_[i].getLocalBounds();
        alturaTotal += bounds.height;

        if (i + 1 < opciones_.size()) {
            alturaTotal += separacionVertical;
        }
    }

    float yInicial = centroPanel.y - (alturaTotal / 2.f);
    float yActual = yInicial;

    for (auto& opcion : opciones_) {
        sf::FloatRect bounds = opcion.getLocalBounds();

        opcion.setOrigin(
            bounds.left + bounds.width / 2.f,
            bounds.top + bounds.height / 2.f
        );

        float x = centroPanel.x;
        float y = yActual + bounds.height / 2.f;

        opcion.setPosition(x, y);

        yActual += bounds.height + separacionVertical;
    }

    actualizarColores();
}

void Menu::animarSeleccion() {
    if (opciones_.empty()) return;

    animandoSeleccion_ = true;
    tiempoAnimacionSeleccion_ = 0.f;
}
