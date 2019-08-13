project (janssonpath)
set (VERSION_MAJOR 2)
set (VERSION_MINOR 0)

cmake_minimum_required (VERSION 2.6)
include(GenerateExportHeader)
include(CheckIncludeFile)
include(CMakeDependentOption)
include(CheckFunctionExists)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

# global setting
if(MSVC)
# /Wall could generate tons of warning even for standard library
	add_definitions( "/W4 /D_CRT_SECURE_NO_WARNINGS /nologo /wd4201 /wd4204 /wd4214 /wd4706 /wd4996" )
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Wmissing-declarations -Wundef -Werror=implicit-function-declaration -Wno-implicit-fallthrough -Wno-deprecated-declarations")
endif()

find_package(JANSSON REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories(${JANSSON_INCLUDE_DIRS})
check_function_exists(iswascii HAVE_ISWASCII)

option(JANSSONPATH_SUPPORT_REGEX "Whether build jansson with regular expression support" ON)
option(JANSSONPATH_CONSTANT_FOLD "Enable jansson path to do constant folding during evaluation" ON)
if(NOT WIN32)
	option(JANSSONPATH_FORCE_PIC "Enable PIC(to link janssonpath_static for a shared library)" OFF)
endif()

set(COMMON_SRC src/memory.c src/error.c)
set(COMMON_INC ${PROJECT_BINARY_DIR}/janssonpath_conf.h ${PROJECT_BINARY_DIR}/janssonpath_export.h include/private/common.h include/private/jansson_memory.h include/private/error.h)
set(LEXEME_SRC src/lexeme.c)
set(LEXEME_INC include/private/lexeme.h)
set(PARSER_SRC src/compile.c)
set(PARSER_INC include/janssonpath.h include/private/jsonpath_ast.h)
set(EVALUATE_SRC src/evaluate.c)
set(EVALUATE_INC include/janssonpath_evaluate.h)
if(JANSSONPATH_SUPPORT_REGEX)
	set(EVALUATE_INC ${EVALUATE_INC} include/private/regex_impl.h)
endif()
if(JANSSONPATH_SUPPORT_REGEX)
	set(EVALUATE_SRC ${EVALUATE_SRC} src/regex_impl.c)
endif()
set(DEPRECATED_SRC src/deprecated.c)
set(DEPRECATED_INC include/janssonpath_deprecated.h)
set(ALL_SRC ${COMMON_SRC} ${LEXEME_SRC} ${PARSER_SRC} ${EVALUATE_SRC} ${DEPRECATED_SRC})
set(ALL_INC ${COMMON_INC} ${LEXEME_INC} ${PARSER_INC} ${EVALUATE_INC} ${DEPRECATED_INC})
set(JANSSONPATH_HDR_PUBLIC include/janssonpath.h ${PROJECT_BINARY_DIR}/janssonpath_conf.h include/janssonpath_error.h include/janssonpath_evaluate.h include/janssonpath_deprecated.h ${PROJECT_BINARY_DIR}/janssonpath_export.h)

add_library(janssonpath SHARED ${ALL_SRC} ${ALL_INC})
target_link_libraries(janssonpath ${JANSSON_LIBRARIES})
generate_export_header(janssonpath)
add_library(janssonpath_static STATIC ${ALL_SRC} ${ALL_INC})
set_target_properties(janssonpath_static PROPERTIES
  COMPILE_FLAGS -DJANSSONPATH_STATIC_DEFINE)
  
set_target_properties(janssonpath janssonpath_static PROPERTIES DEBUG_POSTFIX  "d")

if(JANSSONPATH_FORCE_PIC)
	set_target_properties(janssonpath_static PROPERTIES COMPILE_FLAGS  "-fPIC")
endif()

if(JANSSONPATH_SUPPORT_REGEX)
	set(JANSSONPATH_REGEX_ENGINE "system_regex" CACHE STRING "Engine for regular expression support")
	set_property(CACHE JANSSONPATH_REGEX_ENGINE PROPERTY STRINGS system_regex PCRE2 PCRE)
	if(JANSSONPATH_REGEX_ENGINE STREQUAL "system_regex")
		CHECK_INCLUDE_FILE(regex.h SYSTEM_REGEX_HEADER CMAKE_REQUIRED_FLAGS)
		if(NOT SYSTEM_REGEX_HEADER)
			message("No regex.h found. Switching to PCRE2." )
			set(JANSSONPATH_REGEX_ENGINE "PCRE2" CACHE STRING "Engine for regular expression support" FORCE)
		endif()
	endif()
	if(JANSSONPATH_REGEX_ENGINE STREQUAL "PCRE2")
		find_package(PCRE2)
		if(PCRE2_FOUND)
			target_link_libraries(janssonpath ${PCRE2_LIBRARIES})
			target_include_directories(janssonpath PUBLIC ${PCRE2_INCLUDE_DIRS})
		
			target_link_libraries(janssonpath_static ${PCRE2_LIBRARIES})
			target_include_directories(janssonpath_static PUBLIC ${PCRE2_INCLUDE_DIRS})
		else()
			message("PCRE2 not found. Switching to PCRE." )
			set(JANSSONPATH_REGEX_ENGINE "PCRE" CACHE STRING "Engine for regular expression support" FORCE)
		endif()
	endif()
	if(JANSSONPATH_REGEX_ENGINE STREQUAL "PCRE")
		find_package(PCRE)
		if(PCRE_FOUND)
			target_link_libraries(janssonpath ${PCRE_LIBRARIES})
			target_include_directories(janssonpath PUBLIC ${PCRE_INCLUDE_DIRS})
		
			target_link_libraries(janssonpath_static ${PCRE_LIBRARIES})
			target_include_directories(janssonpath_static PUBLIC ${PCRE_INCLUDE_DIRS})
		else()
			message("PCRE not found. Will build without regular expression support." )
			set(JANSSONPATH_SUPPORT_REGEX OFF CACHE STRING "Whether build jansson with regular expression support" FORCE)
		endif()
	endif()
else()
	set(JANSSONPATH_REGEX_ENGINE "none" CACHE STRING "Engine for regular expression support. Tog JANSSONPATH_SUPPORT_REGEX and reconfiure to choose.")
	set_property(CACHE JANSSONPATH_REGEX_ENGINE PROPERTY STRINGS none)
endif()

add_executable(lexeme_test src/lexeme_test.c ${LEXEME_SRC} ${COMMON_SRC} ${COMMON_INC} ${LEXEME_INC})
target_link_libraries(lexeme_test ${JANSSON_LIBRARIES})
set_target_properties(lexeme_test PROPERTIES
  COMPILE_FLAGS -DJANSSONPATH_STATIC_DEFINE)

add_executable(compile_test src/compile_test.c ${JANSSONPATH_HDR_PUBLIC})
target_link_libraries(compile_test ${JANSSON_LIBRARIES} janssonpath)

add_executable(full_test src/full_test.c ${JANSSONPATH_HDR_PUBLIC})
target_link_libraries(full_test ${JANSSON_LIBRARIES} janssonpath)

option(JANSSONPATH_INSTALL "Generate installation target" ON)

if (WIN32)
	CMAKE_DEPENDENT_OPTION(DEBUG_SYMBOL "Install debug symbols" ON "JANSSONPATH_INSTALL" OFF)
endif()
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
	if(WIN32 AND DEBUG_SYMBOL)
		install(FILES $<TARGET_PDB_FILE:janssonpath> DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" OPTIONAL)
	endif()
endif()

option(DEBUG_ASAN "Enable santinizer for debuging" OFF)
if(DEBUG_ASAN)
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	#target_link_libraries(compile_test -fsanitize=address)
endif()

# important! put it after all options set
configure_file (
	"${PROJECT_SOURCE_DIR}/include/janssonpath_conf.h.in"
	"${PROJECT_BINARY_DIR}/janssonpath_conf.h"
)
include_directories("${PROJECT_BINARY_DIR}")
