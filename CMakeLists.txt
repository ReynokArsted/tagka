cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Qml LinguistTools Sql Concurrent)

qt_standard_project_setup(REQUIRES 6.8)

set(QML_IMPORT_PATH "C:/msys64/ucrt64/qml" CACHE STRING "QML import paths" FORCE)

qt_add_executable(appuntitled
    src/main.cpp
    src/utils/Translator.h
    src/platform/DeviceNotifier.cpp
    src/platform/FileHasher.cpp
    src/platform/FileHasher.h
    src/platform/MoveResolver.cpp
    src/platform/MoveResolver.h
)

qt_add_translations(appuntitled TS_FILES i18n/en.ts)

set_source_files_properties(qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appuntitled
    URI untitled
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/ThemedButton.qml
        qml/components/ThingTile.qml
        qml/components/TagFlow.qml
        qml/components/SearchLine.qml
        qml/bars/TopBar.qml
        qml/bars/BottomBar.qml
        qml/panels/TagPanel.qml
        qml/panels/FilePanel.qml
        qml/dialogs/SettingsDialog.qml
        qml/dialogs/HelpDialog.qml
        qml/dialogs/ErrorDialog.qml
        qml/dialogs/ChooseCandidateDialog.qml
    SOURCES
        src/storage/Storage.h src/storage/Storage.cpp
        src/platform/System.h src/platform/System.cpp
        src/models/Thingie.h src/models/Thingie.cpp
        src/models/ThingieListModel.h src/models/ThingieListModel.cpp
        src/models/FileList.h src/models/FileList.cpp
        src/models/FileEntry.h
        src/platform/FileMonitor.h src/platform/FileMonitor.cpp
        src/processors/RequestProcessor.h src/processors/RequestProcessor.cpp
        src/processors/PathsProcessor.h src/processors/PathsProcessor.cpp
        src/processors/TagsProcessor.h src/processors/TagsProcessor.cpp
        src/platform/VolumeRegistry.h src/platform/VolumeRegistry.cpp
        src/platform/WatchList.h
)

target_include_directories(appuntitled PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/processors
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

set_target_properties(appuntitled PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appuntitled PRIVATE Qt6::Quick)
target_link_libraries(appuntitled PRIVATE Qt6::Sql)
target_link_libraries(appuntitled PRIVATE Qt6::Concurrent)

include(GNUInstallDirs)
install(TARGETS appuntitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)