cmake_minimum_required(VERSION 3.10)
project(VkApp)
include_directories(${CMAKE_BINARY_DIR})

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(vksg
    vksg.cpp
    SceneGraph.cpp)

target_include_directories(vksg PRIVATE /usr/local/include)
target_link_libraries(vksg PRIVATE glfw Vulkan::Vulkan /usr/local/lib/libvk-bootstrap.a)
