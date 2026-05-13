cmake_minimum_required(VERSION 3.28)

project(obs-qr-stream-key VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(OpenSSL REQUIRED)

add_library(obs-qr-stream-key MODULE)

target_sources(
  obs-qr-stream-key
  PRIVATE
    obs-qr-stream-key.cpp
    adda-login-client.cpp
    adda-login-client.hpp
    qr-relay-client.cpp
    qr-relay-client.hpp
    qr-dialog.cpp
    qr-dialog.hpp
    auth-selection-dialog.cpp
    auth-selection-dialog.hpp
    qrcodegen.cpp
    qrcodegen.hpp
    httplib.h
)

target_link_libraries(
  obs-qr-stream-key
  PRIVATE
    OBS::libobs
    OBS::frontend-api
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Disable warnings as errors for this plugin due to OpenSSL deprecation warnings
if(MSVC)
  target_compile_options(obs-qr-stream-key PRIVATE /W3 /WX-)
endif()

set_target_properties(
  obs-qr-stream-key
  PROPERTIES
    FOLDER "plugins"
    PREFIX ""
)

if(OS_WINDOWS)
  target_link_libraries(obs-qr-stream-key PRIVATE ws2_32 crypt32)
endif()

set_target_properties_obs(obs-qr-stream-key PROPERTIES FOLDER plugins PREFIX "")
