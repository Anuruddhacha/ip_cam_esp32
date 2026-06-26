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
    MainWindow.cpp
    MainWindow.h
    VideoWidget.cpp
    VideoWidget.h
)

target_link_libraries(esp32cam_client PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebSockets
)

# On Windows with MSVC, build a GUI app (no console window).
if(WIN32)
    set_target_properties(esp32cam_client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
