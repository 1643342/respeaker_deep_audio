cmake_minimum_required(VERSION 3.10)
project(ei_whistle_cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# EI SDK location (unzipped C++ library)
set(EI_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ei-sdk)

include_directories(
  ${EI_SDK_DIR}
  ${EI_SDK_DIR}/edge-impulse-sdk
  ${EI_SDK_DIR}/edge-impulse-sdk/dsp
  ${EI_SDK_DIR}/edge-impulse-sdk/classifier
  ${EI_SDK_DIR}/model-parameters
  ${EI_SDK_DIR}/tflite-model
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_library(ASOUND asound)

add_executable(whistle_node src/whistle_node.cpp)
target_link_libraries(whistle_node ${ASOUND})
ament_target_dependencies(whistle_node rclcpp std_msgs)

install(TARGETS whistle_node DESTINATION lib/${PROJECT_NAME})

ament_package()

