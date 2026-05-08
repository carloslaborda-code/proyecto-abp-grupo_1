#include "ui/MenuPrincipal.hpp"

MenuPrincipal::MenuPrincipal(sf::RenderWindow& ventana, sf::Font& fuente)
    : Menu(ventana, fuente)
{
    establecerOpciones({
        "MODO HISTORIA",
        "MODO DESAFIO",
        "MODO INFINITO",
        "COMO JUGAR",
        "CREDITOS",
        "SALIR"
    }, 32);
}

void MenuPrincipal::actualizarLayoutSobrePanel(sf::Vector2f centroPanel, float anchoPanel, float altoPanel) {
    posicionarCentradoEnPanel(centroPanel, anchoPanel, altoPanel, 6.f);
}
