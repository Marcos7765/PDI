cmake_minimum_required(VERSION 3.0.0)
project(exercicio_1 VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/../utils/
)

add_executable(filestorage filestorage.cpp)
target_link_libraries(filestorage ${OpenCV_LIBS})
