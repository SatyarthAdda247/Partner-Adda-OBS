cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Widgets Network)

if(APPLE)
  # Use Homebrew OpenSSL for cpp-httplib HTTPS support
  set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")
endif()
find_package(OpenSSL REQUIRED)

add_library(obs-qr-stream-key MODULE)
add_library(OBS::obs-qr-stream-key ALIAS obs-qr-stream-key)

target_sources(
  obs-qr-stream-key
  PRIVATE
    qr-pairing-main.cpp
    network-util.hpp
    network-util.cpp
    qrcode-util.hpp
    qrcode-util.cpp
    qr-pairing-plugin.hpp
    qr-pairing-plugin.cpp
    qr-pairing-server.hpp
    qr-pairing-server.cpp
    qr-choice-dialog.hpp
    qr-choice-dialog.cpp
    qr-choice-dialog.ui
    qr-pairing-dialog.hpp
    qr-pairing-dialog.cpp
    qr-pairing-dialog.ui
    adda-login-client.hpp
    adda-login-client.cpp
    qr-relay-client.hpp
    qr-relay-client.cpp
    vendor/qrcodegen/qrcodegen.cpp
    vendor/qrcodegen/qrcodegen.hpp
)

target_include_directories(obs-qr-stream-key PRIVATE vendor/httplib vendor/qrcodegen ${OPENSSL_INCLUDE_DIR})

target_link_libraries(
  obs-qr-stream-key
  PRIVATE
    OBS::frontend-api
    OBS::libobs
    Qt::Widgets
    Qt::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)
if(APPLE)
  target_link_libraries(obs-qr-stream-key PRIVATE "-framework Security")
endif()

set_target_properties_obs(
  obs-qr-stream-key
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
)
