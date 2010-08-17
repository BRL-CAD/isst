cmake_minimum_required(VERSION 2.6)

project(ISST)
set(ISST_SDL_SOURCES sdl/main.c sdl/event.c)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

include (CheckIncludeFiles)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(getopt.h HAVE_GETOPT)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(unistd.h HAVE_UNISTD_H)

#find SDL
find_package(SDL REQUIRED)
if (NOT (SDL_FOUND))
    message(STATUS "\tDid not find SDL!")
endif (NOT (SDL_FOUND))
include_directories(${SDL_INCLUDE_DIR})
link_libraries(${SDL_LIBRARY})
if (NOT APPLE)
	link_libraries(SDLmain)
endif (NOT APPLE)

find_package(OpenGL REQUIRED)
if (NOT OPENGL_FOUND)
    message(STATUS "\tDid not find OpenGL!")
endif (NOT OPENGL_FOUND)
include_directories(${OPENGL_INCLUDE_DIR})
link_libraries(${OPENGL_gl_LIBRARY})

#Find BRLCAD
find_package ( BRLCAD REQUIRED )
if (NOT BRLCAD_FOUND)
    message(STATUS "\tDid not find BRLCAD!")
        RETURN()
endif (NOT BRLCAD_FOUND)
include_directories(${BRLCAD_INC_DIRS})
link_libraries(${_BRLCAD_LIBRARY_bu} ${_BRLCAD_LIBRARY_bn} ${_BRLCAD_LIBRARY_rt} ${_BRLCAD_LIBRARY_tie} ${_BRLCAD_LIBRARY_render})

set_property(DIRECTORY
	PROPERTY COMPILE_DEFINITIONS	HAVE_FCNTL_H=${HAVE_FCNTL_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_GETOPT=${HAVE_GETOPT}
	PROPERTY COMPILE_DEFINITIONS	HAVE_SYS_TIME_H=${HAVE_SYS_TIME_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_UNISTD_H=${HAVE_UNISTD_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_OPENGL)

add_executable(isst_sdl WIN32 MACOSX_BUNDLE ${ISST_SDL_SOURCES})
