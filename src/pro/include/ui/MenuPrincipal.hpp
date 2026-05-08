#pragma once
#include "ui/Menu.hpp"

class MenuPrincipal : public Menu {
public:
    MenuPrincipal(sf::RenderWindow& ventana, sf::Font& fuente);

    void actualizarLayoutSobrePanel(sf::Vector2f centroPanel, float anchoPanel, float altoPanel);
};