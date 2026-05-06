cmake_minimum_required(VERSION 3.16)

set(LAUNCHER_SOURCES
    main.cpp
)

if(WIN32)
    list(APPEND LAUNCHER_SOURCES "${CMAKE_SOURCE_DIR}/assets/icon.rc")
endif()

add_executable(SpaceRockLauncher
    ${LAUNCHER_SOURCES}
)

set_target_properties(SpaceRockLauncher PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(WIN32)
    set_target_properties(SpaceRockLauncher PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(SpaceRockLauncher PRIVATE /ENTRY:mainCRTStartup)
    endif()
endif()

find_package(SFML COMPONENTS Graphics Window System REQUIRED)

target_link_libraries(SpaceRockLauncher PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(WIN32)
    target_link_libraries(SpaceRockLauncher PRIVATE winhttp)
endif()

if(MSVC)
    target_compile_options(SpaceRockLauncher PRIVATE /utf-8)
endif()
