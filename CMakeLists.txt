# vcpkg-toolchain kiezen vóór project():
# - Windows: C:\vcpkg of %VCPKG_ROOT%
# - macOS/Linux: ~/vcpkg of $VCPKG_ROOT
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(_SRB_VCPKG_CANDIDATES "")
    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        list(APPEND _SRB_VCPKG_CANDIDATES "$ENV{VCPKG_ROOT}")
    endif()
    if(WIN32)
        list(APPEND _SRB_VCPKG_CANDIDATES "C:/vcpkg")
    elseif(APPLE)
        if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
            list(APPEND _SRB_VCPKG_CANDIDATES "$ENV{HOME}/vcpkg")
        endif()
        list(APPEND _SRB_VCPKG_CANDIDATES "/opt/homebrew/share/vcpkg")
    else()
        if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
            list(APPEND _SRB_VCPKG_CANDIDATES "$ENV{HOME}/vcpkg")
        endif()
        list(APPEND _SRB_VCPKG_CANDIDATES "/usr/local/share/vcpkg")
    endif()
    foreach(_SRB_VCPKG_ROOT IN LISTS _SRB_VCPKG_CANDIDATES)
        set(_SRB_TOOLCHAIN "${_SRB_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        if(EXISTS "${_SRB_TOOLCHAIN}")
            set(CMAKE_TOOLCHAIN_FILE "${_SRB_TOOLCHAIN}"
                CACHE FILEPATH "vcpkg toolchain")
            break()
        endif()
    endforeach()
endif()

cmake_minimum_required(VERSION 3.16)
project(SpaceRockBreaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_MACOSX_RPATH ON)
endif()

option(SRB_AUTO_BUMP_VERSION_ON_BUILD
    "Bij build: version.txt-patch + Inno AppVersion automatisch verhogen (Windows installer)"
    ${WIN32})

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
    src/RunFlowController.cpp
    src/UiFlowController.cpp
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
    src/RunFlowController.h
    src/UiFlowController.h
)

# ─── Windows icoon in .exe inbakken ───────────────────────────
if(WIN32)
    set(APP_ICON_RC "${CMAKE_SOURCE_DIR}/assets/icon.rc")
    file(WRITE ${APP_ICON_RC}
        "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/assets/icon.ico\"\n")
    list(APPEND SOURCES ${APP_ICON_RC})
endif()

# ─── Executable ───────────────────────────────────────────────
# WIN32 = Windows GUI-subsystem (geen conhost); main() blijft entry (CMake/MSVC).
if(WIN32)
    add_executable(SpaceRockBreaker WIN32 ${SOURCES} ${HEADERS})
    if(MSVC)
        target_link_options(SpaceRockBreaker PRIVATE /ENTRY:mainCRTStartup)
    endif()
else()
    add_executable(SpaceRockBreaker ${SOURCES} ${HEADERS})
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

# ─── macOS: copy SFML dylibs next to binary (zelfde idee als DLLs op Windows) ─
if(APPLE)
    add_custom_command(TARGET SpaceRockBreaker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SFML::System>"
            "$<TARGET_FILE:SFML::Window>"
            "$<TARGET_FILE:SFML::Graphics>"
            "$<TARGET_FILE:SFML::Audio>"
            "$<TARGET_FILE_DIR:SpaceRockBreaker>"
        COMMENT "SFML runtime dylibs naast SpaceRockBreaker kopiëren"
    )
endif()

# ─── Copy assets folder next to executable ────────────────────
add_custom_command(TARGET SpaceRockBreaker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SpaceRockBreaker>/assets"
)

add_custom_command(TARGET SpaceRockBreaker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/version.txt"
        "$<TARGET_FILE_DIR:SpaceRockBreaker>/version.txt"
    COMMENT "version.txt naast executable (runtime versie-label)"
)

if(APPLE)
    add_custom_target(srb_macos_app_bundle
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/make-macos-app.sh"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS SpaceRockBreaker
        COMMENT "Space Rock Breaker.app in build-macos"
    )
endif()

# Windows-only auto-updater launcher
if(WIN32)
    add_subdirectory(launcher)
endif()

if(SRB_AUTO_BUMP_VERSION_ON_BUILD)
    add_custom_target(srb_bump_app_version ALL
        COMMAND ${CMAKE_COMMAND}
            "-DSRB_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/bump_app_version.cmake"
        COMMENT "Auto-bump semver in version.txt + installer.iss (SRB_AUTO_BUMP_VERSION_ON_BUILD)"
    )

    add_dependencies(SpaceRockBreaker srb_bump_app_version)
    if(WIN32)
        add_dependencies(SpaceRockLauncher srb_bump_app_version)
    endif()
endif()
