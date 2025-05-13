cmake_minimum_required(VERSION 3.0.0)
project(exercicio_5 VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)

include_directories(
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/../utils/
)

add_executable(convolution convolution.cpp)
add_executable(depthoffield depthoffield.cpp)
target_link_libraries(convolution ${OpenCV_LIBS})
target_link_libraries(depthoffield ${OpenCV_LIBS})
