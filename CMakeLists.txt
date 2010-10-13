cmake_minimum_required(VERSION 2.6)

project(ISST)
set(ISST_SDL_SOURCES sdl/main.c sdl/event.c)
set(ISST_GTK_SOURCES gtk/gui.c gtk/local_worker.c gtk/main.c gtk/net_worker.c)

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
#if (NOT APPLE)
#	link_libraries(SDLmain)
#endif (NOT APPLE)

find_package(GTK2 2.2 REQUIRED gtk)
if (NOT GTK2_FOUND)
    message(STATUS "\tDid not find GTK+ 2!")
endif (NOT GTK2_FOUND)

find_package(OpenGL REQUIRED)
if (NOT OPENGL_FOUND)
    message(STATUS "\tDid not find OpenGL!")
endif (NOT OPENGL_FOUND)

#Find BRLCAD
find_package ( BRLCAD REQUIRED )
if (NOT BRLCAD_FOUND)
    message(STATUS "\tDid not find BRLCAD!")
        RETURN()
endif (NOT BRLCAD_FOUND)

include_directories(${OPENGL_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${GTK2_INCLUDE_DIRS} ${BRLCAD_INC_DIRS})
set(MYLIBS ${OPENGL_gl_LIBRARY} ${_BRLCAD_LIBRARY_bu} ${_BRLCAD_LIBRARY_bn} ${_BRLCAD_LIBRARY_rt} ${_BRLCAD_LIBRARY_tie} ${_BRLCAD_LIBRARY_render})

add_executable(isst_gtk WIN32 MACOSX_BUNDLE ${ISST_GTK_SOURCES})
target_link_libraries(isst_gtk ${GTK2_LIBRARIES} ${MYLIBS})

add_library(myplugin SHARED sdl/myplugin.c)
target_link_libraries(myplugin ${_BRLCAD_LIBRARY_bu} ${_BRLCAD_LIBRARY_bn} ${_BRLCAD_LIBRARY_rt} ${_BRLCAD_LIBRARY_tie} ${_BRLCAD_LIBRARY_render})

add_executable(isst_sdl WIN32 MACOSX_BUNDLE ${ISST_SDL_SOURCES})
target_link_libraries(isst_sdl ${SDL_LIBRARY} ${MYLIBS})

if(APPLE)
    add_custom_command(TARGET isst_sdl POST_BUILD DEPENDS myplugin isst_sdl COMMAND mkdir -p ./isst_sdl.app/Contents/Resources && install ./libmyplugin.dylib ./isst_sdl.app/Contents/Resources/libmyplugin.dylib DEPENDS)
    add_dependencies(isst_sdl myplugin)
endif(APPLE)

set_property(DIRECTORY
	PROPERTY COMPILE_DEFINITIONS	HAVE_FCNTL_H=${HAVE_FCNTL_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_GETOPT=${HAVE_GETOPT}
	PROPERTY COMPILE_DEFINITIONS	HAVE_SYS_TIME_H=${HAVE_SYS_TIME_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_UNISTD_H=${HAVE_UNISTD_H}
	PROPERTY COMPILE_DEFINITIONS	HAVE_OPENGL)

