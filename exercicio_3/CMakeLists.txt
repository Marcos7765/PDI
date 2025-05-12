cmake_minimum_required(VERSION 3.0.0)
project(exercicio_1 VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/../utils/
)

add_executable(decoder decoder.cpp)
add_executable(encoder encoder.cpp)
target_link_libraries(decoder ${OpenCV_LIBS})
target_link_libraries(encoder ${OpenCV_LIBS})
