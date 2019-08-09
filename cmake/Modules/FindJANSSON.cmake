# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec <kapecp@gmail.com>
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# Modified at 8/1/2019 to jansson

# Look for the header file.
FIND_PATH(JANSSON_INCLUDE_DIR NAMES jansson.h jansson_config.h PATHS $ENV{JANSSON_DIR}/include $ENV{JANSSON_INCLUDE})

# Look for the library.
FIND_LIBRARY(JANSSON_LIBRARY NAMES jansson jansson_d PATHS $ENV{JANSSON_DIR}/lib $ENV{JANSSON_LIB})

# Handle the QUIETLY and REQUIRED arguments and set PCRE_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(jansson DEFAULT_MSG JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

# Copy the results to the output variables.
IF(JANSSON_FOUND)
	SET(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
	SET(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
ELSE(JANSSON_FOUND)
	SET(JANSSON_LIBRARIES)
	SET(JANSSON_INCLUDE_DIRS)
ENDIF(JANSSON_FOUND)

MARK_AS_ADVANCED(JANSSON_INCLUDE_DIRS JANSSON_LIBRARIES)
