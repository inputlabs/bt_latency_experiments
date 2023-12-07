cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(picolatency C CXX ASM)

pico_sdk_init()
	    
add_subdirectory(host)
add_subdirectory(client)
