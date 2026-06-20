cmake_minimum_required(VERSION 3.19)
project(ChordLabV003A LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(ChordLabV003A
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc ## resources tracking file
    app.rc
    setlistmanager.h
    setlistmanager.cpp
    "resources/setlists/Monday Songbook 1.set" "resources/setlists/Monday Songbook 2.set"        ## app resource tracking file
)
qq
target_link_libraries(ChordLabV003A
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
)

install(TARGETS ChordLabV003A
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ChordLabV003A
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
