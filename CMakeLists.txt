cmake_minimum_required(VERSION 3.16)

project(AudioConverter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core)


qt_add_executable(appAudioConverter
    main.cpp icon.rc
)

qt_add_qml_module(appAudioConverter
    URI AudioConverter
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES converterbackend.h converterbackend.cpp
    RESOURCES
        wexi.jpeg
        about.png

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appAudioConverter PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appAudioConverter
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appAudioConverter
    PRIVATE Qt6::Quick Qt6::Core
)

include(GNUInstallDirs)
install(TARGETS appAudioConverter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
