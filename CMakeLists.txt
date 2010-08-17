cmake_minimum_required(VERSION 2.6)
project(ISST)
set(ISST_SDL_SOURCES sdl/main.c sdl/event.c)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

#find SDL
FIND_PACKAGE(SDL REQUIRED)
IF (NOT SDL_FOUND)
    MESSAGE(STATUS "\tDid not find SDL!")
ENDIF (NOT SDL_FOUND)
link_libraries(${SDL_LIBRARY} SDLmain)
add_executable(isst_sdl WIN32 MACOSX_BUNDLE ${ISST_SDL_SOURCES})

#Find BRLCAD
FIND_PACKAGE ( BRLCAD REQUIRED )
IF (NOT BRLCAD_FOUND)
    MESSAGE(STATUS "\tDid not find BRLCAD!")
        RETURN()
ENDIF (NOT BRLCAD_FOUND)

