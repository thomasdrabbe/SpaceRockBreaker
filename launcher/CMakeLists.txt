cmake_minimum_required(VERSION 3.16)

add_executable(SpaceRockLauncher
    main.cpp
)

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
    target_link_libraries(SpaceRockLauncher PRIVATE winhttp)
endif()

if(MSVC)
    target_compile_options(SpaceRockLauncher PRIVATE /utf-8)
endif()
