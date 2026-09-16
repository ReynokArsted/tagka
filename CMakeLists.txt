cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Qml LinguistTools Sql)

qt_standard_project_setup(REQUIRES 6.8)

set(QML_IMPORT_PATH "C:/msys64/ucrt64/qml" CACHE STRING "QML import paths" FORCE)

qt_add_executable(appuntitled
    Main.cpp
    Translator.h
)

qt_add_translations(appuntitled TS_FILES i18n/en.ts)

set_source_files_properties(Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appuntitled
    URI untitled
    VERSION 1.0
    QML_FILES 
        Main.qml
        Theme.qml
        ThemedButton.qml 
        ThingGrid.qml 
        ThingTile.qml
        SignalHub.qml
        TopBar.qml
        BottomBar.qml
        TagPanel.qml
        SearchLine.qml
    SOURCES 
        Thingie.h Thingie.cpp 
        ThingieListModel.h ThingieListModel.cpp 
        ThingModel.h ThingModel.cpp
        FileListModel.h FileListModel.cpp
        DataBaseModule.h DataBaseModule.cpp
        UsnJournalMonitor.h
)


set_target_properties(appuntitled PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appuntitled PRIVATE Qt6::Quick)
target_link_libraries(appuntitled PRIVATE Qt6::Sql)

include(GNUInstallDirs)
install(TARGETS appuntitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)