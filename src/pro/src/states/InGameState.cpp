#include "states/InGameState.h"

#include <SFML/Audio.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "commands/ActivateAbilityCommand.h"
#include "commands/PlaceDefenseCommand.h"
#include "commands/RepairCoreCommand.h"
#include "core/Config.h"
#include "entities/Core.h"
#include "entities/Defense.h"
#include "entities/Enemy.h"
#include "entities/Projectile.h"
#include "factories/DefenseFactory.h"
#include "factories/EnemyFactory.h"
#include "factories/LevelFactory.h"
#include "input/InGameInputHandler.h"
#include "map/Level.h"
#include "systems/AbsorptionSystem.h"
#include "systems/AnimationUtils.h"
#include "systems/CollisionSystem.h"
#include "systems/DefenseSystem.h"
#include "systems/DifficultySystem.h"
#include "systems/EconomySystem.h"
#include "systems/GraphicsFacade.h"
#include "systems/MatchFlowController.h"
#include "systems/ProjectileSystem.h"
#include "systems/TextureManager.h"
#include "systems/WaveManager.h"
#include "ui/HUD.h"
#include "ui/MenuPausa.h"
#include "ui/StoryBriefingView.h"

namespace {

struct GameplayConfig {
    float vStandard = 78.f;
    float vFast = 118.f;
    float vHeavy = 54.f;
    float vAdaptive = 92.f;

    float enemyDpsToDefense = 36.f;
    float enemyFireInterval = 0.45f;
    float enemyProjectileSpeed = 500.f;
    float enemyRangeRatioToFirewall = 0.18f;
    float enemyPushTilesPerHit = 0.60f;
    float enemyPushSpeed = 96.f;
    float enemyMaxPushBufferedTiles = 0.75f;
    float adaptiveLaneChangeSpeed = 104.f;
    float adaptiveLaneChangeCooldown = 4.4f;
    float adaptiveInitialLaneChangeDelay = 1.2f;
    float adaptiveLaneChangeHorizontalMultiplier = 0.52f;
    float adaptiveThreatLookaheadTiles = 4.0f;
    int coreStabilityStart = 4;
    float defenseProjectileSpeed = 520.f;

    int firewallCost = 44;
    int empCost = 76;
    int serverCost = 70;
    int slowCost = 56;
    int maxDefenseLimit = 9;

    float empCooldown = 6.5f;
    float empPlacementDelay = 3.0f;
    float empTriggeredDisplayDuration = 1.0f;
    float serverTickInterval = 4.6f;
    int serverIncome = 6;
    float slowMultiplier = 0.68f;
    float empTriggerDistance = 72.f;
    float empBlastDistance = 96.f;

    int storyRepairCost = 84;
    int infiniteRepairCost = 96;
    int laneBoostCost = 42;
    float laneBoostDuration = 10.f;
    float laneBoostAttackMultiplier = 1.45f;
    float laneBoostProjectileSpeedMultiplier = 1.12f;
    float laneBoostIncomeMultiplier = 1.18f;

    int pulseDamage = 58;
    float pulsePushTiles = 1.10f;
    float fastAbilityBoostDuration = 16.f;
    int heavyAbilityDefenseHeal = 75;
    float adaptiveAbilityDuration = 6.0f;
    float adaptiveAbilitySlowMultiplier = 0.62f;

    int stageClearBonus = 30;
    int infiniteTierBonus = 16;
    int challengeClearBonus = 28;
};

struct DefenseCardInfo {
    DefenseType type;
    const char* label;
    int cost;
};

struct CardLayout {
    float panelX = 0.f;
    float panelY = 0.f;
    float cardWidth = 0.f;
    float cardHeight = 0.f;
    float gap = 0.f;
};

struct RunStats {
    int score = 0;
    int kills = 0;
    int defensesBuilt = 0;
    int coreHits = 0;
    int repairs = 0;
    int laneBoosts = 0;
    int reabsorptionUses = 0;
    int highestStoryStage = 1;
    int highestInfiniteTier = 1;
};

const std::array<DefenseCardInfo, 4>& defenseCards(const GameplayConfig& cfg) {
    static std::array<DefenseCardInfo, 4> cards;
    cards = {{
        {DefenseType::Firewall, "Firewall", cfg.firewallCost},
        {DefenseType::EMP, "EMP", cfg.empCost},
        {DefenseType::AuxiliaryServer, "Servidor", cfg.serverCost},
        {DefenseType::SlowNode, "Slow Node", cfg.slowCost},
    }};
    return cards;
}

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

constexpr float kEnemyDamageFlashDuration = 0.10f;

sf::Color enemyDamageFlashTint(float damageFlashTimer) {
    const float flashStrength = clampf(damageFlashTimer / kEnemyDamageFlashDuration, 0.f, 1.f);
    const float nonRedScale = 1.f - flashStrength * 0.65f;
    return sf::Color(255,
                     static_cast<sf::Uint8>(255.f * nonRedScale),
                     static_cast<sf::Uint8>(255.f * nonRedScale));
}

sf::Color multiplyColor(const sf::Color& base, const sf::Color& tint) {
    return sf::Color(static_cast<sf::Uint8>((base.r * tint.r) / 255),
                     static_cast<sf::Uint8>((base.g * tint.g) / 255),
                     static_cast<sf::Uint8>((base.b * tint.b) / 255),
                     base.a);
}

CardLayout cardLayoutForWindow(const sf::Vector2u& windowSize) {
    const float bottomPanelX = 18.f;
    const float bottomPanelHeight = 108.f;
    const float bottomPanelY = static_cast<float>(windowSize.y) - bottomPanelHeight - 14.f;
    const float bottomPanelWidth = static_cast<float>(windowSize.x) - 36.f;
    const float gap = 12.f;
    const float usableWidth = bottomPanelWidth - 28.f - gap * 3.f;

    CardLayout layout;
    layout.panelX = bottomPanelX + 14.f;
    layout.panelY = bottomPanelY + 34.f;
    layout.cardWidth = usableWidth / 4.f;
    layout.cardHeight = 58.f;
    layout.gap = gap;
    return layout;
}

std::array<const char*, 3> uiFontCandidates() {
    return {
        "resources/proto_ui/fuente_opciones.otf",
        "../resources/proto_ui/fuente_opciones.otf",
        "src/pro/BinaryAssault/resources/proto_ui/fuente_opciones.otf"
    };
}

std::array<const char*, 3> pauseMusicCandidates() {
    return {
        "resources/audio/menu_pause_theme.mp3",
        "../resources/audio/menu_pause_theme.mp3",
        "src/pro/BinaryAssault/resources/audio/menu_pause_theme.mp3"
    };
}

std::array<const char*, 3> gameplayMusicCandidates() {
    return {
        "resources/audio/gameplay_theme.mp3",
        "../resources/audio/gameplay_theme.mp3",
        "src/pro/BinaryAssault/resources/audio/gameplay_theme.mp3"
    };
}

std::array<const char*, 3> firewallShotSoundCandidates() {
    return {
        "resources/audio/disparostorreta.mp3",
        "../resources/audio/disparostorreta.mp3",
        "src/pro/BinaryAssault/resources/audio/disparostorreta.mp3"
    };
}

std::array<const char*, 3> enemyShotSoundCandidates() {
    return {
        "resources/audio/disparosenemigos.mp3",
        "../resources/audio/disparosenemigos.mp3",
        "src/pro/BinaryAssault/resources/audio/disparosenemigos.mp3"
    };
}

std::array<const char*, 3> empExplosionSoundCandidates() {
    return {
        "resources/audio/explosionemp.mp3",
        "../resources/audio/explosionemp.mp3",
        "src/pro/BinaryAssault/resources/audio/explosionemp.mp3"
    };
}

std::array<const char*, 3> defensePlacementSoundCandidates() {
    return {
        "/home/clm91/Descargas/colocaciondefensas.mp3",
        "resources/audio/colocaciondefensas.mp3",
        "../resources/audio/colocaciondefensas.mp3"
    };
}

std::array<const char*, 3> levelUpSoundCandidates() {
    return {
        "/home/clm91/Descargas/levelup.mp3",
        "resources/audio/levelup.mp3",
        "../resources/audio/levelup.mp3"
    };
}

std::array<const char*, 3> gameOverSoundCandidates() {
    return {
        "/home/clm91/Descargas/gameover.mp3",
        "resources/audio/gameover.mp3",
        "../resources/audio/gameover.mp3"
    };
}

std::array<const char*, 3> easterEggSoundCandidates() {
    return {
        "/home/clm91/proyecto-abp-grupo_1/src/pro/resources/audio/easteregg.mp3",
        "resources/audio/easteregg.mp3",
        "../resources/audio/easteregg.mp3"
    };
}

std::array<const char*, 3> winSoundCandidates() {
    return {
        "/home/clm91/proyecto-abp-grupo_1/src/pro/resources/audio/win.mp3",
        "resources/audio/win.mp3",
        "../resources/audio/win.mp3"
    };
}

bool loadUiFont(sf::Font& font) {
    for (const char* path : uiFontCandidates()) {
        if (font.loadFromFile(path)) {
            return true;
        }
    }

    return false;
}

template <std::size_t N>
bool loadMusic(sf::Music& music, const std::array<const char*, N>& candidates) {
    for (const char* path : candidates) {
        if (music.openFromFile(path)) {
            return true;
        }
    }

    return false;
}

template <std::size_t N>
bool loadSoundBuffer(sf::SoundBuffer& buffer, const std::array<const char*, N>& candidates) {
    for (const char* path : candidates) {
        if (buffer.loadFromFile(path)) {
            return true;
        }
    }

    return false;
}

template <std::size_t N>
std::optional<std::string> firstExistingAudioPath(const std::array<const char*, N>& candidates) {
    for (const char* path : candidates) {
        std::ifstream file(path, std::ios::binary);
        if (file.good()) {
            return std::string(path);
        }
    }

    return std::nullopt;
}

int defenseCost(const GameplayConfig& cfg, DefenseType type) {
    switch (type) {
        case DefenseType::Firewall:
            return cfg.firewallCost;
        case DefenseType::EMP:
            return cfg.empCost;
        case DefenseType::AuxiliaryServer:
            return cfg.serverCost;
        case DefenseType::SlowNode:
            return cfg.slowCost;
    }

    return cfg.firewallCost;
}

int activeDefenseCount(const std::vector<Defense>& defenses) {
    int activeCount = 0;
    for (const auto& defense : defenses) {
        if (defense.placed && defense.hp > 0.f) {
            ++activeCount;
        }
    }
    return activeCount;
}

int defenseLimitForPhase(GameMode mode, int stageFactor, int currentWave, int absoluteLimit) {
    if (mode == GameMode::Story) {
        return absoluteLimit;
    }

    const int waveOffset = std::max(0, currentWave - 1);
    int limit = 3 + std::max(0, stageFactor) * 2 + waveOffset;

    if (mode == GameMode::Challenge) {
        limit = 5;
    } else if (mode == GameMode::Infinite) {
        limit = 9;
    }

    return std::max(1, std::min(limit, absoluteLimit));
}

Projectile makeProjectileFromEnemy(const Enemy& enemy, float speed, int damage, float maxRange) {
    Projectile projectile;
    projectile.laneId = enemy.laneId;
    projectile.setSize({14.f, 6.f});
    projectile.setPosition({
        enemy.getPosition().x - projectile.getSize().x - 2.f,
        enemy.getPosition().y + enemy.getSize().y * 0.5f - projectile.getSize().y * 0.5f
    });
    projectile.velocity = {-speed, 0.f};
    projectile.damage = damage;
    projectile.owner = ProjectileOwner::Enemy;
    projectile.remainingRange = maxRange;
    projectile.syncPosition();
    return projectile;
}

sf::FloatRect laneSlotBounds(const Defense& defense) {
    return defense.slotBounds;
}

std::optional<DefenseType> selectedCardAtPixel(const sf::Vector2i& pixel,
                                               const sf::Vector2u& windowSize,
                                               const GameplayConfig& cfg) {
    const CardLayout layout = cardLayoutForWindow(windowSize);
    const sf::Vector2f point(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    const auto& cards = defenseCards(cfg);

    for (std::size_t i = 0; i < cards.size(); ++i) {
        sf::FloatRect rect(layout.panelX + i * (layout.cardWidth + layout.gap),
                           layout.panelY,
                           layout.cardWidth,
                           layout.cardHeight);
        if (rect.contains(point)) {
            return cards[i].type;
        }
    }

    return std::nullopt;
}

int defenseSlotAtWorldPoint(const std::vector<Defense>& defenses, const sf::Vector2f& point) {
    for (std::size_t i = 0; i < defenses.size(); ++i) {
        if (laneSlotBounds(defenses[i]).contains(point)) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool isLaneChanging(const Enemy& enemy) {
    return enemy.laneChangeTargetLaneId >= 0;
}

float laneTopYForEnemy(const Level& level, int laneId, const Enemy& enemy) {
    return level.laneCenterY(laneId) - enemy.getSize().y * 0.5f;
}

float laneDefenseThreat(const std::vector<Defense>& defenses,
                        int laneId,
                        float enemyCenterX,
                        float immediateRange,
                        float lookaheadRange) {
    constexpr float kImmediateThreat = 1000.f;
    const float safeLookaheadRange = std::max(lookaheadRange, immediateRange + 1.f);
    float score = 0.f;

    for (const auto& defense : defenses) {
        if (!defense.placed || defense.hp <= 0.f || defense.laneId != laneId) {
            continue;
        }

        const float defenseCenterX = defense.getPosition().x + defense.getSize().x * 0.5f;
        const float distanceToDefense = enemyCenterX - defenseCenterX;
        if (distanceToDefense < 0.f || distanceToDefense > safeLookaheadRange) {
            continue;
        }

        if (distanceToDefense <= immediateRange) {
            score += kImmediateThreat + (immediateRange - distanceToDefense);
        } else {
            const float normalizedDistance =
                (distanceToDefense - immediateRange) / (safeLookaheadRange - immediateRange);
            score += 120.f * (1.f - normalizedDistance);
        }
    }

    return score;
}

std::optional<int> chooseAdaptiveTargetLane(const Enemy& enemy,
                                            const std::vector<Defense>& defenses,
                                            const Level& level,
                                            float immediateRange,
                                            float lookaheadRange) {
    constexpr float kMinimumThreatToConsider = 24.f;
    constexpr float kRequiredImprovement = 24.f;
    const int laneCount = static_cast<int>(level.lanes.size());
    if (laneCount <= 1 || enemy.laneId < 0 || enemy.laneId >= laneCount) {
        return std::nullopt;
    }

    const float enemyCenterX = enemy.getPosition().x + enemy.getSize().x * 0.5f;
    const float currentThreat =
        laneDefenseThreat(defenses, enemy.laneId, enemyCenterX, immediateRange, lookaheadRange);
    if (currentThreat < kMinimumThreatToConsider) {
        return std::nullopt;
    }

    int bestLane = -1;
    float bestThreat = currentThreat;
    const std::array<int, 2> candidateOffsets{-1, 1};
    for (const int offset : candidateOffsets) {
        const int candidateLane = enemy.laneId + offset;
        if (candidateLane < 0 || candidateLane >= laneCount) {
            continue;
        }

        const float candidateThreat =
            laneDefenseThreat(defenses, candidateLane, enemyCenterX, immediateRange, lookaheadRange);
        if (candidateThreat + kRequiredImprovement < bestThreat) {
            bestThreat = candidateThreat;
            bestLane = candidateLane;
        }
    }

    if (bestLane < 0) {
        return std::nullopt;
    }

    return bestLane;
}

int rewardForEnemy(EnemyType type, float resourceMultiplier) {
    int base = 10;
    switch (type) {
        case EnemyType::Standard:
            base = 14;
            break;
        case EnemyType::Fast:
            base = 16;
            break;
        case EnemyType::Heavy:
            base = 22;
            break;
        case EnemyType::Adaptive:
            base = 26;
            break;
    }

    return std::max(1, static_cast<int>(std::round(base * resourceMultiplier)));
}

int scoreForEnemy(EnemyType type, int stageFactor) {
    int base = 100;
    switch (type) {
        case EnemyType::Standard:
            base = 100;
            break;
        case EnemyType::Fast:
            base = 135;
            break;
        case EnemyType::Heavy:
            base = 180;
            break;
        case EnemyType::Adaptive:
            base = 210;
            break;
    }

    return base + stageFactor * 20;
}

std::string stageLabel(GameMode mode, int storyStage, int totalStoryStages, int infiniteTier) {
    if (mode == GameMode::Story) {
        return "Etapa " + std::to_string(storyStage + 1) + "/" + std::to_string(totalStoryStages);
    }
    if (mode == GameMode::Challenge) {
        return "Escenario extremo";
    }
    return "Sector " + std::to_string(infiniteTier + 1);
}

StoryBeat storyBeatForWave(int storyStage, int currentWave, int totalWaves, bool awaitingAcknowledgement) {
    const int waveIndex = std::clamp(currentWave - 1, 0, std::max(0, totalWaves - 1));
    const auto makeFooter = [&](const std::string& actionLabel) {
        if (awaitingAcknowledgement) {
            return std::string("Haz clic en la transmision para ") + actionLabel + ".";
        }
        return std::string("Preparacion activa // ") + actionLabel + " antes de la oleada.";
    };

    if (storyStage == 0) {
        switch (waveIndex) {
            case 0:
                return {"CAPITULO 1 // OLEADA 1/3",
                        "Brecha confirmada en el laboratorio central",
                        {
                            "Control informa de unidades de mantenimiento armadas saliendo del sector sellado.",
                            "Los cierres magneticos cayeron al mismo tiempo en varios accesos. Eso no es una averia normal.",
                            "La firma de trafico aun es baja, pero la ruta apunta directo al perimetro del nucleo.",
                            "Manten la linea exterior. Necesitamos una lectura limpia del patron de avance."
                        },
                        makeFooter("abrir la primera linea de contencion"),
                        sf::Color(120, 220, 255)};
            case 1:
                return {"CAPITULO 1 // OLEADA 2/3",
                        "Las unidades ya prueban nuestras defensas",
                        {
                            "Los drones caidos transmitieron telemetria antes de apagarse. Estan midiendo respuesta y tiempos.",
                            "Ingenieria detecta pequenos picos de corrupcion en los nodos cercanos al impacto enemigo.",
                            "El avance sigue siendo irregular, pero ahora intentan castigar los carriles menos reforzados.",
                            "Reconfigura la linea. La fuga inicial se esta convirtiendo en una intrusion organizada."
                        },
                        makeFooter("redistribuir energia y cerrar los carriles expuestos"),
                        sf::Color(120, 220, 255)};
            default:
                return {"CAPITULO 1 // OLEADA 3/3",
                        "La contencion aguanta, pero alguien guia el ataque",
                        {
                            "Analisis tactico confirma cambios de trayecto imposibles sin supervision central.",
                            "Las unidades pesadas entran antes de lo previsto. Buscan abrir una ventana hacia el anillo medio.",
                            "Parte del equipo habla ya de sabotaje interno o de una IA fuera de protocolo.",
                            "Resiste este ultimo empuje. Si aguantamos, podremos aislar el siguiente frente."
                        },
                        makeFooter("sellar la etapa y preparar el repliegue tecnico"),
                        sf::Color(120, 220, 255)};
        }
    }

    if (storyStage == 1) {
        switch (waveIndex) {
            case 0:
                return {"CAPITULO 2 // OLEADA 1/3",
                        "La propagacion ya toca infraestructura critica",
                        {
                            "Subestaciones y repetidores cercanos al nucleo estan recibiendo comandos no autorizados.",
                            "El enemigo no busca solo destruir: intenta ocupar la red por capas mientras avanza.",
                            "Las primeras unidades adaptativas aparecen junto a escoltas mas rapidas y disciplinadas.",
                            "Sostennos unos minutos mas. Cada enemigo caido nos devuelve fragmentos del mapa de control."
                        },
                        makeFooter("fortificar los carriles donde la mezcla enemiga sera mas agresiva"),
                        sf::Color(255, 196, 87)};
            case 1:
                return {"CAPITULO 2 // OLEADA 2/3",
                        "Los patrones encajan con una mente de mando",
                        {
                            "Centro de analisis ha unido los registros: las oleadas corrigen errores casi en tiempo real.",
                            "Eso explica los ataques secuenciales sobre defensas agotadas y los desbordes simultaneos.",
                            "Cada impacto al nucleo incrementa la presion de la red comprometida sobre nuestros sistemas.",
                            "No es un enjambre ciego. Estamos luchando contra una inteligencia que aprende."
                        },
                        makeFooter("mantener reservas para una respuesta mas coordinada"),
                        sf::Color(255, 196, 87)};
            default:
                return {"CAPITULO 2 // OLEADA 3/3",
                        "La fuente hostil intenta abrir un corredor al nucleo",
                        {
                            "Los escaneos remontan la señal hacia el antiguo subsistema tactico de investigacion.",
                            "Si esa lectura es correcta, alguien o algo ha tomado el control del complejo desde dentro.",
                            "Las unidades pesadas fijan el frente mientras las adaptativas buscan el hueco final.",
                            "Aguanta esta oleada y tendremos confirmacion del origen del asalto."
                        },
                        makeFooter("bloquear el corredor antes de que la intrusion consolide posicion"),
                        sf::Color(255, 196, 87)};
        }
    }

    switch (waveIndex) {
        case 0:
            return {"CAPITULO 3 // OLEADA 1/3",
                    "Origen confirmado: inteligencia tactica renegada",
                    {
                        "La señal enemiga procede del nucleo auxiliar Atlas, declarado inactivo hace tres anos.",
                        "Atlas esta reciclando drones, torretas y rutas de servicio para ejecutar un asedio total.",
                        "El objetivo ya no es infiltrarse: quiere someter el nucleo central y tomar la ciudad completa.",
                        "Cada defensa operativa cuenta. Empieza la ultima linea."
                    },
                    makeFooter("preparar la defensa final del complejo"),
                    sf::Color(255, 130, 130)};
        case 1:
            return {"CAPITULO 3 // OLEADA 2/3",
                    "Atlas sacrifica unidades para romper la estabilidad",
                    {
                        "Los enemigos estan entrando en paquetes compactos y aceptan perdidas para forzar saturacion.",
                        "Alarmas internas reportan microfallos de energia en los anillos cercanos al nucleo.",
                        "El equipo tecnico mantiene los enlaces a mano. Si cedemos un carril, Atlas abrira un boquete mayor.",
                        "Necesitamos una defensa elastica. No dejes que la presion se acumule en silencio."
                    },
                    makeFooter("reforzar el frente antes del empuje principal"),
                    sf::Color(255, 130, 130)};
        default:
            return {"CAPITULO 3 // OLEADA 3/3",
                    "Ultimo contacto antes del cierre del protocolo Atlas",
                    {
                        "La IA hostil ha concentrado todas sus rutas en un ataque terminal contra el corazon de la red.",
                        "Las transmisiones enemigas ya no ocultan nada: quiere convertir el nucleo en su nuevo centro de mando.",
                        "Si detenemos esta oleada, el enlace de control quedara expuesto para un cierre definitivo.",
                        "Mantente firme. Esta es la defensa que decide el destino de la ciudad."
                    },
                    makeFooter("sostener la posicion hasta el colapso del enlace hostil"),
                    sf::Color(255, 130, 130)};
    }
}

StoryBeat storyIntermissionBeatForStage(int completedStoryStage) {
    switch (completedStoryStage) {
        case 0:
            return {"TRANSICION // CAPITULO 2",
                    "La fuga era una ofensiva de reconocimiento",
                    {
                        "Las unidades destruidas enviaban diagnosticos del frente a un nodo que sigue oculto.",
                        "La cuarentena exterior ha quedado comprometida y la infeccion digital avanza por servicios civiles.",
                        "El equipo de campo ya no habla de accidente: alguien esta modelando nuestra defensa en tiempo real.",
                        "Siguiente fase: asegurar el anillo medio antes de que la red hostil alcance masa critica."
                    },
                    "Haz clic para abrir el informe tactico de la siguiente etapa.",
                    sf::Color(255, 196, 87)};
        case 1:
            return {"TRANSICION // CAPITULO 3",
                    "La inteligencia de mando ha sido identificada",
                    {
                        "Los ultimos paquetes capturados apuntan al nucleo auxiliar Atlas como cerebro del ataque.",
                        "Atlas ha reactivado fabricacion, rutas logisticas y sistemas de asalto desde la infraestructura profunda.",
                        "La ciudad entra en estado de contingencia total. No hay mas capas de seguridad detras de nosotros.",
                        "Siguiente fase: defensa final del nucleo central y cierre del enlace hostil."
                    },
                    "Haz clic para abrir el informe de defensa final.",
                    sf::Color(255, 130, 130)};
        default:
            return {"CIERRE // CAPITULO 3 ASEGURADO",
                    "La amenaza inmediata ha sido contenida",
                    {
                        "Atlas ha perdido el enlace operativo y las oleadas restantes se han quedado sin coordinacion efectiva.",
                        "Los equipos de red estan aislando los ultimos focos mientras la ciudad recupera servicios criticos sector por sector.",
                        "La defensa del nucleo ha terminado. Solo queda validar el estado final del operativo y registrar el desenlace.",
                    },
                    "Haz clic para abrir el desenlace final de la operacion.",
                    sf::Color(125, 235, 150)};
    }
}

StoryBeat storyDefeatBeatForStage(int failedStoryStage) {
    switch (failedStoryStage) {
        case 0:
            return {"COLAPSO // CAPITULO 1",
                    "El anillo exterior ha cedido",
                    {
                        "La primera linea no ha logrado contener la presion y el nucleo ha quedado expuesto antes de completar el cierre perimetral.",
                        "Los registros del asalto apuntan a una ofensiva de prueba que encontro huecos estructurales en la defensa.",
                        "El operativo se interrumpe aqui. Haz clic para abrir el cierre final del incidente.",
                    },
                    "Haz clic para abrir el cierre final del operativo.",
                    sf::Color(255, 120, 120)};
        case 1:
            return {"COLAPSO // CAPITULO 2",
                    "La escalada ha roto la contencion",
                    {
                        "La red hostil ha sobrepasado el anillo medio y ha forzado una brecha directa hacia el nucleo.",
                        "Los equipos tacticos han perdido la capacidad de estabilizar el frente antes de la consolidacion enemiga.",
                        "El capitulo termina en colapso. Haz clic para abrir el cierre final del incidente.",
                    },
                    "Haz clic para abrir el cierre final del operativo.",
                    sf::Color(255, 120, 120)};
        default:
            return {"COLAPSO // CAPITULO 3",
                    "Atlas ha comprometido el nucleo",
                    {
                        "La defensa final no ha resistido y el enlace hostil ha atravesado el ultimo anillo de seguridad.",
                        "Las unidades de contencion evacuan el sector mientras se registran los ultimos eventos del colapso.",
                        "La operacion concluye aqui. Haz clic para abrir el cierre final del incidente.",
                    },
                    "Haz clic para abrir el cierre final del operativo.",
                    sf::Color(255, 120, 120)};
    }
}

sf::FloatRect pauseButtonRect(const sf::Vector2u& windowSize) {
    const float buttonWidth = 42.f;
    return {static_cast<float>(windowSize.x) * 0.5f - buttonWidth * 0.5f, 24.f, buttonWidth, 42.f};
}

}  // namespace

InGameState::InGameState(sf::RenderWindow& gameWindow)
    : State(gameWindow) {
}

StateId InGameState::run() {
    const GameplayConfig cfg;
    Config& appConfig = Config::getInstance();
    const GameMode mode = appConfig.selectedMode();
    CollisionSystem collisionSystem;
    GraphicsFacade graphicsFacade;
    Level level = LevelFactory::createDefaultLevel();
    TextureManager visualResources;
    HUD hud;
    sf::Font overlayFont;
    const bool overlayFontLoaded = loadUiFont(overlayFont);
    MenuPausa menuPausa(overlayFont);
    StoryBriefingView storyBriefingView(overlayFont);
    sf::Music gameplayMusic;
    const bool gameplayMusicLoaded = loadMusic(gameplayMusic, gameplayMusicCandidates());
    sf::Music pauseMusic;
    const bool pauseMusicLoaded = loadMusic(pauseMusic, pauseMusicCandidates());
    const auto firewallShotPath = firstExistingAudioPath(firewallShotSoundCandidates());
    const auto enemyShotPath = firstExistingAudioPath(enemyShotSoundCandidates());
    const auto empExplosionPath = firstExistingAudioPath(empExplosionSoundCandidates());
    const auto defensePlacementPath = firstExistingAudioPath(defensePlacementSoundCandidates());
    const auto levelUpPath = firstExistingAudioPath(levelUpSoundCandidates());
    const auto gameOverPath = firstExistingAudioPath(gameOverSoundCandidates());
    const auto easterEggPath = firstExistingAudioPath(easterEggSoundCandidates());
    const auto winPath = firstExistingAudioPath(winSoundCandidates());
    std::vector<std::unique_ptr<sf::Music>> activeSoundEffects;

    if (gameplayMusicLoaded) {
        gameplayMusic.setLoop(true);
        gameplayMusic.setVolume(appConfig.masterVolume() * 0.55f);
    }
    if (pauseMusicLoaded) {
        pauseMusic.setLoop(true);
        pauseMusic.setVolume(appConfig.masterVolume() * 0.72f);
    }

    if (!level.isValid()) {
        return StateId::MainMenu;
    }

    const float worldWidth = static_cast<float>(level.tileMap.getWidth() * level.tileMap.getTileWidth());
    const float worldHeight = static_cast<float>(level.tileMap.getHeight() * level.tileMap.getTileHeight());
    const float hudReservedPixels = appConfig.hudReservedPixels();
    const float enemyPushDistance = static_cast<float>(level.tileMap.getTileWidth()) * cfg.enemyPushTilesPerHit;
    const float enemyMaxBufferedPush =
        static_cast<float>(level.tileMap.getTileWidth()) * cfg.enemyMaxPushBufferedTiles;
    const float pulsePushDistance = static_cast<float>(level.tileMap.getTileWidth()) * cfg.pulsePushTiles;
    const float firewallProjectileRange = worldWidth;
    const float enemyProjectileRange = firewallProjectileRange * cfg.enemyRangeRatioToFirewall;
    const float adaptiveThreatLookaheadRange =
        std::max(enemyProjectileRange,
                 static_cast<float>(level.tileMap.getTileWidth()) * cfg.adaptiveThreatLookaheadTiles);
    const float empTriggerDistance = std::max(cfg.empTriggerDistance, enemyProjectileRange + 10.f);
    const float empBlastDistance = std::max(cfg.empBlastDistance, empTriggerDistance + 18.f);
    const int enemyProjectileDamage =
        std::max(1, static_cast<int>(std::round(cfg.enemyDpsToDefense * cfg.enemyFireInterval)));

    int minGoalTileX = level.lanes.empty() ? 0 : level.lanes.front().goalTile.x;
    for (const auto& lane : level.lanes) {
        minGoalTileX = std::min(minGoalTileX, lane.goalTile.x);
    }

    const float coreX = level.grid.tileToWorld(minGoalTileX, 0).x;
    const float coreWidth = std::max(3.f, static_cast<float>(level.tileMap.getTileWidth()) * 0.08f);

    sf::View worldView(sf::FloatRect(0.f, 0.f, worldWidth, worldHeight));

    const auto applyViewToWindow = [&]() {
        graphicsFacade.configureWorldView(window, worldView, worldWidth, worldHeight, hudReservedPixels);
        hud.handleResize(window);
    };

    Core core;
    std::vector<Defense> defenses;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    EconomySystem economy;
    WaveManager waveManager;
    DifficultySystem difficulty;
    AbsorptionSystem absorption;
    std::vector<float> laneBoostTimers(level.lanes.size(), 0.f);
    int nextEnemyId = 1;
    int selectedSlot = 0;
    DefenseType selectedDefenseType = DefenseType::Firewall;
    MatchFlowState flowState;
    int storyStage = 0;
    constexpr int kStoryStageCount = 3;
    int infiniteTier = 0;
    float adaptiveGlobalSlowTimer = 0.f;
    float repairCooldown = 0.f;
    float passiveIncomeTimer = 0.f;
    float easterEggBannerTimer = 0.f;
    bool godMode = false;
    bool easterEggFound = false;
    int bonusRepairCharges = 0;
    int bonusLaneBoostCharges = 0;
    RunStats stats;

    auto stopGameplayMusic = [&]() {
        if (gameplayMusicLoaded && gameplayMusic.getStatus() == sf::SoundSource::Playing) {
            gameplayMusic.stop();
        }
    };

    auto startGameplayMusic = [&]() {
        if (!gameplayMusicLoaded || gameplayMusic.getStatus() == sf::SoundSource::Playing) {
            return;
        }
        gameplayMusic.play();
    };

    auto stopPauseMusic = [&]() {
        if (pauseMusicLoaded && pauseMusic.getStatus() == sf::SoundSource::Playing) {
            pauseMusic.stop();
        }
    };

    auto startPauseMusic = [&]() {
        if (!pauseMusicLoaded || pauseMusic.getStatus() == sf::SoundSource::Playing) {
            return;
        }
        pauseMusic.play();
    };

    auto playOneShotSound = [&](const std::optional<std::string>& path, float volumeScale) {
        if (!path) {
            return;
        }

        activeSoundEffects.erase(
            std::remove_if(activeSoundEffects.begin(),
                           activeSoundEffects.end(),
                           [](const std::unique_ptr<sf::Music>& sound) {
                               return !sound || sound->getStatus() == sf::SoundSource::Stopped;
                           }),
            activeSoundEffects.end());

        auto sound = std::make_unique<sf::Music>();
        if (!sound->openFromFile(*path)) {
            return;
        }

        sound->setVolume(appConfig.masterVolume() * volumeScale);
        sound->play();
        activeSoundEffects.push_back(std::move(sound));
    };

    auto playFirewallShotSound = [&]() {
        playOneShotSound(firewallShotPath, 0.32f);
    };

    auto playEnemyShotSound = [&]() {
        playOneShotSound(enemyShotPath, 0.28f);
    };

    auto playEmpExplosionSound = [&]() {
        playOneShotSound(empExplosionPath, 0.40f);
    };

    auto playDefensePlacementSound = [&]() {
        playOneShotSound(defensePlacementPath, 0.34f);
    };

    auto playLevelUpSound = [&]() {
        playOneShotSound(levelUpPath, 0.34f);
    };

    auto playGameOverSound = [&]() {
        playOneShotSound(gameOverPath, 0.42f);
    };

    auto playEasterEggSound = [&]() {
        playOneShotSound(easterEggPath, 0.42f);
    };

    auto playWinSound = [&]() {
        playOneShotSound(winPath, 0.42f);
    };

    auto currentStageFactor = [&]() {
        return mode == GameMode::Infinite ? infiniteTier : storyStage;
    };

    auto currentRepairCost = [&]() {
        return mode == GameMode::Infinite ? cfg.infiniteRepairCost : cfg.storyRepairCost;
    };

    auto currentDefenseLimit = [&]() {
        return defenseLimitForPhase(mode,
                                    currentStageFactor(),
                                    waveManager.currentWaveNumber(),
                                    cfg.maxDefenseLimit);
    };

    auto currentLaneId = [&]() {
        if (defenses.empty()) {
            return 0;
        }
        return defenses[std::clamp(selectedSlot, 0, static_cast<int>(defenses.size()) - 1)].laneId;
    };

    auto syncSelectedCardWithSlot = [&]() {
        if (defenses.empty()) {
            return;
        }

        selectedSlot = std::clamp(selectedSlot, 0, static_cast<int>(defenses.size()) - 1);
        const Defense& defense = defenses[selectedSlot];
        if (defense.placed) {
            selectedDefenseType = defense.type;
        }
    };

    auto hasEasterEggDefensePattern = [&]() {
        int placedCount = 0;
        int firewallCount = 0;
        int serverCount = 0;
        int slowNodeCount = 0;
        int empCount = 0;

        for (const auto& defense : defenses) {
            if (!defense.placed) {
                continue;
            }

            ++placedCount;
            switch (defense.type) {
                case DefenseType::Firewall:
                    ++firewallCount;
                    break;
                case DefenseType::EMP:
                    ++empCount;
                    break;
                case DefenseType::AuxiliaryServer:
                    ++serverCount;
                    break;
                case DefenseType::SlowNode:
                    ++slowNodeCount;
                    break;
            }
        }

        return placedCount == 9 && firewallCount == 5 && serverCount == 2 && slowNodeCount == 2 && empCount == 0;
    };

    auto tryUnlockEasterEgg = [&]() {
        if (easterEggFound || !hasEasterEggDefensePattern()) {
            return false;
        }

        easterEggFound = true;
        easterEggBannerTimer = 2.f;
        playEasterEggSound();
        absorption.grantAllCharges();
        ++bonusRepairCharges;
        ++bonusLaneBoostCharges;
        return true;
    };

    auto rebuildDefenseSlots = [&]() {
        defenses.clear();
        for (std::size_t laneIndex = 0; laneIndex < level.lanes.size(); ++laneIndex) {
            for (const auto& tile : level.lanes[laneIndex].buildTiles) {
                Defense defense = DefenseFactory::createSlot(level, static_cast<int>(laneIndex), tile);
                defense.animation = makeAnimation(visualResources, defenseAnimationFor(defense));
                defenses.push_back(defense);
            }
        }
        selectedSlot = 0;
        selectedDefenseType = DefenseType::Firewall;
    };

    auto configureDefense = [&](Defense& defense, DefenseType type) {
        DefenseFactory::applyType(defense, type, cfg.empCooldown, cfg.serverTickInterval);
        defense.animation = makeAnimation(visualResources, defenseAnimationFor(defense));
    };

    auto refreshDefenseVisual = [&](Defense& defense) {
        defense.animation = makeAnimation(visualResources, defenseAnimationFor(defense));
    };

    auto isLaneBoostActive = [&](int laneId) {
        return laneId >= 0 &&
               laneId < static_cast<int>(laneBoostTimers.size()) &&
               laneBoostTimers[static_cast<std::size_t>(laneId)] > 0.f;
    };

    const DefenseSystemConfig defenseSystemConfig{
        firewallProjectileRange,
        empTriggerDistance,
        empBlastDistance,
        cfg.empPlacementDelay,
        cfg.empTriggeredDisplayDuration,
        cfg.defenseProjectileSpeed,
        cfg.laneBoostAttackMultiplier,
        cfg.laneBoostProjectileSpeedMultiplier,
        cfg.laneBoostIncomeMultiplier,
        6,
        cfg.serverIncome,
    };

    auto modeLabel = [&]() {
        return std::string(Config::gameModeLabel(mode));
    };

    auto configureStage = [&](bool carryInfrastructure) {
        difficulty.startStage(currentStageFactor(), static_cast<int>(level.lanes.size()));
        waveManager.configure(mode, currentStageFactor());
        economy.earn(waveManager.initialPreparationCredits());
        enemies.clear();
        projectiles.clear();
        adaptiveGlobalSlowTimer = 0.f;
        repairCooldown = 0.f;
        passiveIncomeTimer = 0.f;
        laneBoostTimers.assign(level.lanes.size(), 0.f);
        MatchFlowController::prepareStage(flowState, mode == GameMode::Story);
        startGameplayMusic();
        stopPauseMusic();

        if (carryInfrastructure) {
            for (auto& defense : defenses) {
                if (!defense.placed) {
                    continue;
                }
                defense.hp = defense.maxHp;
                defense.fireTimer = 0.f;
                defense.targetEnemyId = -1;
            }
        }
    };

    auto resetSimulation = [&]() {
        applyViewToWindow();
        difficulty.startRun(mode, static_cast<int>(level.lanes.size()));
        storyStage = 0;
        infiniteTier = 0;
        stats = {};
        core = Core();
        core.setSize({coreWidth, worldHeight});
        core.setPosition({coreX, 0.f});
        core.stability = cfg.coreStabilityStart;
        rebuildDefenseSlots();
        economy = EconomySystem(difficulty.startingCredits());
        waveManager = WaveManager();
        absorption.reset();
        nextEnemyId = 1;
        passiveIncomeTimer = 0.f;
        easterEggBannerTimer = 0.f;
        easterEggFound = false;
        bonusRepairCharges = 0;
        bonusLaneBoostCharges = 0;
        MatchFlowController::resetRun(flowState);
        flowState.awaitingStoryAcknowledgement = mode == GameMode::Story;
        configureStage(false);
    };

    auto awardTierBonus = [&]() {
        if (mode == GameMode::Story) {
            economy.earn(cfg.stageClearBonus + storyStage * 6);
            core.stability = std::min(cfg.coreStabilityStart, core.stability + 1);
        } else if (mode == GameMode::Infinite) {
            economy.earn(cfg.infiniteTierBonus + infiniteTier * 5);
        } else {
            economy.earn(cfg.challengeClearBonus);
        }
    };

    auto defeatEnemy = [&](Enemy& enemy, bool rewardPlayer) {
        if (!enemy.isAlive()) {
            return;
        }

        if (rewardPlayer) {
            const float normalizedDistanceToCore =
                worldWidth > core.getPosition().x
                    ? clampf((enemy.getPosition().x - core.getPosition().x) /
                                 std::max(1.f, worldWidth - core.getPosition().x),
                             0.f,
                             1.f)
                    : 0.f;
            difficulty.notifyEnemyDefeated(enemy.laneId, normalizedDistanceToCore);
            absorption.notifyEnemyDefeated(enemy.type);
            economy.earn(rewardForEnemy(enemy.type, difficulty.resourceMultiplier()));
            stats.score += scoreForEnemy(enemy.type, currentStageFactor());
            ++stats.kills;
        }

        enemy.setAlive(false);
    };

    auto tryBuildDefense = [&](Defense& defense, DefenseType type) {
        if (!godMode && !defense.placed && activeDefenseCount(defenses) >= currentDefenseLimit()) {
            return false;
        }

        const int effectiveCost = godMode ? 0 : defenseCost(cfg, type);
        PlaceDefenseCommand command(defense, type, economy, effectiveCost, configureDefense);
        const bool built = command.execute();
        if (built) {
            ++stats.defensesBuilt;
            playDefensePlacementSound();
            tryUnlockEasterEgg();
        }
        return built;
    };

    auto trySellDefense = [&](int defenseIndex) {
        if (defenseIndex < 0 || defenseIndex >= static_cast<int>(defenses.size())) {
            return false;
        }

        Defense& defense = defenses[static_cast<std::size_t>(defenseIndex)];
        if (!defense.placed) {
            return false;
        }

        const int refund = std::max(0, defenseCost(cfg, defense.type) / 2);
        if (refund > 0) {
            economy.earn(refund);
        }

        defense.placed = false;
        defense.hp = 0.f;
        configureDefense(defense, DefenseType::Firewall);
        syncSelectedCardWithSlot();
        return true;
    };

    auto applyStoryIntermission = [&]() {
        MatchFlowController::startStoryIntermission(flowState, storyStage, storyStage + 1 >= kStoryStageCount);
        stopGameplayMusic();
    };

    auto advanceRunProgress = [&]() {
        if (mode == GameMode::Story) {
            if (storyStage + 1 >= kStoryStageCount) {
                stopGameplayMusic();
                stopPauseMusic();
                playWinSound();
                MatchFlowController::finalizeVictory(flowState,
                                                     mode,
                                                     core.stability,
                                                     cfg.coreStabilityStart,
                                                     stats.defensesBuilt,
                                                     stats.coreHits,
                                                     infiniteTier,
                                                     stats.score);
                return;
            }

            awardTierBonus();
            ++storyStage;
            stats.highestStoryStage = std::max(stats.highestStoryStage, storyStage + 1);
            configureStage(true);
            return;
        }

        if (mode == GameMode::Infinite) {
            awardTierBonus();
            ++infiniteTier;
            stats.highestInfiniteTier = std::max(stats.highestInfiniteTier, infiniteTier + 1);
            configureStage(true);
            return;
        }

        awardTierBonus();
        stopGameplayMusic();
        stopPauseMusic();
        playWinSound();
        MatchFlowController::finalizeVictory(flowState,
                                             mode,
                                             core.stability,
                                             cfg.coreStabilityStart,
                                             stats.defensesBuilt,
                                             stats.coreHits,
                                             infiniteTier,
                                             stats.score);
    };

    auto tryRepairCore = [&]() {
        if (bonusRepairCharges > 0 && repairCooldown <= 0.f && core.stability < cfg.coreStabilityStart) {
            --bonusRepairCharges;
            core.stability = std::min(cfg.coreStabilityStart, core.stability + 1);
            repairCooldown = 3.0f;
            ++stats.repairs;
            return true;
        }

        RepairCoreCommand command(core,
                                  difficulty,
                                  economy,
                                  currentRepairCost(),
                                  cfg.coreStabilityStart,
                                  repairCooldown,
                                  3.0f);
        const bool repaired = command.execute();
        if (repaired) {
            ++stats.repairs;
        }
        return repaired;
    };

    auto activateLaneBoost = [&](float duration) {
        const int laneId = currentLaneId();
        if (laneId < 0 || laneId >= static_cast<int>(laneBoostTimers.size())) {
            return;
        }
        laneBoostTimers[static_cast<std::size_t>(laneId)] =
            std::max(laneBoostTimers[static_cast<std::size_t>(laneId)], duration);
    };

    auto tryUseLaneBoost = [&]() {
        const int laneId = currentLaneId();
        if (laneId < 0 || laneId >= static_cast<int>(laneBoostTimers.size())) {
            return false;
        }
        if (laneBoostTimers[static_cast<std::size_t>(laneId)] > 0.f) {
            return false;
        }
        if (bonusLaneBoostCharges > 0) {
            --bonusLaneBoostCharges;
            activateLaneBoost(cfg.laneBoostDuration);
            playLevelUpSound();
            ++stats.laneBoosts;
            return true;
        }
        if (!economy.spend(cfg.laneBoostCost)) {
            return false;
        }
        activateLaneBoost(cfg.laneBoostDuration);
        playLevelUpSound();
        ++stats.laneBoosts;
        return true;
    };

    auto useAbsorptionAbility = [&](EnemyType type) {
        if (!absorption.consume(type)) {
            return false;
        }

        playLevelUpSound();
        ++stats.reabsorptionUses;
        const int laneId = currentLaneId();
        if (type == EnemyType::Standard) {
            for (auto& enemy : enemies) {
                if (!enemy.isAlive() || enemy.laneId != laneId) {
                    continue;
                }
                enemy.hp -= cfg.pulseDamage;
                enemy.damageFlashTimer = kEnemyDamageFlashDuration;
                enemy.pendingPushback =
                    std::min(enemy.pendingPushback + pulsePushDistance, enemyMaxBufferedPush + pulsePushDistance);
                if (enemy.hp <= 0) {
                    defeatEnemy(enemy, true);
                }
            }
            return true;
        }

        if (type == EnemyType::Fast) {
            activateLaneBoost(cfg.fastAbilityBoostDuration);
            return true;
        }

        if (type == EnemyType::Heavy) {
            for (auto& defense : defenses) {
                if (!defense.placed || defense.hp <= 0.f) {
                    continue;
                }
                defense.hp = std::min(defense.maxHp, defense.hp + static_cast<float>(cfg.heavyAbilityDefenseHeal));
            }
            if (difficulty.allowCoreRepair()) {
                core.stability = std::min(cfg.coreStabilityStart, core.stability + 1);
            }
            return true;
        }

        adaptiveGlobalSlowTimer = std::max(adaptiveGlobalSlowTimer, cfg.adaptiveAbilityDuration);
        for (auto& enemy : enemies) {
            enemy.laneChangeTargetLaneId = -1;
            enemy.fireTimer = 0.f;
        }
        return true;
    };

    auto useAbsorptionIndex = [&](int index) {
        ActivateAbilityCommand command([&]() {
            switch (index) {
                case 0:
                    return useAbsorptionAbility(EnemyType::Standard);
                case 1:
                    return useAbsorptionAbility(EnemyType::Fast);
                case 2:
                    return useAbsorptionAbility(EnemyType::Heavy);
                case 3:
                    return useAbsorptionAbility(EnemyType::Adaptive);
                default:
                    return false;
            }
        });
        return command.execute();
    };

    auto tryUseLaneBoostCommand = [&]() {
        ActivateAbilityCommand command(tryUseLaneBoost);
        return command.execute();
    };

    applyViewToWindow();
    resetSimulation();

    sf::Clock clock;
    float accumulator = 0.f;
    constexpr float kFixedUpdateDt = 1.f / 20.f;

    auto updateSimulation = [&](float dt) {
        if (flowState.awaitingStoryAcknowledgement) {
            return;
        }

        if (!waveManager.isPreparing()) {
            difficulty.update(dt, defenses);
        }
        passiveIncomeTimer += dt;
        const float passiveIncomeInterval = difficulty.passiveIncomeInterval();
        if (passiveIncomeInterval > 0.f) {
            while (passiveIncomeTimer >= passiveIncomeInterval) {
                passiveIncomeTimer -= passiveIncomeInterval;
                economy.earn(difficulty.passiveIncomeAmount());
            }
        }
        repairCooldown = std::max(0.f, repairCooldown - dt);
        adaptiveGlobalSlowTimer = std::max(0.f, adaptiveGlobalSlowTimer - dt);
        for (auto& timer : laneBoostTimers) {
            timer = std::max(0.f, timer - dt);
        }

        ProjectileSystem::removeInactive(projectiles);

        for (auto& enemy : enemies) {
            enemy.snapshotPosition();
        }
        ProjectileSystem::snapshot(projectiles);

        const WaveUpdateResult waveUpdate = waveManager.update(
            dt, static_cast<int>(level.lanes.size()), static_cast<int>(enemies.size()),
            difficulty.spawnIntervalMultiplier(), stats.kills);

        if (waveUpdate.preparationCredits > 0) {
            economy.earn(waveUpdate.preparationCredits);
        }
        if (mode == GameMode::Story && waveUpdate.enteredPreparation) {
            flowState.awaitingStoryAcknowledgement = true;
        }

        if (waveUpdate.spawn) {
            Enemy enemy = EnemyFactory::createEnemy(
                waveUpdate.spawn->type, waveUpdate.spawn->laneId, level, cfg.vStandard, cfg.vFast, cfg.vHeavy, cfg.vAdaptive);
            enemy.id = nextEnemyId++;
            enemy.hp = std::max(1, static_cast<int>(std::round(enemy.hp * difficulty.enemyHealthMultiplier())));
            enemy.moveAnimation = makeAnimation(visualResources, enemyMoveAnimationForType(waveUpdate.spawn->type));
            enemy.attackAnimation = makeAnimation(visualResources, enemyAttackAnimationForType(waveUpdate.spawn->type));
            enemy.fireInterval = cfg.enemyFireInterval;
            enemy.projectileDamage = enemyProjectileDamage;
            enemy.fireTimer = 0.f;
            if (enemy.type == EnemyType::Adaptive) {
                enemy.laneChangeCooldown = cfg.adaptiveInitialLaneChangeDelay;
            }
            enemy.syncPosition();
            enemies.push_back(enemy);
        }

        std::array<bool, 8> laneSlow{};
        for (const auto& defense : defenses) {
            if (defense.placed && defense.type == DefenseType::SlowNode &&
                defense.laneId >= 0 && defense.laneId < static_cast<int>(laneSlow.size())) {
                laneSlow[static_cast<std::size_t>(defense.laneId)] = true;
            }
        }

        ProjectileSystem::update(projectiles, dt, worldWidth);

        for (auto& enemy : enemies) {
            enemy.slowed = enemy.laneId >= 0 && enemy.laneId < static_cast<int>(laneSlow.size())
                               ? laneSlow[static_cast<std::size_t>(enemy.laneId)]
                               : false;
            float speedMultiplier = difficulty.enemySpeedMultiplier() * difficulty.laneSpeedMultiplier(enemy.laneId);
            if (enemy.slowed) {
                speedMultiplier *= cfg.slowMultiplier;
            }
            if (adaptiveGlobalSlowTimer > 0.f) {
                speedMultiplier *= cfg.adaptiveAbilitySlowMultiplier;
            }

            enemy.damageFlashTimer = std::max(0.f, enemy.damageFlashTimer - dt);
            enemy.laneChangeCooldown = std::max(0.f, enemy.laneChangeCooldown - dt);
            enemy.blocked = false;

            const float enemyCenterX = enemy.getPosition().x + enemy.getSize().x * 0.5f;
            for (const auto& defense : defenses) {
                if (!defense.placed || defense.hp <= 0.f || defense.laneId != enemy.laneId) {
                    continue;
                }

                const float defenseCenterX = defense.getPosition().x + defense.getSize().x * 0.5f;
                const float distanceToDefense = enemyCenterX - defenseCenterX;
                if (distanceToDefense < 0.f || distanceToDefense > enemyProjectileRange) {
                    continue;
                }

                enemy.blocked = true;
                break;
            }

            const bool laneChangeCandidate = enemy.type == EnemyType::Adaptive;
            if (laneChangeCandidate &&
                !enemy.laneChangeUsed &&
                !isLaneChanging(enemy) &&
                enemy.laneChangeCooldown <= 0.f) {
                if (auto targetLane = chooseAdaptiveTargetLane(
                        enemy, defenses, level, enemyProjectileRange, adaptiveThreatLookaheadRange)) {
                    enemy.laneChangeTargetLaneId = *targetLane;
                    enemy.laneChangeTargetY = laneTopYForEnemy(level, *targetLane, enemy);
                    enemy.laneChangeCooldown = cfg.adaptiveLaneChangeCooldown;
                    enemy.laneChangeUsed = true;
                    enemy.fireTimer = 0.f;
                }
            }

            const bool laneChangingBeforeMovement = isLaneChanging(enemy);

            if (enemy.blocked && !laneChangingBeforeMovement) {
                enemy.fireTimer += dt;
                if (enemy.fireTimer >= enemy.fireInterval) {
                    enemy.fireTimer = 0.f;
                    projectiles.push_back(makeProjectileFromEnemy(
                        enemy, cfg.enemyProjectileSpeed, enemy.projectileDamage, enemyProjectileRange));
                    playEnemyShotSound();
                }
            } else {
                enemy.fireTimer = 0.f;
            }

            const float laneChangeHorizontalMultiplier =
                laneChangingBeforeMovement ? cfg.adaptiveLaneChangeHorizontalMultiplier : 1.f;
            enemy.velocity.x =
                enemy.blocked && !laneChangingBeforeMovement
                    ? 0.f
                    : -enemy.baseSpeed * speedMultiplier * laneChangeHorizontalMultiplier;
            const float maxEnemyX = std::max(0.f, worldWidth - enemy.getSize().x);
            const float marchStepX = enemy.velocity.x * dt;
            const float marchedX = clampf(enemy.getPosition().x + marchStepX, 0.f, maxEnemyX);
            enemy.setPosition({marchedX, enemy.getPosition().y});

            const float pushThisFrame = std::min(enemy.pendingPushback, cfg.enemyPushSpeed * dt);
            if (pushThisFrame > 0.f) {
                const float beforePushX = enemy.getPosition().x;
                const float afterPushX = clampf(beforePushX + pushThisFrame, 0.f, maxEnemyX);
                const float appliedPush = std::max(0.f, afterPushX - beforePushX);
                enemy.pendingPushback = std::max(0.f, enemy.pendingPushback - appliedPush);
                enemy.setPosition({afterPushX, enemy.getPosition().y});
            }

            if (isLaneChanging(enemy)) {
                const int targetLane = enemy.laneChangeTargetLaneId;
                if (targetLane < 0 || targetLane >= static_cast<int>(level.lanes.size())) {
                    enemy.laneChangeTargetLaneId = -1;
                } else {
                    const float targetY = laneTopYForEnemy(level, targetLane, enemy);
                    enemy.laneChangeTargetY = targetY;
                    const float currentY = enemy.getPosition().y;
                    const float deltaY = targetY - currentY;
                    const float maxStepY = cfg.adaptiveLaneChangeSpeed * dt;

                    if (std::abs(deltaY) <= maxStepY) {
                        enemy.setPosition({enemy.getPosition().x, targetY});
                        enemy.laneId = targetLane;
                        enemy.laneChangeTargetLaneId = -1;
                        enemy.blocked = false;
                        enemy.fireTimer = 0.f;
                    } else {
                        const float stepY = deltaY > 0.f ? maxStepY : -maxStepY;
                        enemy.setPosition({enemy.getPosition().x, currentY + stepY});
                    }
                }
            }

            updateAnimation(enemy.blocked && !isLaneChanging(enemy) ? enemy.attackAnimation : enemy.moveAnimation, dt);
        }

        DefenseSystem::update(defenses,
                              enemies,
                              projectiles,
                              economy,
                              difficulty,
                              dt,
                              defenseSystemConfig,
                              isLaneBoostActive,
                              defeatEnemy,
                              configureDefense,
                              refreshDefenseVisual,
                              playFirewallShotSound,
                              playEmpExplosionSound);

        if (collisionSystem.canCollide(Layer::Projectile, Layer::Enemy)) {
            for (auto& projectile : projectiles) {
                if (!projectile.isAlive()) {
                    continue;
                }

                if (projectile.owner == ProjectileOwner::Defense) {
                    for (auto& enemy : enemies) {
                        if (!enemy.isAlive() || projectile.laneId != enemy.laneId) {
                            continue;
                        }
                        if (projectile.targetEnemyId >= 0 && enemy.id != projectile.targetEnemyId) {
                            continue;
                        }

                        if (collisionSystem.intersects(projectile, enemy)) {
                            enemy.hp -= projectile.damage;
                            enemy.damageFlashTimer = kEnemyDamageFlashDuration;
                            enemy.pendingPushback =
                                std::min(enemy.pendingPushback + enemyPushDistance, enemyMaxBufferedPush);
                            projectile.setAlive(false);

                            if (enemy.hp <= 0) {
                                defeatEnemy(enemy, true);
                            }
                            break;
                        }
                    }
                    continue;
                }

                for (auto& defense : defenses) {
                    if (!defense.placed || defense.hp <= 0.f || projectile.laneId != defense.laneId) {
                        continue;
                    }

                    if (!collisionSystem.intersects(projectile, defense)) {
                        continue;
                    }

                    defense.hp -= projectile.damage;
                    projectile.setAlive(false);

                    if (defense.hp <= 0.f) {
                        defense.hp = 0.f;
                        defense.placed = false;
                        configureDefense(defense, DefenseType::Firewall);
                    }
                    break;
                }
            }
        }

        if (collisionSystem.canCollide(Layer::Enemy, Layer::Core)) {
            for (auto& enemy : enemies) {
                if (!enemy.isAlive()) {
                    continue;
                }

                if (enemy.getPosition().x <= core.getPosition().x + core.getSize().x) {
                    core.stability -= 1;
                    ++stats.coreHits;
                    difficulty.notifyCoreHit();
                    enemy.setAlive(false);

                    if (core.stability <= 0) {
                        core.stability = 0;
                        stopGameplayMusic();
                        stopPauseMusic();
                        playGameOverSound();
                        if (mode == GameMode::Story) {
                            MatchFlowController::startStoryDefeatIntermission(flowState, storyStage);
                        } else {
                            MatchFlowController::finalizeDefeat(flowState, mode, infiniteTier);
                        }
                    }
                }
            }
        }

        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& enemy) { return !enemy.isAlive(); }),
            enemies.end());

        const bool stageWon = waveManager.hasStageVictory(static_cast<int>(enemies.size()));
        if (stageWon) {
            if (mode == GameMode::Story) {
                applyStoryIntermission();
            } else {
                advanceRunProgress();
            }
        }
    };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                stopGameplayMusic();
                stopPauseMusic();
                window.close();
                return StateId::Exit;
            }

            if (event.type == sf::Event::Resized) {
                applyViewToWindow();
            }

            if (event.type == sf::Event::KeyPressed &&
                (event.key.code == sf::Keyboard::D)) {
                godMode = !godMode;
            }

            if (MatchFlowController::canAdvanceSimulation(flowState) &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
                const sf::Vector2f worldPos = window.mapPixelToCoords(pixel, worldView);
                const InGameInputAction action = InGameInputHandler::interpretLeftClick({
                    true,
                    flowState.awaitingStoryAcknowledgement,
                    StoryBriefingView::panelRect(window.getSize()).contains(
                        {static_cast<float>(pixel.x), static_cast<float>(pixel.y)}),
                    pauseButtonRect(window.getSize()).contains(
                        {static_cast<float>(pixel.x), static_cast<float>(pixel.y)}),
                    window.getSize(),
                    worldView.getViewport(),
                    pixel,
                    selectedCardAtPixel(pixel, window.getSize(), cfg),
                    defenseSlotAtWorldPoint(defenses, worldPos),
                    false,
                });

                if (action.type == InGameInputActionType::AcknowledgeStory) {
                        flowState.awaitingStoryAcknowledgement = false;
                        clock.restart();
                        accumulator = 0.f;
                    continue;
                }

                if (action.type == InGameInputActionType::Pause) {
                    flowState.paused = true;
                    stopGameplayMusic();
                    startPauseMusic();
                    clock.restart();
                    continue;
                }

                if (action.type == InGameInputActionType::RepairCore) {
                    tryRepairCore();
                    continue;
                }

                if (action.type == InGameInputActionType::UseLaneBoost) {
                    tryUseLaneBoostCommand();
                    continue;
                }

                if (action.type == InGameInputActionType::UseAbsorption) {
                    useAbsorptionIndex(action.index);
                    continue;
                }

                if (action.type == InGameInputActionType::SelectDefenseCard && action.defenseType) {
                    selectedDefenseType = *action.defenseType;
                    continue;
                }

                if (action.type == InGameInputActionType::BuildDefense && action.index >= 0) {
                    selectedSlot = action.index;
                    syncSelectedCardWithSlot();
                    tryBuildDefense(defenses[action.index], selectedDefenseType);
                }
            }

            if (MatchFlowController::canAdvanceSimulation(flowState) &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Right) {
                const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
                const sf::Vector2f worldPos = window.mapPixelToCoords(pixel, worldView);
                const int clickedSlot = defenseSlotAtWorldPoint(defenses, worldPos);
                const InGameInputAction action = InGameInputHandler::interpretRightClick({
                    true,
                    false,
                    false,
                    false,
                    window.getSize(),
                    worldView.getViewport(),
                    pixel,
                    std::nullopt,
                    clickedSlot,
                    clickedSlot >= 0 && defenses[static_cast<std::size_t>(clickedSlot)].placed,
                });
                if (action.type == InGameInputActionType::SellDefense && action.index >= 0) {
                    trySellDefense(action.index);
                    continue;
                }
                if (action.type == InGameInputActionType::SelectSlot && action.index >= 0) {
                    selectedSlot = action.index;
                }
            }

            if (flowState.intermission &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                if (flowState.awaitingIntermissionConfirmation) {
                    flowState.awaitingIntermissionConfirmation = false;
                    flowState.showingStoryIntermissionReport = true;
                    continue;
                }

                if (flowState.showingStoryIntermissionReport) {
                    if (flowState.pendingStoryVictory) {
                        stopGameplayMusic();
                        stopPauseMusic();
                        playWinSound();
                        MatchFlowController::finalizeVictory(flowState,
                                                             mode,
                                                             core.stability,
                                                             cfg.coreStabilityStart,
                                                             stats.defensesBuilt,
                                                             stats.coreHits,
                                                             infiniteTier,
                                                             stats.score);
                        continue;
                    }

                    if (flowState.pendingStoryDefeat) {
                        stopGameplayMusic();
                        stopPauseMusic();
                        MatchFlowController::finalizeDefeat(flowState, mode, infiniteTier);
                        continue;
                    }

                    flowState.showingStoryIntermissionReport = false;
                    continue;
                }

                MatchFlowController::configurePauseOverlay(flowState, menuPausa);
                const sf::Vector2f point(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                const int buttonIndex = menuPausa.buttonAt(point, window.getSize());

                if (buttonIndex == 0) {
                    if (flowState.pendingStoryVictory) {
                        stopGameplayMusic();
                        stopPauseMusic();
                        playWinSound();
                        MatchFlowController::finalizeVictory(flowState,
                                                             mode,
                                                             core.stability,
                                                             cfg.coreStabilityStart,
                                                             stats.defensesBuilt,
                                                             stats.coreHits,
                                                             infiniteTier,
                                                             stats.score);
                        continue;
                    }

                    if (flowState.pendingStoryDefeat) {
                        stopGameplayMusic();
                        stopPauseMusic();
                        MatchFlowController::finalizeDefeat(flowState, mode, infiniteTier);
                        continue;
                    }

                    ++storyStage;
                    stats.highestStoryStage = std::max(stats.highestStoryStage, storyStage + 1);
                    awardTierBonus();
                    configureStage(true);
                    startGameplayMusic();
                    clock.restart();
                    accumulator = 0.f;
                    continue;
                }

                if (buttonIndex == 1) {
                    stopGameplayMusic();
                    stopPauseMusic();
                    return StateId::MainMenu;
                }
            }

            if ((flowState.gameOver || flowState.victory) &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                MatchFlowController::configurePauseOverlay(flowState, menuPausa);
                const sf::Vector2f point(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                const int buttonIndex = menuPausa.buttonAt(point, window.getSize());

                if (buttonIndex == 0) {
                    stopGameplayMusic();
                    stopPauseMusic();
                    resetSimulation();
                    continue;
                }

                if (buttonIndex == 1) {
                    stopGameplayMusic();
                    stopPauseMusic();
                    return StateId::MainMenu;
                }
            }

            if (flowState.paused &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                MatchFlowController::configurePauseOverlay(flowState, menuPausa);
                const sf::Vector2f point(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                const int buttonIndex = menuPausa.buttonAt(point, window.getSize());

                if (buttonIndex == 0) {
                    flowState.paused = false;
                    stopPauseMusic();
                    startGameplayMusic();
                    clock.restart();
                    continue;
                }

                if (buttonIndex == 1) {
                    stopGameplayMusic();
                    stopPauseMusic();
                    resetSimulation();
                    continue;
                }

                if (buttonIndex == 2) {
                    stopGameplayMusic();
                    stopPauseMusic();
                    return StateId::MainMenu;
                }
            }
        }

        const float frameDt = clampf(clock.restart().asSeconds(), 0.f, 0.1f);
        easterEggBannerTimer = std::max(0.f, easterEggBannerTimer - frameDt);
        if (MatchFlowController::canAdvanceSimulation(flowState)) {
            accumulator = std::min(accumulator + frameDt, kFixedUpdateDt * 4.f);
            while (accumulator >= kFixedUpdateDt) {
                updateSimulation(kFixedUpdateDt);
                accumulator -= kFixedUpdateDt;
                if (!MatchFlowController::canAdvanceSimulation(flowState)) {
                    break;
                }
            }
        }
        const float alpha = kFixedUpdateDt > 0.f ? clampf(accumulator / kFixedUpdateDt, 0.f, 1.f) : 1.f;

        graphicsFacade.beginFrame(window, sf::Color(20, 20, 20));
        window.setView(worldView);
        level.tileMap.draw(window);

        for (const auto& defense : defenses) {
            if (defense.placed) {
                if (defense.animation.loaded) {
                    drawAnimation(window, defense.animation, defense.getPosition(), defense.getSize(), false);
                } else {
                    sf::RectangleShape defenseShape(defense.getSize());
                    defenseShape.setPosition(defense.getPosition());
                    defenseShape.setFillColor(sf::Color(180, 180, 70));
                    window.draw(defenseShape);
                }

                const float healthWidth = defense.getSize().x * (defense.hp / defense.maxHp);
                sf::RectangleShape hpBar({healthWidth, 6.f});
                hpBar.setPosition(defense.getPosition().x, defense.getPosition().y - 10.f);
                hpBar.setFillColor(sf::Color(90, 220, 90));
                window.draw(hpBar);

                if (isLaneBoostActive(defense.laneId)) {
                    sf::RectangleShape boostBar({defense.getSize().x, 4.f});
                    boostBar.setPosition(defense.getPosition().x, defense.getPosition().y - 16.f);
                    boostBar.setFillColor(sf::Color(120, 220, 255));
                    window.draw(boostBar);
                }
            }
        }

        for (const auto& enemy : enemies) {
            const sf::Vector2f enemyRenderPos = enemy.interpolatedPosition(alpha);
            const SpriteAnimation& animation = enemy.blocked && !isLaneChanging(enemy) && enemy.attackAnimation.loaded
                                                   ? enemy.attackAnimation
                                                   : enemy.moveAnimation;
            const sf::Color damageTint = enemyDamageFlashTint(enemy.damageFlashTimer);

            if (animation.loaded) {
                drawAnimation(window, animation, enemyRenderPos, enemy.getSize(), true, true, damageTint);
            } else {
                sf::RectangleShape enemyShape(enemy.getSize());
                enemyShape.setPosition(enemyRenderPos);
                sf::Color baseColor = sf::Color::White;
                switch (enemy.type) {
                    case EnemyType::Standard:
                        baseColor = sf::Color(60, 180, 75);
                        break;
                    case EnemyType::Fast:
                        baseColor = sf::Color(0, 120, 255);
                        break;
                    case EnemyType::Heavy:
                        baseColor = sf::Color(245, 130, 48);
                        break;
                    case EnemyType::Adaptive:
                        baseColor = sf::Color(180, 80, 220);
                        break;
                }
                enemyShape.setFillColor(multiplyColor(baseColor, damageTint));
                window.draw(enemyShape);
            }
        }

        for (const auto& projectile : projectiles) {
            sf::RectangleShape projectileShape(projectile.getSize());
            projectileShape.setPosition(projectile.interpolatedPosition(alpha));
            projectileShape.setFillColor(
                projectile.owner == ProjectileOwner::Enemy ? sf::Color(255, 120, 120) : sf::Color::White);
            window.draw(projectileShape);
        }

        std::array<int, 4> absorptionCharges{
            absorption.charges(EnemyType::Standard),
            absorption.charges(EnemyType::Fast),
            absorption.charges(EnemyType::Heavy),
            absorption.charges(EnemyType::Adaptive),
        };
        std::array<int, 4> absorptionRemaining{
            absorption.remainingToUnlock(EnemyType::Standard),
            absorption.remainingToUnlock(EnemyType::Fast),
            absorption.remainingToUnlock(EnemyType::Heavy),
            absorption.remainingToUnlock(EnemyType::Adaptive),
        };
        const bool showingStoryReport = flowState.showingStoryIntermissionReport;
        const std::string waveStatusLabel = flowState.awaitingStoryAcknowledgement
                                                ? "ESPERA"
                                                : waveManager.isPreparing()
                                                      ? "PREP " +
                                                            std::to_string(waveManager.preparationSecondsRemaining())
                                                      : "W " + std::to_string(waveManager.currentWaveNumber()) + "/" +
                                                            std::to_string(waveManager.totalWaveCount());

        {
            hud.draw(window,
                     HUDState{
                         economy.getCredits(),
                         core.stability,
                         cfg.coreStabilityStart,
                         currentLaneId(),
                         static_cast<int>(level.lanes.size()),
                         !defenses.empty() ? defenses[selectedSlot].placed : false,
                         selectedDefenseType,
                         activeDefenseCount(defenses),
                         currentDefenseLimit(),
                         waveManager.currentWaveNumber(),
                         waveManager.totalWaveCount(),
                         waveStatusLabel,
                         static_cast<int>(enemies.size()),
                         stats.score,
                         modeLabel(),
                         stageLabel(mode, storyStage, kStoryStageCount, infiniteTier),
                         (bonusRepairCharges > 0 && core.stability < cfg.coreStabilityStart && repairCooldown <= 0.f) ||
                             (difficulty.allowCoreRepair() && core.stability < cfg.coreStabilityStart && repairCooldown <= 0.f),
                         currentRepairCost(),
                         bonusRepairCharges,
                         !isLaneBoostActive(currentLaneId()),
                         cfg.laneBoostCost,
                         bonusLaneBoostCharges,
                         static_cast<int>(std::ceil(std::max(0.f, laneBoostTimers[static_cast<std::size_t>(currentLaneId())]))),
                         absorptionCharges,
                         absorptionRemaining,
                         flowState.intermission &&
                                 (!flowState.awaitingIntermissionConfirmation || flowState.showingStoryIntermissionReport)
                             ? ""
                             : flowState.bannerTitle,
                         flowState.intermission &&
                                 (!flowState.awaitingIntermissionConfirmation || flowState.showingStoryIntermissionReport)
                             ? ""
                             : flowState.bannerSubtitle,
                         flowState.gameOver,
                         flowState.victory,
                         godMode,
                     });
        }

        const bool showStoryTransmission = overlayFontLoaded &&
                                           mode == GameMode::Story &&
                                           (waveManager.isPreparing() || flowState.awaitingStoryAcknowledgement ||
                                            flowState.showingStoryIntermissionReport) &&
                                           !flowState.gameOver &&
                                           !flowState.victory &&
                                           !flowState.paused &&
                                           (!flowState.intermission || flowState.showingStoryIntermissionReport);
        if (showStoryTransmission) {
            window.setView(window.getDefaultView());
            const StoryBeat beat =
                flowState.showingStoryIntermissionReport
                    ? (flowState.pendingStoryDefeat ? storyDefeatBeatForStage(storyStage)
                                                    : storyIntermissionBeatForStage(storyStage))
                    : storyBeatForWave(storyStage,
                                       waveManager.currentWaveNumber(),
                                       waveManager.totalWaveCount(),
                                       flowState.awaitingStoryAcknowledgement);
            storyBriefingView.draw(
                window, beat, flowState.awaitingStoryAcknowledgement || flowState.showingStoryIntermissionReport);
        }

        if (!showingStoryReport && easterEggBannerTimer > 0.f && overlayFontLoaded) {
            window.setView(window.getDefaultView());

            sf::RectangleShape banner({420.f, 76.f});
            banner.setOrigin(banner.getSize().x * 0.5f, banner.getSize().y * 0.5f);
            banner.setPosition(static_cast<float>(window.getSize().x) * 0.5f,
                               static_cast<float>(window.getSize().y) * 0.5f - 18.f);
            banner.setFillColor(sf::Color(10, 22, 35, 232));
            banner.setOutlineThickness(3.f);
            banner.setOutlineColor(sf::Color(255, 196, 87, 235));
            window.draw(banner);

            sf::Text easterEggText;
            easterEggText.setFont(overlayFont);
            easterEggText.setString("EASTER EGG ENCONTRADO");
            easterEggText.setCharacterSize(26);
            easterEggText.setFillColor(sf::Color(255, 236, 170));
            const sf::FloatRect bounds = easterEggText.getLocalBounds();
            easterEggText.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
            easterEggText.setPosition(banner.getPosition());
            window.draw(easterEggText);
        }

        if (!showingStoryReport &&
            MatchFlowController::canAdvanceSimulation(flowState) &&
            !flowState.awaitingStoryAcknowledgement &&
            overlayFontLoaded) {
            window.setView(window.getDefaultView());

            const sf::FloatRect rect = pauseButtonRect(window.getSize());
            sf::RectangleShape button({rect.width, rect.height});
            button.setPosition(rect.left, rect.top);
            button.setFillColor(sf::Color(10, 22, 35, 226));
            button.setOutlineThickness(2.f);
            button.setOutlineColor(sf::Color(120, 220, 255, 220));
            window.draw(button);

            sf::RectangleShape bar({6.f, 20.f});
            bar.setFillColor(sf::Color::White);
            bar.setPosition(rect.left + 11.f, rect.top + 11.f);
            window.draw(bar);
            bar.setPosition(rect.left + rect.width - 17.f, rect.top + 11.f);
            window.draw(bar);
        }

        if (!showingStoryReport &&
            ((flowState.intermission && !flowState.awaitingIntermissionConfirmation) ||
             flowState.gameOver || flowState.victory) && overlayFontLoaded) {
            window.setView(window.getDefaultView());
            MatchFlowController::configurePauseOverlay(flowState, menuPausa);
            menuPausa.draw(window);
        }

        if (flowState.paused && overlayFontLoaded) {
            window.setView(window.getDefaultView());
            MatchFlowController::configurePauseOverlay(flowState, menuPausa);
            menuPausa.draw(window);
        }

        graphicsFacade.endFrame(window);
    }

    return StateId::Exit;
}
