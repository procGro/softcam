cmake_minimum_required(VERSION 3.10)
project(virtual_camera_engine)

set(CMAKE_CXX_STANDARD 17)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Find FFmpeg (via pkg-config)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)

# Include directories
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../src
)

# Avoid __cdecl macro error on linux
if(NOT WIN32)
    add_compile_definitions(__cdecl=)
endif()

# Executable
add_executable(virtual_camera_engine main.cpp MediaReader.cpp LensModel.cpp VirtualPTZ.cpp)

# Link libraries
target_link_libraries(virtual_camera_engine
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
)

# Note: on Windows, you would also link against softcam.lib
if(WIN32)
    target_link_directories(virtual_camera_engine PRIVATE ${CMAKE_SOURCE_DIR}/../../dist/lib/x64)
    target_link_libraries(virtual_camera_engine softcam)
else()
    # Stub the API for linux build/test validation
    target_sources(virtual_camera_engine PRIVATE
        linux_stub.cpp
    )
endif()
