# cmake build file for Mathomatic and the Symbolic Math Library,
# originally contributed by Jonathan Stark.
# Produces the normal version of Mathomatic with readline support.
# If you need the Symbolic Math Library as a shared library,
# change the line "add_library(mathomatic_cmake" to:
# "add_library(mathomatic_cmake SHARED" below.
# This is all untested!  It is recommended to use "makefile" instead!
# To use cmake instead of "makefile", type:
#
# cmake .
# make -f Makefile
#
# Please note that cmake makes an unfixable mess of the Mathomatic
# source distribution directory, so make a copy, first.

cmake_minimum_required(VERSION 2.6)

project(mathomatic)

file(READ VERSION FV)
string(STRIP ${FV} MATHOMATIC_VERSION)
message(STATUS VERSION: ${MATHOMATIC_VERSION})
add_definitions(-O3 -Wall -Wshadow -Wno-char-subscripts -fexceptions -DVERSION="${MATHOMATIC_VERSION}")

add_library(mathomatic_cmake
	includes.h
	standard.h
	am.h
	altproto.h
	externs.h
	blt.h
	license.h
	complex.h
	proto.h
	lib/mathomatic.h
	lib/lib.c
	globals.c
	complex.c	
	poly.c
	super.c
	am.c
	factor.c
	help.c	
	list.c	
	unfactor.c
	complex_lib.c
	factor_int.c
	simplify.c
	cmds.c	
	diff.c	
	gcd.c	
	integrate.c
	parse.c	
	solve.c
)

add_executable(testmain
	lib/mathomatic.h
	lib/testmain.c
)

add_dependencies(testmain
	mathomatic_cmake
)

add_executable(mathomatic
	includes.h
	standard.h
	am.h
	altproto.h
	externs.h
	blt.h
	license.h
	complex.h
	proto.h	
	globals.c
	complex.c	
	poly.c
	super.c
	am.c
	factor.c
	help.c	
	list.c	
	unfactor.c
	complex_lib.c
	factor_int.c
	main.c	
	simplify.c
	cmds.c	
	diff.c	
	gcd.c	
	integrate.c
	parse.c	
	solve.c
)

set_target_properties(mathomatic_cmake PROPERTIES COMPILE_FLAGS "-DLIBRARY")
set_target_properties(mathomatic PROPERTIES COMPILE_FLAGS "-DREADLINE -DUNIX")
target_link_libraries(mathomatic -lm -lreadline)
target_link_libraries(testmain mathomatic_cmake)
target_link_libraries(mathomatic_cmake -lm)
