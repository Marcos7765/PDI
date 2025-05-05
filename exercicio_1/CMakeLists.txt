cmake_minimum_required(VERSION 3.0.0)
project(exercicio_1 VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/../utils/
)

add_executable(switch_regions switch_regions.cpp)
add_executable(regions regions.cpp)
target_link_libraries(switch_regions ${OpenCV_LIBS})
target_link_libraries(regions ${OpenCV_LIBS})
