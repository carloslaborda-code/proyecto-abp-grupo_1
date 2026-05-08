#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

class AnimacionFrames {
public:
    AnimacionFrames() = default;

    bool cargarDesdeSecuencia(
        const std::string& prefijoRuta,
        const std::string& extension,
        int totalFrames,
        float duracionTotal,
        bool loop = true
    ) {
        prefijoRuta_ = prefijoRuta;
        extension_ = extension;
        totalFrames_ = totalFrames;
        duracionTotal_ = duracionTotal;
        loop_ = loop;
        tiempoActual_ = 0.f;
        frameActual_ = 0;
        terminada_ = false;
        texturaActual_ = sf::Texture();

        if (totalFrames_ <= 0) {
            return false;
        }

        return cargarFrame(0);
    }

    void actualizar(float dt) {
        if (totalFrames_ <= 0 || duracionTotal_ <= 0.f) {
            return;
        }

        if (!loop_ && terminada_) {
            return;
        }

        tiempoActual_ += dt;

        if (loop_) {
            while (tiempoActual_ >= duracionTotal_) {
                tiempoActual_ -= duracionTotal_;
            }
        } else {
            if (tiempoActual_ >= duracionTotal_) {
                tiempoActual_ = duracionTotal_;
                terminada_ = true;
            }
        }

        float progreso = tiempoActual_ / duracionTotal_;
        int nuevoFrame = static_cast<int>(progreso * totalFrames_);

        if (nuevoFrame >= totalFrames_) {
            nuevoFrame = totalFrames_ - 1;
        }

        nuevoFrame = std::clamp(nuevoFrame, 0, totalFrames_ - 1);

        if (nuevoFrame != frameActual_) {
            frameActual_ = nuevoFrame;
            cargarFrame(frameActual_);
        }
    }

    void dibujar(sf::RenderWindow& ventana) {
        if (totalFrames_ > 0) {
            ventana.draw(sprite_);
        }
    }

    void setPosition(float x, float y) {
        sprite_.setPosition(x, y);
    }

    void setScale(float x, float y) {
        sprite_.setScale(x, y);
    }

    void ajustarAlTamanoVentana(const sf::RenderWindow& ventana) {
        if (totalFrames_ <= 0) {
            return;
        }

        sf::Vector2u tamTextura = texturaActual_.getSize();
        sf::Vector2u tamVentana = ventana.getSize();

        if (tamTextura.x == 0 || tamTextura.y == 0) {
            return;
        }

        float escalaX = static_cast<float>(tamVentana.x) / static_cast<float>(tamTextura.x);
        float escalaY = static_cast<float>(tamVentana.y) / static_cast<float>(tamTextura.y);

        sprite_.setScale(escalaX, escalaY);
    }

    void fijarFrame(int indice) {
        if (indice >= 0 && indice < totalFrames_) {
            frameActual_ = indice;
            cargarFrame(frameActual_);
        }
    }

    void reiniciar() {
        tiempoActual_ = 0.f;
        frameActual_ = 0;
        terminada_ = false;
        cargarFrame(0);
    }

    bool terminada() const {
        return terminada_;
    }

    sf::Vector2u getTamanoFrame() const {
        if (totalFrames_ <= 0) {
            return sf::Vector2u(0u, 0u);
        }
        return texturaActual_.getSize();
    }

private:
    bool cargarFrame(int indice) {
        std::ostringstream ruta;
        ruta << prefijoRuta_
             << std::setw(3) << std::setfill('0') << indice
             << extension_;

        if (!texturaActual_.loadFromFile(ruta.str())) {
            std::cerr << "No se pudo cargar: " << ruta.str() << '\n';
            return false;
        }

        sprite_.setTexture(texturaActual_, true);
        return true;
    }

    sf::Texture texturaActual_;
    sf::Sprite sprite_;
    std::string prefijoRuta_;
    std::string extension_;

    int totalFrames_ = 0;
    int frameActual_ = 0;

    float duracionTotal_ = 0.f;
    float tiempoActual_ = 0.f;

    bool loop_ = true;
    bool terminada_ = false;
};
