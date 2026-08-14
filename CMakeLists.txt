cmake_minimum_required(VERSION 3.21)
project(scry-qt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets WebSockets)
# Modern protobuf (22+, with abseil deps) ships `protobufConfig.cmake`
# whose `protobuf::libprotobuf` target carries transitive absl link deps.
# Older protobuf 3.x (e.g. Ubuntu 24.04's system 3.21.12) ships no
# Config file, only CMake's built-in FindProtobuf.cmake. Try CONFIG
# first; fall back to module mode for legacy distros.
find_package(protobuf CONFIG QUIET)
if(NOT protobuf_FOUND)
  find_package(Protobuf REQUIRED)
endif()

# --- protobuf (shared schema with scry-cpp) ----------------------------
set(SEQ_PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(SEQ_PROTO_FILES
  "${SEQ_PROTO_DIR}/seq/v1/events.proto"
  "${SEQ_PROTO_DIR}/seq/v1/client.proto"
)

if(NOT EXISTS "${SEQ_PROTO_DIR}/seq/v1/events.proto")
  message(FATAL_ERROR
    "scry-proto submodule is not checked out. Run:\n"
    "  git submodule update --init --recursive")
endif()

set(SEQ_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/gen-proto")
file(MAKE_DIRECTORY "${SEQ_PROTO_GEN_DIR}")

set(SEQ_PROTO_SRCS)
set(SEQ_PROTO_HDRS)
foreach(proto ${SEQ_PROTO_FILES})
  get_filename_component(proto_name "${proto}" NAME_WE)
  get_filename_component(proto_dir "${proto}" DIRECTORY)
  file(RELATIVE_PATH proto_rel "${SEQ_PROTO_DIR}" "${proto_dir}")
  set(cc "${SEQ_PROTO_GEN_DIR}/${proto_rel}/${proto_name}.pb.cc")
  set(hh "${SEQ_PROTO_GEN_DIR}/${proto_rel}/${proto_name}.pb.h")
  add_custom_command(
    OUTPUT "${cc}" "${hh}"
    COMMAND protobuf::protoc
            --cpp_out ${SEQ_PROTO_GEN_DIR}
            --proto_path ${SEQ_PROTO_DIR}
            ${proto}
    DEPENDS "${proto}" protobuf::protoc
    COMMENT "protoc ${proto_rel}/${proto_name}.proto"
    VERBATIM
  )
  list(APPEND SEQ_PROTO_SRCS "${cc}")
  list(APPEND SEQ_PROTO_HDRS "${hh}")
endforeach()

add_library(seq-proto STATIC ${SEQ_PROTO_SRCS} ${SEQ_PROTO_HDRS})
target_include_directories(seq-proto PUBLIC "${SEQ_PROTO_GEN_DIR}")
target_link_libraries(seq-proto PUBLIC protobuf::libprotobuf)
set_target_properties(seq-proto PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)

# --- application sources ----------------------------------------------------
set(SOURCES
  src/main.cpp
  src/app/MainWindow.cpp
  src/app/ConnectDialog.cpp
  src/app/Settings.cpp
  src/daemon/DaemonConnection.cpp
  src/daemon/SpawnModel.cpp
  src/daemon/ZoneState.cpp
  src/daemon/PlayerState.cpp
  src/util/ConColor.cpp
  src/widgets/SpawnListWidget.cpp
  src/widgets/MapWidget.cpp
  src/widgets/CompassWidget.cpp
  src/widgets/PlayerStatsWidget.cpp
  src/widgets/SpellListWidget.cpp
  src/widgets/MessageWindow.cpp
  src/widgets/NetworkDiagWidget.cpp
  resources/scry-qt.qrc
)

add_executable(scry-qt ${SOURCES})

target_include_directories(scry-qt PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${SEQ_PROTO_GEN_DIR}"
)

target_link_libraries(scry-qt PRIVATE
  seq-proto
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::WebSockets
)

# On Windows set the executable subsystem to Windows (no console window).
if(WIN32)
  set_target_properties(scry-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# --- tests ------------------------------------------------------------------
# Qt6::Test ships with qtbase but isn't guaranteed on every packaging
# split, so the test target is opt-in on its presence rather than a hard
# requirement for building the app.
find_package(Qt6 6.2 QUIET COMPONENTS Test)
if(Qt6Test_FOUND)
  enable_testing()
  add_executable(concolor_test tests/concolor_test.cpp src/util/ConColor.cpp)
  target_include_directories(concolor_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
  target_link_libraries(concolor_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
  add_test(NAME concolor COMMAND concolor_test)
endif()

# --- install rules (used by .deb CPack and AppImage packaging) --------------
include(GNUInstallDirs)
install(TARGETS scry-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
  install(FILES   packaging/scry-qt.desktop
          DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES   packaging/scry-qt.png
          DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
endif()

# --- AppImage convenience target -------------------------------------------
# `cmake --build <build> --target appimage` runs packaging/build-appimage.sh
# against this build dir. The script downloads linuxdeploy on demand.
if(UNIX AND NOT APPLE)
  add_custom_target(appimage
    COMMAND ${CMAKE_COMMAND} -E env
            "VERSION=${CPACK_PACKAGE_VERSION}"
            bash ${CMAKE_SOURCE_DIR}/packaging/build-appimage.sh ${CMAKE_BINARY_DIR}
    DEPENDS scry-qt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
    COMMENT "Building scry-qt AppImage")
endif()

# --- CPack ------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "scry-qt")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Scry Qt6 Client")
if(WIN32)
  set(CPACK_GENERATOR "NSIS")
elseif(UNIX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "scry-eq")
endif()
include(CPack)
