#########################################################################
#
#	BRL-CAD
#
#	Copyright (c) 1997-2010 United States Government as represented by
#	the U.S. Army Research Laboratory.
#
#	This library is free software; you can redistribute it and/or
#	modify it under the terms of the GNU Lesser General Public License
#	version 2.1 as published by the Free Software Foundation.
#
#	This library is distributed in the hope that it will be useful, but
#	WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#	Lesser General Public License for more details.
#
#	You should have received a copy of the GNU Lesser General Public
#	License along with this file; see the file named COPYING for more
#	information.
#
#########################################################################
#	@file /cmake/FindBRLCAD.cmake
#
# 	Try to find brlcad libraries.
# 	Once done, this will define:
#
#  	BRLCAD_FOUND - system has BRL-CAD
#  	BRLCAD_VERSION - the BRL-CAD version string
#  	BRLCAD_INCLUDE_DIRS - the BRL-CAD include directories
#  	BRLCAD_LIBRARIES - link these to use the BRL-CAD Libraries
#
#
#	$Revision:  $
#	$Author:  $
#
#########################################################################

message(STATUS "\tSearching for BRLCAD...")

if(RT3_VERBOSE_CMAKE_OUTPUT)
  message(STATUS "\t\tEnviornment Variable 'PATH': $ENV{PATH}")
endif(RT3_VERBOSE_CMAKE_OUTPUT)

#First, find the install directories.
if(BRLCAD_BASE_DIR)
  message(STATUS "\t\t Using BRLCAD_BASE_DIR...")
  #if BRLCAD_BASE_DIR is set, then this makes it easy!
  set(BRLCAD_BIN_DIR "${BRLCAD_BASE_DIR}/bin")
  set(BRLCAD_INC_DIRS "${BRLCAD_BASE_DIR}/include" "${BRLCAD_BASE_DIR}/include/brlcad" "${BRLCAD_BASE_DIR}/include/openNURBS" "${BRLCAD_BASE_DIR}/include/tie")
  set(BRLCAD_LIB_DIR "${BRLCAD_BASE_DIR}/lib")
else(BRLCAD_BASE_DIR)
  message(STATUS "\t\t Searching for BRLCAD components...")
  #if BRLCAD_BASE_DIR is NOT set, then search for files KNOWN to be in the BRLCAD installation.

  #Find /bin
  find_path(BRLCAD_BIN_DIR brlcad-config "$ENV{PATH}")
  if(NOT BRLCAD_BIN_DIR)
    message(STATUS "\t\t Could not find BRLCAD bin directory anywhere in paths: $ENV{PATH}")
    return()
  endif(NOT BRLCAD_BIN_DIR)

  #Find include directories (aka more than one)
  set(HEADERS_TO_SEARCH_FOR brlcad/bu.h bu.h opennurbs.h )

  foreach (tHead ${HEADERS_TO_SEARCH_FOR})

    find_path(_HEADER_DIR_${tHead} ${tHead} "$ENV{PATH}")

    if(RT3_VERBOSE_CMAKE_OUTPUT)
      if(_HEADER_DIR_${tHead})
	message(STATUS "\t\t\t'${tHead}' was found: ${_HEADER_DIR_${tHead}}")
      else(_HEADER_DIR_${tHead})
	message(STATUS "\t\t\t'${tHead}' was NOT found.")
      endif(_HEADER_DIR_${tHead})
    endif(RT3_VERBOSE_CMAKE_OUTPUT)

    if(_HEADER_DIR_${tHead})
      set(BRLCAD_INC_DIRS ${BRLCAD_INC_DIRS} ${_HEADER_DIR_${tHead}})
      set(BRLCAD_HEADERS_FOUND ${BRLCAD_HEADERS_FOUND} ${tHead})
    else(_HEADER_DIR_${tHead})
      set(BRLCAD_HEADERS_NOTFOUND ${BRLCAD_HEADERS_NOTFOUND} ${tHead})
    endif(_HEADER_DIR_${tHead})

  endforeach (tHead ${HEADERS_TO_SEARCH_FOR})

  if(NOT BRLCAD_INC_DIRS)
    message(STATUS "\t\tCould not find BRLCAD include directories anywhere in paths: $ENV{PATH}")
    return()
  endif(NOT BRLCAD_INC_DIRS)

  #Find /lib
  if (UNIX)
    set(LIB_EXT ".so")
  else (UNIX)
    set(LIB_EXT ".lib")
  endif(UNIX)

  find_path(BRLCAD_LIB_DIR "libbu${LIB_EXT}")

  if(NOT BRLCAD_LIB_DIR)
    message(STATUS "\t\tCould not find brlcad library directory in: $ENV{PATH}")
    return()
  endif(NOT BRLCAD_LIB_DIR)

endif(BRLCAD_BASE_DIR)



#Attempt to get brlcad version.
find_program(BRLCAD_CONFIGEXE brlcad-config)
if(BRLCAD_CONFIGEXE)
  execute_process(COMMAND ${BRLCAD_CONFIGEXE} --version OUTPUT_VARIABLE BRLCAD_VERSION)
  string(STRIP ${BRLCAD_VERSION} BRLCAD_VERSION)

  if(BRLCAD_VERSION)
    string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" BRLCAD_MAJOR_VERSION "${BRLCAD_VERSION}")
    string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" BRLCAD_MINOR_VERSION "${BRLCAD_VERSION}")
    string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" BRLCAD_PATCH_VERSION "${BRLCAD_VERSION}")
    set(BRLCAD_VERSION_FOUND TRUE)
  elseif(BRLCAD_VERSION)
    message(STATUS "\t\t'brlcad-config --version' was found and executed, but produced no output.")
    set(BRLCAD_VERSION_FOUND FALSE)
  endif(BRLCAD_VERSION)

else(BRLCAD_CONFIGEXE)
  message(STATUS "\t\tCould not locate 'brlcad-config'.")
  set(BRLCAD_VERSION_FOUND FALSE)
endif(BRLCAD_CONFIGEXE)

#TODO need to make the BRLCAD version checking a requirement for coreInterface, but nothing else.
#TODO figure out why brlcad-config isn't present on Windows.

##########################################################################
#Search for Libs

set(LIBS_TO_SEARCH_FOR
  analyze
  bn
  brlcad
  bu
  cursor
  dm
  exppp
  express
  fb
  fft
  gcv
  ged
  icv
  multispectral
  openNURBS
  optical
  orle
  pkg
  png14
  png
  regex
  render
  rt
  stepcore
  stepdai
  stepeditor
  steputils
  sysv
  termio
  utahrle
  wdb
  )

message(STATUS "BRLCAD_LIB_DIR: ${BRLCAD_LIB_DIR}")
message(STATUS "LIB_EXT: ${LIB_EXT}")

foreach (tlib ${LIBS_TO_SEARCH_FOR})
  find_library(_BRLCAD_LIBRARY_${tlib} ${tlib} ${BRLCAD_LIB_DIR} )

  if(RT3_VERBOSE_CMAKE_OUTPUT)
    if(_BRLCAD_LIBRARY_${tlib})
      message(STATUS "\t\t'${tlib}' was found: ${_BRLCAD_LIBRARY_${tlib}}")
    else(_BRLCAD_LIBRARY_${tlib})
      message(STATUS "\t\t'${tlib}' was NOT found.")
    endif(_BRLCAD_LIBRARY_${tlib})
  endif(RT3_VERBOSE_CMAKE_OUTPUT)

  if(_BRLCAD_LIBRARY_${tlib})
    set(BRLCAD_LIBRARIES ${BRLCAD_LIBRARIES} ${_BRLCAD_LIBRARY_${tlib}})
    set(BRLCAD_LIBRARIES_FOUND ${BRLCAD_LIBRARIES_FOUND} ${tlib})
  else(_BRLCAD_LIBRARY_${tlib})
    set(BRLCAD_LIBRARIES_NOTFOUND ${BRLCAD_LIBRARIES_NOTFOUND} ${tlib})
  endif(_BRLCAD_LIBRARY_${tlib})

endforeach (tlib ${LIBS_TO_SEARCH_FOR})

##########################################################################
#Print status
message(STATUS "")
message(STATUS "\t\t Discovered BRLCAD Version ${BRLCAD_VERSION}")
message(STATUS "\t\t BRLCAD_BIN_DIR:     ${BRLCAD_BIN_DIR}")
message(STATUS "\t\t BRLCAD_INC_DIRS:    ${BRLCAD_INC_DIRS}")
message(STATUS "\t\t BRLCAD_LIB_DIR:     ${BRLCAD_LIB_DIR}")
message(STATUS "\t\t BRLCAD_LIBRARIES_FOUND:    ${BRLCAD_LIBRARIES_FOUND}")
message(STATUS "\t\t BRLCAD_LIBRARIES_NOTFOUND: ${BRLCAD_LIBRARIES_NOTFOUND}")
message(STATUS "\t\t BRLCAD_CONFIGEXE: ${BRLCAD_CONFIGEXE}")
if(BRLCAD_CONFIGEXE)
  message(STATUS "\t\t\t BRLCAD_MAJOR_VERSION: ${BRLCAD_MAJOR_VERSION}")
  message(STATUS "\t\t\t BRLCAD_MINOR_VERSION: ${BRLCAD_MINOR_VERSION}")
  message(STATUS "\t\t\t BRLCAD_PATCH_VERSION: ${BRLCAD_PATCH_VERSION}")
endif(BRLCAD_CONFIGEXE)
message(STATUS "")

#Set found flag
set(BRLCAD_FOUND TRUE)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
