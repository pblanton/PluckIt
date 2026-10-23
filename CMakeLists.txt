cmake_minimum_required(VERSION 3.16)
project(PluckIt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(X11 REQUIRED)

# Drawing the mouse cursor into captures needs XFixes (Debian: libxfixes-dev).
# Optional so the project still builds without it; "Include Cursor" is then a
# no-op.
if(X11_Xfixes_FOUND)
    set(PLUCKIT_XFIXES_LIBS ${X11_Xfixes_LIB})
else()
    message(WARNING "XFixes development files not found; cursor capture disabled (install libxfixes-dev)")
endif()

add_executable(PluckIt
    src/headers/FloatingWidget.h
    src/headers/CaptureBackend.h
    src/headers/X11CaptureBackend.h
    src/headers/RegionSelectOverlay.h
    src/headers/WindowSelectOverlay.h
    src/headers/CaptureManager.h
    src/headers/Annotation.h
    src/implementations/TextAnnotation.cpp
    src/headers/TextAnnotation.h
    src/headers/EditorCanvas.h
    src/headers/EditorWindow.h
    src/headers/ImageEffects.h
    src/headers/IconFactory.h
    src/headers/SettingsPage.h
    src/headers/CaptureSettings.h
    src/headers/CaptureOptionsPanel.h
    src/headers/CaptureButton.h
    src/headers/CaptureTray.h
    src/headers/ToggleSwitch.h
    src/headers/SettingsKeys.h
    src/implementations/main.cpp
    src/implementations/FloatingWidget.cpp
    src/implementations/X11CaptureBackend.cpp
    src/implementations/RegionSelectOverlay.cpp
    src/implementations/WindowSelectOverlay.cpp
    src/implementations/CaptureManager.cpp
    src/implementations/Annotation.cpp
    src/implementations/EditorCanvas.cpp
    src/implementations/EditorWindow.cpp
    src/implementations/ImageEffects.cpp
    src/implementations/IconFactory.cpp
    src/implementations/SettingsPage.cpp
    src/implementations/CaptureSettings.cpp
    src/implementations/CaptureOptionsPanel.cpp
    src/implementations/CaptureButton.cpp
    src/implementations/CaptureTray.cpp
    src/implementations/ToggleSwitch.cpp
)

target_link_libraries(PluckIt PRIVATE
    Qt6::Widgets
    Qt6::Network
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
    ${PLUCKIT_XFIXES_LIBS}
)

target_include_directories(PluckIt PRIVATE ${X11_INCLUDE_DIR})

if(X11_Xfixes_FOUND)
    target_compile_definitions(PluckIt PRIVATE PLUCKIT_HAVE_XFIXES)
    target_include_directories(PluckIt PRIVATE ${X11_Xfixes_INCLUDE_PATH})
endif()

include(GNUInstallDirs)
install(TARGETS PluckIt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/pluckit.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
foreach(size 16 24 32 48 64 96 128 256 512)
    install(FILES repo_files/PluckIt_Logo_PNGs/pluckit-glyph-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME pluckit.png)
endforeach()
