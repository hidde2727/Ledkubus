cmake_minimum_required(VERSION 3.17)

add_library("ledcube-lib"
"Ledcube.h"
"Ledcube.cpp"
"GPIO.h"
"GPIO.cpp"
"SPI.h"
"SPI.cpp"
"PCH.h"
"Logging.h"
"Logging.cpp"
"MultiThreading.h"
"MultiThreading.cpp"
)

target_precompile_headers("ledcube-lib" PUBLIC "PCH.h")