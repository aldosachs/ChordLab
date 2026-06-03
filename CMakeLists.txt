cmake_minimum_required(VERSION 3.19)
project(ChordLabV001A LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

qt_add_executable(ChordLabV001A
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc ## resources tracking file
    app.rc        ## app resource tracking file
)

target_link_libraries(ChordLabV001A
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
)

install(TARGETS ChordLabV001A
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ChordLabV001A
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
