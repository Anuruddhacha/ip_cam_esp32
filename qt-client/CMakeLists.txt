cmake_minimum_required(VERSION 3.16)
project(esp32cam_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Works with either Qt6 or Qt5 (whichever is installed).
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebSockets)

add_executable(esp32cam_client
    main.cpp

    app/Theme.h
    app/Theme.cpp

    core/StreamClient.h
    core/StreamClient.cpp
    core/StreamStats.h
    core/StreamStats.cpp
    core/FrameRecorder.h
    core/FrameRecorder.cpp

    ui/VideoWidget.h
    ui/VideoWidget.cpp
    ui/ControlPanel.h
    ui/ControlPanel.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
)

# Allow includes like "core/StreamClient.h" from anywhere in the project.
target_include_directories(esp32cam_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(esp32cam_client PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebSockets
)

if(WIN32)
    set_target_properties(esp32cam_client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
