cmake_minimum_required(VERSION 3.17)

project("ledcube")
add_executable("ledcube" 
"main.cpp")

add_subdirectory("LedcubeLibrary")
target_include_directories("ledcube-lib"
    PUBLIC 
    "LedcubeLibrary/"
)
#target_compile_definitions("ledcube-lib" PUBLIC "LEDCUBE_FRAME_PROFILING")

target_link_libraries("ledcube"
    PRIVATE 
    "ledcube-lib"
)