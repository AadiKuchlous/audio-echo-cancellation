# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(audio_echo_cancellation_library VERSION 0.0.1 LANGUAGES C++)

add_library(audio_echo_cancellation SHARED
  "audio_echo_cancellation.cpp"
)

set_target_properties(audio_echo_cancellation PROPERTIES
  PUBLIC_HEADER audio_echo_cancellation.h
  OUTPUT_NAME "audio_echo_cancellation"
)

target_compile_definitions(audio_echo_cancellation PUBLIC DART_SHARED_LIB)
