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

include_directories("external/glm")