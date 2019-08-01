cmake_minimum_required (VERSION 2.6)
include(GenerateExportHeader)
include(CheckIncludeFile)
include(CMakeDependentOption)
project (janssonpath)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)

configure_file (
	"${PROJECT_SOURCE_DIR}/include/janssonpath_conf.h.in"
	"${PROJECT_BINARY_DIR}/janssonpath_conf.h"
)
include_directories("${PROJECT_BINARY_DIR}")

find_package(JANSSON REQUIRED)

include_directories ("${PROJECT_SOURCE_DIR}/include")
include_directories(${JANSSON_INCLUDE_DIRS})

set(COMMON_SRC src/error.c src/memory.c)
set(COMMON_INC ${PROJECT_BINARY_DIR}/janssonpath_conf.h ${PROJECT_BINARY_DIR}/janssonpath_export.h include/private/common.h include/private/jansson_memory.h)
set(LEXEME_SRC src/lexeme.c)
set(LEXEME_INC include/private/lexeme.h)
set(PARSER_SRC src/compile.c)
set(PARSER_INC include/janssonpath.h include/private/jsonpath_ast.h)

set(ALL_SRC ${COMMON_SRC} ${LEXEME_SRC} ${PARSER_SRC})
set(ALL_INC ${COMMON_INC} ${LEXEME_INC} ${PARSER_INC})
set(JANSSONPATH_HDR_PUBLIC include/janssonpath.h ${PROJECT_BINARY_DIR}/janssonpath_conf.h include/janssonpath_error.h ${PROJECT_BINARY_DIR}/janssonpath_export.h)

add_library(janssonpath SHARED ${ALL_SRC} ${ALL_INC})
target_link_libraries(janssonpath ${JANSSON_LIBRARIES})
generate_export_header(janssonpath)
add_library(janssonpath_static STATIC ${ALL_SRC} ${ALL_INC})
set_target_properties(janssonpath_static PROPERTIES
  COMPILE_FLAGS -DJANSSONPATH_STATIC_DEFINE)

OPTION(JANSSONPATH_SUPPORT_REGEX OFF)

if(JANSSONPATH_SUPPORT_REGEX)
	set(REGEX_ENGINE "system_regex" CACHE STRING "Engine for regular expression support")
	set_property(CACHE REGEX_ENGINE PROPERTY STRINGS system_regex PCRE2)
	if(REGEX_ENGINE STREQUAL "system_regex")
		CHECK_INCLUDE_FILE(regex.h SYSTEM_REGEX CMAKE_REQUIRED_FLAGS)
		if(NOT SYSTEM_REGEX)
			message("No regex.h found. switching to PCRE2." )
			set(REGEX_ENGINE "PCRE2")
		endif()
	endif()
	if(REGEX_ENGINE STREQUAL "PCRE2")
		find_package(PCRE2 REQUIRED)
		target_link_libraries(janssonpath ${PCRE2_LIBRARIES})
		target_include_directories(janssonpath PUBLIC ${PCRE2_INCLUDE_DIRS})
	endif()
endif()

add_executable(lexeme_test src/lexeme_test.c ${LEXEME_SRC} ${COMMON_SRC} ${COMMON_INC} ${LEXEME_INC})
target_link_libraries(lexeme_test ${JANSSON_LIBRARIES})
set_target_properties(lexeme_test PROPERTIES
  COMPILE_FLAGS -DJANSSONPATH_STATIC_DEFINE)

option(JANSSONPATH_INSTALL "Generate installation target" ON)
CMAKE_DEPENDENT_OPTION(INSTALL_WITH_SYMBOL "Install debug symbols" ON "JANSSONPATH_INSTALL" OFF)
if (JANSSONPATH_INSTALL)
	install(TARGETS janssonpath
		LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
		RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
		INCLUDES DESTINATION "${CMAKE_INSTALL_PREFIX}/include")

	install(TARGETS janssonpath_static
		LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
		RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
		INCLUDES DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
	install(FILES ${JANSSONPATH_HDR_PUBLIC}
		DESTINATION "${CMAKE_INSTALL_PREFIX}/include")

	install(FILES $<TARGET_PDB_FILE:janssonpath> DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" OPTIONAL)
endif()

if (MSVC)
   add_definitions( "/W3 /D_CRT_SECURE_NO_WARNINGS /wd4996 /nologo" )
endif()