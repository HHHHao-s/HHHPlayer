cmake_minimum_required(VERSION 3.5)

project(HHHPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)





find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        mediaplayer.h
        mediaplayer.cpp
        msgqueue.h
        msgqueue.cpp
        ffplayer.h
        ffplayer.cpp
        logger.h
        bufferqueue.h
        bufferqueue.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HHHPlayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ctrlbar.h ctrlbar.cpp ctrlbar.ui
        titlebar.h titlebar.cpp titlebar.ui
        playlistwindow.h playlistwindow.cpp playlistwindow.ui
        displaywindow.h displaywindow.cpp displaywindow.ui
        icon.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET HHHPlayer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(HHHPlayer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(HHHPlayer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(HHHPlayer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set(LIBS SDL2 avcodec avformat swscale avdevice avutil avfilter swresample postproc)
set(LINK_DIR
    ${PROJECT_SOURCE_DIR}/thirdParty/ffmpeg/lib
    ${PROJECT_SOURCE_DIR}/thirdParty/SDL2/lib/x64
)

set(INCLUDE_DIR 
    ${PROJECT_SOURCE_DIR}/thirdParty/ffmpeg/include
    ${PROJECT_SOURCE_DIR}/thirdParty/SDL2/include)

target_link_directories(HHHPlayer PRIVATE ${LINK_DIR})
target_include_directories(HHHPlayer PRIVATE ${INCLUDE_DIR})
target_link_libraries(HHHPlayer PRIVATE ${LIBS})


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.HHHPlayer)
endif()
set_target_properties(HHHPlayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS HHHPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(HHHPlayer)
endif()


add_compile_definitions(ROOT_DIR="${PROJECT_SOURCE_DIR}")