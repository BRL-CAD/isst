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

MESSAGE(STATUS "\tSearching for BRLCAD...")

IF(RT3_VERBOSE_CMAKE_OUTPUT)   
    MESSAGE(STATUS "\t\tEnviornment Variable 'PATH': $ENV{PATH}")
ENDIF(RT3_VERBOSE_CMAKE_OUTPUT)  

#First, find the install directories.
IF(BRLCAD_BASE_DIR)
    MESSAGE(STATUS "\t\t Using BRLCAD_BASE_DIR...")
    #if BRLCAD_BASE_DIR is set, then this makes it easy!
    SET(BRLCAD_BIN_DIR "${BRLCAD_BASE_DIR}/bin")
    SET(BRLCAD_INC_DIRS "${BRLCAD_BASE_DIR}/include" "${BRLCAD_BASE_DIR}/include/brlcad" "${BRLCAD_BASE_DIR}/include/openNURBS")
    SET(BRLCAD_LIB_DIR "${BRLCAD_BASE_DIR}/lib")
ELSE(BRLCAD_BASE_DIR)
    MESSAGE(STATUS "\t\t Searching for BRLCAD components...")
    #if BRLCAD_BASE_DIR is NOT set, then search for files KNOWN to be in the BRLCAD installation.

    #Find /bin
    FIND_PATH(BRLCAD_BIN_DIR brlcad-config "$ENV{PATH}")
    IF(NOT BRLCAD_BIN_DIR)
 	    MESSAGE(STATUS "\t\t Could not find BRLCAD bin directory anywhere in paths: $ENV{PATH}")
	    RETURN()
    ENDIF(NOT BRLCAD_BIN_DIR)

    #Find include directories (aka more than one)
    SET(HEADERS_TO_SEARCH_FOR brlcad/bu.h bu.h opennurbs.h )
       
    FOREACH (tHead ${HEADERS_TO_SEARCH_FOR})
            
        FIND_PATH(_HEADER_DIR_${tHead} ${tHead} "$ENV{PATH}")
    
        IF(RT3_VERBOSE_CMAKE_OUTPUT)   
            IF(_HEADER_DIR_${tHead})
                MESSAGE(STATUS "\t\t\t'${tHead}' was found: ${_HEADER_DIR_${tHead}}")
            ELSE(_HEADER_DIR_${tHead})
                MESSAGE(STATUS "\t\t\t'${tHead}' was NOT found.")
            ENDIF(_HEADER_DIR_${tHead})
        ENDIF(RT3_VERBOSE_CMAKE_OUTPUT)       
       
        IF(_HEADER_DIR_${tHead})
            SET(BRLCAD_INC_DIRS ${BRLCAD_INC_DIRS} ${_HEADER_DIR_${tHead}})
            SET(BRLCAD_HEADERS_FOUND ${BRLCAD_HEADERS_FOUND} ${tHead})        
        ELSE(_HEADER_DIR_${tHead})
            SET(BRLCAD_HEADERS_NOTFOUND ${BRLCAD_HEADERS_NOTFOUND} ${tHead})        
        ENDIF(_HEADER_DIR_${tHead})
    
    ENDFOREACH (tHead ${HEADERS_TO_SEARCH_FOR})
    
    IF(NOT BRLCAD_INC_DIRS)
      	MESSAGE(STATUS "\t\tCould not find BRLCAD include directories anywhere in paths: $ENV{PATH}")
    	RETURN()
    ENDIF(NOT BRLCAD_INC_DIRS)

    #Find /lib
    IF (UNIX)
        SET(LIB_EXT ".so")
    ELSE (UNIX)
        SET(LIB_EXT ".lib")
    ENDIF(UNIX)
    
    FIND_PATH(BRLCAD_LIB_DIR "libbu${LIB_EXT}")
    
    IF(NOT BRLCAD_LIB_DIR)
    	MESSAGE(STATUS "\t\tCould not find brlcad library directory in: $ENV{PATH}")
    	RETURN()
    ENDIF(NOT BRLCAD_LIB_DIR)

ENDIF(BRLCAD_BASE_DIR)



#Attempt to get brlcad version.
FIND_PROGRAM(BRLCAD_CONFIGEXE brlcad-config)
IF(BRLCAD_CONFIGEXE)
	EXECUTE_PROCESS(COMMAND ${BRLCAD_CONFIGEXE} --version OUTPUT_VARIABLE BRLCAD_VERSION)
	STRING(STRIP ${BRLCAD_VERSION} BRLCAD_VERSION)
	
	IF(BRLCAD_VERSION)
    	STRING(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" BRLCAD_MAJOR_VERSION "${BRLCAD_VERSION}")
    	STRING(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" BRLCAD_MINOR_VERSION "${BRLCAD_VERSION}")
    	STRING(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" BRLCAD_PATCH_VERSION "${BRLCAD_VERSION}")
        SET(BRLCAD_VERSION_FOUND TRUE)
    ELSEIF(BRLCAD_VERSION)
        MESSAGE(STATUS "\t\t'brlcad-config --version' was found and executed, but produced no output.")
        SET(BRLCAD_VERSION_FOUND FALSE)
    ENDIF(BRLCAD_VERSION)

ELSE(BRLCAD_CONFIGEXE)
    MESSAGE(STATUS "\t\tCould not locate 'brlcad-config'.")
    SET(BRLCAD_VERSION_FOUND FALSE)
ENDIF(BRLCAD_CONFIGEXE)

#TODO need to make the BRLCAD version checking a requirement for coreInterface, but nothing else.
#TODO figure out why brlcad-config isn't present on Windows.

##########################################################################
#Search for Libs

SET(LIBS_TO_SEARCH_FOR 
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
	tie
	utahrle
	wdb
)

MESSAGE(STATUS "BRLCAD_LIB_DIR: ${BRLCAD_LIB_DIR}")
MESSAGE(STATUS "LIB_EXT: ${LIB_EXT}")

FOREACH (tlib ${LIBS_TO_SEARCH_FOR})
    FIND_LIBRARY(_BRLCAD_LIBRARY_${tlib} ${tlib} ${BRLCAD_LIB_DIR} )

    IF(RT3_VERBOSE_CMAKE_OUTPUT)   
        IF(_BRLCAD_LIBRARY_${tlib})
            MESSAGE(STATUS "\t\t'${tlib}' was found: ${_BRLCAD_LIBRARY_${tlib}}")
        ELSE(_BRLCAD_LIBRARY_${tlib})
            MESSAGE(STATUS "\t\t'${tlib}' was NOT found.")
        ENDIF(_BRLCAD_LIBRARY_${tlib})
    ENDIF(RT3_VERBOSE_CMAKE_OUTPUT)       
   
    IF(_BRLCAD_LIBRARY_${tlib})
        SET(BRLCAD_LIBRARIES ${BRLCAD_LIBRARIES} ${_BRLCAD_LIBRARY_${tlib}})
        SET(BRLCAD_LIBRARIES_FOUND ${BRLCAD_LIBRARIES_FOUND} ${tlib})        
    ELSE(_BRLCAD_LIBRARY_${tlib})
        SET(BRLCAD_LIBRARIES_NOTFOUND ${BRLCAD_LIBRARIES_NOTFOUND} ${tlib})        
    ENDIF(_BRLCAD_LIBRARY_${tlib})

ENDFOREACH (tlib ${LIBS_TO_SEARCH_FOR})

##########################################################################
#Print status
MESSAGE(STATUS "")
MESSAGE(STATUS "\t\t Discovered BRLCAD Version ${BRLCAD_VERSION}")
MESSAGE(STATUS "\t\t BRLCAD_BIN_DIR:     ${BRLCAD_BIN_DIR}")
MESSAGE(STATUS "\t\t BRLCAD_INC_DIRS:    ${BRLCAD_INC_DIRS}")
MESSAGE(STATUS "\t\t BRLCAD_LIB_DIR:     ${BRLCAD_LIB_DIR}")
MESSAGE(STATUS "\t\t BRLCAD_LIBRARIES_FOUND:    ${BRLCAD_LIBRARIES_FOUND}")
MESSAGE(STATUS "\t\t BRLCAD_LIBRARIES_NOTFOUND: ${BRLCAD_LIBRARIES_NOTFOUND}")
MESSAGE(STATUS "\t\t BRLCAD_CONFIGEXE: ${BRLCAD_CONFIGEXE}")
IF(BRLCAD_CONFIGEXE)
    MESSAGE(STATUS "\t\t\t BRLCAD_MAJOR_VERSION: ${BRLCAD_MAJOR_VERSION}")
    MESSAGE(STATUS "\t\t\t BRLCAD_MINOR_VERSION: ${BRLCAD_MINOR_VERSION}")
    MESSAGE(STATUS "\t\t\t BRLCAD_PATCH_VERSION: ${BRLCAD_PATCH_VERSION}")
ENDIF(BRLCAD_CONFIGEXE)
MESSAGE(STATUS "")

#Set found flag
SET(BRLCAD_FOUND TRUE)