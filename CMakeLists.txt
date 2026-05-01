set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
cmake_minimum_required(VERSION 3.16)
project(SpaceRockBreaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── SFML ─────────────────────────────────────────────────────

find_package(SFML COMPONENTS Graphics Window System Audio REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

# ─── Sources ──────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/GameState.cpp
    src/Particle.cpp
    src/Asteroid.cpp
    src/Bullet.cpp
    src/Turret.cpp
    src/Ore.cpp
    src/KeyPickup.cpp
    src/Plinko.cpp
    src/Shop.cpp
    src/ChestScreen.cpp
    src/MiningScreen.cpp
	  src/Player.cpp
    src/Game.cpp
    src/SoundHub.cpp
)

set(HEADERS
    src/Constants.h
    src/Utils.h
    src/GameState.h
    src/Particle.h
    src/Asteroid.h
    src/Bullet.h
    src/Turret.h
    src/Ore.h
    src/KeyPickup.h
    src/Plinko.h
    src/Shop.h
    src/ChestScreen.h
    src/MiningScreen.h
	  src/Player.h
    src/Game.h
    src/SoundHub.h
)

# ─── Executable ───────────────────────────────────────────────
add_executable(SpaceRockBreaker ${SOURCES} ${HEADERS})

target_include_directories(SpaceRockBreaker PRIVATE src)

target_link_libraries(SpaceRockBreaker PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
  SFML::Audio
  ImGui-SFML::ImGui-SFML
)

# ─── Windows: copy SFML (incl. audio) DLLs next to .exe ───────
# Gebruikt dezelfde binaries als vcpkg/CMake aan de targets koppelt
# (geen vaste SFML 2.6-paden meer).
if(WIN32)
    add_custom_command(TARGET SpaceRockBreaker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SFML::System>"
            "$<TARGET_FILE:SFML::Window>"
            "$<TARGET_FILE:SFML::Graphics>"
            "$<TARGET_FILE:SFML::Audio>"
            "$<TARGET_FILE_DIR:SpaceRockBreaker>"
        COMMENT "SFML runtime DLLs naar outputmap kopiëren"
    )
endif()

# ─── Copy assets folder next to .exe ──────────────────────────
add_custom_command(TARGET SpaceRockBreaker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SpaceRockBreaker>/assets"
)
