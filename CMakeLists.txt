set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
cmake_minimum_required(VERSION 3.16)
project(SpaceRockBreaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SRB_AUTO_BUMP_VERSION_ON_BUILD "Bij build: version.txt-patch + Inno AppVersion automatisch verhogen" ON)

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
    src/NotificationSystem.cpp
    src/UnlockSystem.cpp
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
    src/NotificationSystem.h
    src/UnlockSystem.h
)

# ─── Windows icoon in .exe inbakken ───────────────────────────
if(WIN32)
    set(APP_ICON_RC "${CMAKE_SOURCE_DIR}/assets/icon.rc")
    file(WRITE ${APP_ICON_RC}
        "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/assets/icon.ico\"\n")
    list(APPEND SOURCES ${APP_ICON_RC})
endif()

# ─── Executable ───────────────────────────────────────────────
add_executable(SpaceRockBreaker ${SOURCES} ${HEADERS})

# Geen zwart console-venster bij start (Windows GUI-subsystem; main() blijft entry).
if(WIN32)
    set_target_properties(SpaceRockBreaker PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(SpaceRockBreaker PRIVATE /ENTRY:mainCRTStartup)
    endif()
endif()

target_include_directories(SpaceRockBreaker PRIVATE src)

target_link_libraries(SpaceRockBreaker PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    ImGui-SFML::ImGui-SFML
)

# MSVC: string literals als UTF-8 (voorkomt kapotte tekens zoals -, x, bullets in UI)
if(MSVC)
    target_compile_options(SpaceRockBreaker PRIVATE /utf-8)
endif()

# ─── Windows: copy SFML DLLs next to .exe ─────────────────────
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

# Zelfde semver als in repo/zip: hoofdmenu leest applicationDirectory()/version.txt
add_custom_command(TARGET SpaceRockBreaker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/version.txt"
        "$<TARGET_FILE_DIR:SpaceRockBreaker>/version.txt"
    COMMENT "version.txt naast SpaceRockBreaker.exe (runtime versie-label)"
)

add_subdirectory(launcher)

if(SRB_AUTO_BUMP_VERSION_ON_BUILD)
    add_custom_target(srb_bump_app_version ALL
        COMMAND ${CMAKE_COMMAND}
            "-DSRB_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/bump_app_version.cmake"
        COMMENT "Auto-bump semver in version.txt + installer.iss (SRB_AUTO_BUMP_VERSION_ON_BUILD)"
    )

    foreach(_tgt IN ITEMS SpaceRockBreaker SpaceRockLauncher)
        add_dependencies(${_tgt} srb_bump_app_version)
    endforeach()
endif()