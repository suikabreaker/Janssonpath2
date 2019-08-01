# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec <kapecp@gmail.com>
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# Modified at 8/1/2019 to PCRE2

# Look for the header file.
FIND_PATH(PCRE2_INCLUDE_DIR NAMES pcre2.h PCRE2.h PATHS $ENV{PCRE2}/include)

# Look for the library.
FIND_LIBRARY(PCRE2_LIBRARY NAMES pcre2-8d pcre2-posixd pcre2-8 pcre2-posix PATHS $ENV{PCRE2}/lib)

# Handle the QUIETLY and REQUIRED arguments and set PCRE_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE2 DEFAULT_MSG PCRE2_LIBRARY PCRE2_INCLUDE_DIR)

# Copy the results to the output variables.
IF(PCRE2_FOUND)
	SET(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
	SET(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
ELSE(PCRE2_FOUND)
	SET(PCRE2_LIBRARIES)
	SET(PCRE2_INCLUDE_DIRS)
ENDIF(PCRE2_FOUND)

MARK_AS_ADVANCED(PCRE2_INCLUDE_DIRS PCRE2_LIBRARIES)
