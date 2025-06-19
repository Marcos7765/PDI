cmake_minimum_required(VERSION 3.0.0)
project(exercicio_10 VERSION 0.1.0 LANGUAGES C CXX)
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/../utils/
)

add_executable(pontilhismo pontilhismo.cpp)
target_link_libraries(pontilhismo ${OpenCV_LIBS})
