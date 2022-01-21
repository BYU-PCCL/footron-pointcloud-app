cmake_minimum_required(VERSION 3.20)
project(pointcloud_renderer)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Ofast")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(websocketpp REQUIRED)

add_executable(pointcloud_renderer main.cpp glad/glad.c glad/glad.h KHR/khrplatform.h)
target_link_libraries(pointcloud_renderer glfw)
target_link_libraries(pointcloud_renderer OpenGL::GL)
#target_link_libraries(pointcloud_renderer m)
target_link_libraries(pointcloud_renderer ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} Threads::Threads)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cell_phone_1.pcd
        ${CMAKE_CURRENT_BINARY_DIR}/cell_phone_1.pcd COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water_bottle_1.pcd
        ${CMAKE_CURRENT_BINARY_DIR}/water_bottle_1.pcd COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.txt
        ${CMAKE_CURRENT_BINARY_DIR}/test.txt COPYONLY)
