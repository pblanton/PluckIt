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
    src/SettingsPage.cpp
    src/SettingsPage.h
    src/CaptureSettings.cpp
    src/CaptureSettings.h
    src/CaptureOptionsPanel.cpp
    src/CaptureOptionsPanel.h
    src/CaptureButton.cpp
    src/CaptureButton.h
    src/CaptureTray.cpp
    src/CaptureTray.h
    src/ToggleSwitch.cpp
    src/ToggleSwitch.h
    src/SettingsKeys.h
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
