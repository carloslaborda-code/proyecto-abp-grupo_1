cmake_minimum_required(VERSION 3.16)
project(MiJuego)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
include_directories(include)

add_executable(MiJuego
    main.cpp
    src/core/Config.cpp
    src/core/Game.cpp
    src/states/State.cpp
    src/states/MainMenuState.cpp
    src/states/InGameState.cpp
    src/commands/PlaceDefenseCommand.cpp
    src/commands/RepairCoreCommand.cpp
    src/commands/ActivateAbilityCommand.cpp
    src/input/InGameInputHandler.cpp
    src/entities/Entity.cpp
    src/entities/Enemy.cpp
    src/entities/Defense.cpp
    src/entities/Projectile.cpp
    src/entities/Core.cpp
    src/systems/CollisionSystem.cpp
    src/map/Grid.cpp
    src/map/Lane.cpp
    src/map/TileMap.cpp
    src/map/Level.cpp
    src/factories/DefenseFactory.cpp
    src/factories/EnemyFactory.cpp
    src/factories/LevelFactory.cpp
    src/systems/WaveManager.cpp
    src/systems/EconomySystem.cpp
    src/systems/AbsorptionSystem.cpp
    src/systems/AnimationUtils.cpp
    src/systems/DefenseSystem.cpp
    src/systems/DifficultySystem.cpp
    src/systems/GraphicsFacade.cpp
    src/systems/MatchFlowController.cpp
    src/systems/ProjectileSystem.cpp
    src/systems/TextureManager.cpp
    src/ui/Menu.cpp
    src/ui/MenuPrincipal.cpp
    src/ui/MenuPausa.cpp
    src/ui/HUD.cpp
    src/ui/MainMenu.cpp
    src/ui/StoryBriefingView.cpp
)

add_custom_command(TARGET MiJuego 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MiJuego> ..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
find_library(TINYXML2_LIB tinyxml2 REQUIRED)
find_package(SFML REQUIRED system window graphics network audio)
if (SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    target_link_libraries(MiJuego ${SFML_LIBRARIES})
endif()

target_link_libraries(MiJuego ${TINYXML2_LIB})
