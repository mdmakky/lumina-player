cmake_minimum_required(VERSION 3.28)

project(Lumina VERSION 0.2.0 LANGUAGES CXX DESCRIPTION "Lumina Player — premium cross-platform media player")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Quick Sql Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

qt_standard_project_setup(REQUIRES 6.4)
# Resources under :/qt/qml/<URI> (the paths main.cpp and QML icons use).
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
# Generate qmldir files for QML subdirectories (Components/, Theme/ ...).
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

# ---------------------------------------------------------------------------
# Main application target
# ---------------------------------------------------------------------------
# bin/ avoids a clash with the Lumina/ QML module dir on case-insensitive filesystems (macOS).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

qt_add_executable(lumina
    app/main.cpp
    app/ApplicationContext.h
    app/ApplicationContext.cpp
    app/SingleInstance.h
    app/SingleInstance.cpp
    app/UiTest.h
    app/UiTest.cpp
    src/utilities/Logger.h
    src/utilities/Logger.cpp
    src/utilities/TimeUtils.h
    src/utilities/TimeUtils.cpp
    src/core/PlayerController.h
    src/core/PlayerController.cpp
    src/core/PlaylistModel.h
    src/core/PlaylistModel.cpp
    src/storage/Database.h
    src/storage/Database.cpp
    src/storage/HistoryRepository.h
    src/storage/HistoryRepository.cpp
    src/storage/HistoryModel.h
    src/storage/HistoryModel.cpp
    src/engine/MpvEngine.h
    src/engine/MpvEngine.cpp
    src/engine/MpvVideoItem.h
    src/engine/MpvVideoItem.cpp
)

target_compile_definitions(lumina PRIVATE
    LUMINA_VERSION="${PROJECT_VERSION}"
)

option(LUMINA_MAC_BUNDLE "Build a macOS .app bundle (default OFF: plain bin/lumina for dev/CI)" OFF)

set_source_files_properties(resources/icons/app/lumina-256.png PROPERTIES QT_RESOURCE_ALIAS icons/lumina.png)
set_source_files_properties(ui/Theme/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

file(GLOB LUMINA_ICONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} resources/icons/*.svg)
foreach(icon IN LISTS LUMINA_ICONS)
    get_filename_component(icon_name ${icon} NAME)
    set_source_files_properties(${icon} PROPERTIES QT_RESOURCE_ALIAS icons/${icon_name})
endforeach()

qt_add_qml_module(lumina
    URI Lumina.Player
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/Theme/Theme.qml
        ui/Components/IconButton.qml
        ui/Components/TimeSlider.qml
        ui/Components/VolumeSlider.qml
        ui/Components/TrackMenu.qml
        ui/Components/ThemedMenuItem.qml
        ui/Components/PlaylistPanel.qml
        ui/Components/StatsOverlay.qml
        ui/Settings/SettingsDialog.qml
        ui/Components/Toast.qml
        ui/Components/VideoAdjustPopup.qml
    RESOURCES
        resources/icons/play.svg
        resources/icons/pause.svg
        resources/icons/stop.svg
        resources/icons/volume-high.svg
        resources/icons/volume-mute.svg
        resources/icons/fullscreen.svg
        resources/icons/fullscreen-exit.svg
        resources/icons/folder-open.svg
        resources/icons/skip-next.svg
        resources/icons/skip-previous.svg
        resources/icons/playlist.svg
        resources/icons/chapters.svg
        resources/icons/subtitles.svg
        resources/icons/settings.svg
        resources/icons/more.svg
        resources/icons/shuffle.svg
        resources/icons/repeat.svg
        resources/icons/repeat-one.svg
        resources/icons/app/lumina-256.png
)

if(APPLE AND LUMINA_MAC_BUNDLE)
    set(LUMINA_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app/lumina.icns)
    set_source_files_properties(${LUMINA_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(lumina PRIVATE ${LUMINA_ICNS})
    set_target_properties(lumina PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER io.github.mdmakky.Lumina
        MACOSX_BUNDLE_BUNDLE_NAME "Lumina Player"
        MACOSX_BUNDLE_ICON_FILE lumina.icns
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()

target_include_directories(lumina PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(lumina PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Quick
    Qt6::Sql
    Qt6::Test # UiTest driver (synthetic mouse/key events); inert unless env vars set
    PkgConfig::MPV
)

include(GNUInstallDirs)
install(TARGETS lumina
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/io.github.mdmakky.Lumina.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/io.github.mdmakky.Lumina.metainfo.xml
            DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    foreach(sz 16 32 48 64 128 256 512)
        install(FILES resources/icons/app/lumina-${sz}.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${sz}x${sz}/apps
                RENAME io.github.mdmakky.Lumina.png)
    endforeach()
    install(FILES resources/icons/app/lumina.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME io.github.mdmakky.Lumina.svg)
endif()

# Basic CPack (tarball / DEB-free; Flatpak is the primary Linux package).
set(CPACK_PACKAGE_NAME lumina-player)
set(CPACK_PACKAGE_VENDOR mdmakky)
set(CPACK_PACKAGE_CONTACT "makky.cse@gmail.com")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_GENERATOR TGZ)
include(CPack)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_executable(tst_timeutils
        tests/unit/tst_TimeUtils.cpp
        src/utilities/TimeUtils.cpp
    )
    target_include_directories(tst_timeutils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_timeutils PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME TimeUtils COMMAND tst_timeutils)

    add_executable(tst_playlistmodel
        tests/unit/tst_PlaylistModel.cpp
        src/core/PlaylistModel.cpp
        src/core/PlaylistModel.h
    )
    target_include_directories(tst_playlistmodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_playlistmodel PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME PlaylistModel COMMAND tst_playlistmodel)
    add_executable(tst_singleinstance
        tests/unit/tst_SingleInstance.cpp
        app/SingleInstance.cpp
    )
    target_include_directories(tst_singleinstance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)
    target_link_libraries(tst_singleinstance PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    add_test(NAME SingleInstance COMMAND tst_singleinstance)

    add_executable(tst_historyrepository
        tests/unit/tst_HistoryRepository.cpp
        src/storage/Database.cpp
        src/storage/HistoryRepository.cpp
        src/storage/HistoryModel.cpp
        src/utilities/Logger.cpp
    )
    target_include_directories(tst_historyrepository PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_historyrepository PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
    add_test(NAME HistoryRepository COMMAND tst_historyrepository)
endif()
