cmake_minimum_required(VERSION 3.16)
project(PluckIt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(X11 REQUIRED)

add_executable(PluckIt
    src/main.cpp
    src/FloatingWidget.cpp
    src/FloatingWidget.h
    src/CaptureBackend.h
    src/X11CaptureBackend.cpp
    src/X11CaptureBackend.h
    src/RegionSelectOverlay.cpp
    src/RegionSelectOverlay.h
    src/WindowSelectOverlay.cpp
    src/WindowSelectOverlay.h
    src/CaptureManager.cpp
    src/CaptureManager.h
    src/Annotation.cpp
    src/Annotation.h
    src/EditorCanvas.cpp
    src/EditorCanvas.h
    src/EditorWindow.cpp
    src/EditorWindow.h
    src/ImageEffects.cpp
    src/ImageEffects.h
    src/IconFactory.cpp
    src/IconFactory.h
    src/SettingsWindow.cpp
    src/SettingsWindow.h
    src/SettingsKeys.h
)

target_link_libraries(PluckIt PRIVATE
    Qt6::Widgets
    Qt6::Network
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
)

target_include_directories(PluckIt PRIVATE ${X11_INCLUDE_DIR})
