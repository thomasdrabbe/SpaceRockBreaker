cmake_minimum_required(VERSION 3.16)

set(LAUNCHER_SOURCES
    main.cpp
)

if(WIN32)
    list(APPEND LAUNCHER_SOURCES "${CMAKE_SOURCE_DIR}/assets/icon.rc")
endif()

if(WIN32)
    add_executable(SpaceRockLauncher WIN32 ${LAUNCHER_SOURCES})
    if(MSVC)
        target_link_options(SpaceRockLauncher PRIVATE /ENTRY:mainCRTStartup)
    endif()
else()
    add_executable(SpaceRockLauncher ${LAUNCHER_SOURCES})
endif()

set_target_properties(SpaceRockLauncher PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

find_package(SFML COMPONENTS Graphics Window System REQUIRED)

target_link_libraries(SpaceRockLauncher PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(WIN32)
    target_link_libraries(SpaceRockLauncher PRIVATE winhttp shell32)
endif()

if(MSVC)
    target_compile_options(SpaceRockLauncher PRIVATE /utf-8)
endif()

# Launcher toont / vergelijkt lokale semver naast eigen .exe
if(WIN32)
    add_custom_command(TARGET SpaceRockLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/version.txt"
            "$<TARGET_FILE_DIR:SpaceRockLauncher>/version.txt"
        COMMENT "version.txt naast SpaceRockLauncher.exe"
    )
endif()
