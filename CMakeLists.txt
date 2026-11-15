cmake_minimum_required(VERSION 3.15)
project(tp_opengl CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------
# GLFW est fourni dans extern/glfw : rien a installer sur la machine.
# On desactive tout ce qui ne sert pas au module.
# ---------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# Sur Linux, on s'en tient a X11 : evite d'exiger les outils Wayland.
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
endif()
add_subdirectory(extern/glfw)

# ---------------------------------------------------------------
# Le programme
# ---------------------------------------------------------------
add_executable(app
    src/main.cpp
    extern/glad/glad.c)

target_include_directories(app PRIVATE
    extern/glad/include
    extern/glm
    extern/stb
    src)

find_package(OpenGL REQUIRED)
target_link_libraries(app PRIVATE glfw OpenGL::GL)

# ---------------------------------------------------------------
# Les shaders et les ressources sont recopies a cote de l'executable
# a chaque compilation : les chemins relatifs fonctionnent toujours.
# ---------------------------------------------------------------
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:app>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:app>/assets)

# Visual Studio : demarrer sur app, et se placer dans le bon dossier
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT app)
set_target_properties(app PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:app>)
