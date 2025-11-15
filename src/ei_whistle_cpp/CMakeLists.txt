cmake_minimum_required(VERSION 3.10)
project(ei_whistle_cpp)

# ROS 2
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Edge Impulse: build for Linux/posix, not CMSIS
add_compile_definitions(
  EI_PORTING_POSIX=1
  EIDSP_USE_CMSIS_DSP=0
  EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0
)

# ALSA
find_library(ALSA_LIB asound REQUIRED)

# ---- Include dirs ----
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/classifier
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/dsp
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/third_party
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow
  ${CMAKE_CURRENT_SOURCE_DIR}/model-parameters
  ${CMAKE_CURRENT_SOURCE_DIR}/tflite-model
)

# ---- Collect ALL EI / TFLM sources ----
file(GLOB_RECURSE EI_SDK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/classifier/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/dsp/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/dsp/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/porting/posix/*.cpp

 
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/c/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/core/api/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/micro/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/*.cc


  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/internal/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/internal/*/*.cc

  
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/third_party/flatbuffers/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/third_party/flatbuffers/src/*.cc

  ${CMAKE_CURRENT_SOURCE_DIR}/edge-impulse-sdk/third_party/kissfft/*.c
)



# Model: EON compiled .cpp (or plain model C++ file)
file(GLOB EI_MODEL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/tflite-model/*.cpp
)

add_executable(whistle_node
  whistle_node.cpp
  ${EI_SDK_SOURCES}
  ${EI_MODEL_SOURCES}
)

# Link ROS 2 deps and ALSA
ament_target_dependencies(whistle_node rclcpp std_msgs)
target_link_libraries(whistle_node ${ALSA_LIB})

# Install
install(TARGETS whistle_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()
