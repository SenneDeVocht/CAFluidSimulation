cmake_minimum_required (VERSION 3.8)

project(CellularAutomata)

add_executable (
	CellularAutomata
	"src/Main.cpp" 
	"src/World.h" "src/World.cpp")

include_directories("src")

add_subdirectory("external/SDL")
include_directories(CellularAutomata "external/SDL/include")
target_link_libraries(CellularAutomata SDL2-static)
set_property(TARGET CellularAutomata PROPERTY CXX_STANDARD 17)

include_directories("external/glm")