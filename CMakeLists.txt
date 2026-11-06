cmake_minimum_required(VERSION 3.28)

project(Lumina VERSION 0.3.0 LANGUAGES CXX DESCRIPTION "Lumina Player — premium cross-platform media player")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Quick Sql Test)
find_package(Qt6 6.4 OPTIONAL_COMPONENTS LinguistTools)
if(UNIX AND NOT APPLE)
    find_package(Qt6 6.4 REQUIRED COMPONENTS DBus)
endif()

# libmpv: pkg-config where available (Linux, macOS, MSYS2); otherwise an
# mpv-dev SDK layout (include/mpv/client.h + lib/mpv.lib|libmpv.dll.a), found
# via -DLIBMPV_ROOT=<dir> (MSVC / shinchiro builds).
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MPV IMPORTED_TARGET mpv)
endif()
if(TARGET PkgConfig::MPV)
    set(LUMINA_MPV_TARGET PkgConfig::MPV)
else()
    find_path(MPV_INCLUDE_DIR mpv/client.h HINTS ${LIBMPV_ROOT} ENV LIBMPV_ROOT
              PATH_SUFFIXES include)
    find_library(MPV_LIBRARY NAMES mpv libmpv mpv-2 HINTS ${LIBMPV_ROOT} ENV LIBMPV_ROOT
                 PATH_SUFFIXES lib lib/x64 .)
    if(NOT MPV_INCLUDE_DIR OR NOT MPV_LIBRARY)
        message(FATAL_ERROR "libmpv not found. Install it (pkg-config 'mpv') or pass -DLIBMPV_ROOT=<mpv-dev dir>.")
    endif()
    add_library(lumina_mpv INTERFACE)
    target_include_directories(lumina_mpv INTERFACE ${MPV_INCLUDE_DIR})
    target_link_libraries(lumina_mpv INTERFACE ${MPV_LIBRARY})
    set(LUMINA_MPV_TARGET lumina_mpv)
endif()

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
# bin/ avoids a clash with the Lumina/ QML module dir on case-insensitive filesystems
# (macOS, Windows). Linux keeps the binary at <build>/lumina.
if(APPLE OR WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

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
    src/utilities/SettingsIO.h
    src/utilities/SettingsIO.cpp
    src/utilities/UrlUtils.h
    src/utilities/UrlUtils.cpp
    src/utilities/AudioFilters.h
    src/utilities/TrackLabels.h
    src/utilities/TrackLabels.cpp
    src/core/PlayerController.h
    src/core/PlayerController.cpp
    src/core/PlaylistModel.h
    src/core/PlaylistModel.cpp
    src/core/ShortcutManager.h
    src/core/ShortcutManager.cpp
    src/storage/Database.h
    src/storage/Database.cpp
    src/storage/HistoryRepository.h
    src/storage/HistoryRepository.cpp
    src/storage/HistoryModel.h
    src/storage/HistoryModel.cpp
    src/storage/LibraryRepository.h
    src/storage/LibraryRepository.cpp
    src/storage/LibraryModel.h
    src/storage/LibraryModel.cpp
    src/services/LibraryScanner.h
    src/services/LibraryScanner.cpp
    src/engine/MpvEngine.h
    src/engine/MpvEngine.cpp
    src/engine/MpvVideoItem.h
    src/engine/MpvVideoItem.cpp
    src/engine/MpvThumbnailer.h
    src/engine/MpvThumbnailer.cpp
    src/media/ThumbnailCache.h
    src/media/ThumbnailService.h
    src/media/ThumbnailService.cpp
    src/platform/PlatformIntegration.h
    src/platform/PlatformIntegration.cpp
    src/platform/Mpris.h
    src/platform/Mpris.cpp
)

if(UNIX AND NOT APPLE)
    target_sources(lumina PRIVATE src/platform/MprisIntegration.h src/platform/MprisIntegration.cpp)
    target_link_libraries(lumina PRIVATE Qt6::DBus)
elseif(APPLE)
    enable_language(OBJCXX)
    target_sources(lumina PRIVATE src/platform/MacIntegration.mm)
    set_source_files_properties(src/platform/MacIntegration.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    target_link_libraries(lumina PRIVATE "-framework IOKit" "-framework CoreFoundation"
                                         "-framework Foundation" "-framework MediaPlayer")
endif()

target_compile_definitions(lumina PRIVATE
    LUMINA_VERSION="${PROJECT_VERSION}"
)

option(LUMINA_MAC_BUNDLE "Build a macOS .app bundle (default OFF: plain bin/lumina for dev/CI)" OFF)

set_source_files_properties(resources/icons/app/lumina-256.png PROPERTIES QT_RESOURCE_ALIAS icons/lumina.png)
set(LUMINA_SETTINGS_ICONS "")
foreach(page general playback video audio subtitles appearance library shortcuts advanced about)
    list(APPEND LUMINA_SETTINGS_ICONS resources/icons/s-${page}.svg resources/icons/s-${page}-a.svg)
endforeach()
set_source_files_properties(ui/Settings/SettingsState.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
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
        ui/Components/PopupPanel.qml
        ui/Components/MenuRow.qml
        ui/Components/MenuChip.qml
        ui/Components/MenuSectionHeader.qml
        ui/Components/PanelSeparator.qml
        ui/Components/DelayStepper.qml
        ui/Components/ThemedToolTip.qml
        ui/Components/PlaylistPanel.qml
        ui/Components/StatsOverlay.qml
        ui/Settings/SettingsDialog.qml
        ui/Settings/SettingsState.qml
        ui/Settings/SettingsPage.qml
        ui/Settings/SettingSection.qml
        ui/Settings/SettingRow.qml
        ui/Settings/SettingsButton.qml
        ui/Settings/SegmentedControl.qml
        ui/Settings/SwitchRow.qml
        ui/Settings/StepperRow.qml
        ui/Settings/SliderRow.qml
        ui/Settings/ComboRow.qml
        ui/Settings/AudioOutputSection.qml
        ui/Settings/AudioDeviceRow.qml
        ui/Components/AudioBackdrop.qml
        ui/Settings/ButtonRow.qml
        ui/Settings/ShortcutsEditor.qml
        ui/Components/Toast.qml
        ui/Components/VideoAdjustPopup.qml
        ui/Components/EqualizerPopup.qml
        ui/Components/BookmarksPopup.qml
        ui/Components/OpenUrlDialog.qml
        ui/Components/MediaInfoDialog.qml
        ui/Components/ResizeGrips.qml
        ui/Library/LibraryView.qml
    RESOURCES
        resources/icons/play.svg
        resources/icons/pause.svg
        resources/icons/stop.svg
        resources/icons/volume-high.svg
        resources/icons/volume-mute.svg
        resources/icons/fullscreen.svg
        resources/icons/fullscreen-exit.svg
        resources/icons/folder-open.svg
        resources/icons/folder-open-dark.svg
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
        resources/icons/shuffle-dark.svg
        resources/icons/repeat-dark.svg
        resources/icons/repeat-one-dark.svg
        ${LUMINA_SETTINGS_ICONS}
        resources/icons/app/lumina-256.png
)

# Translations: lumina_en.ts is the English source (plural forms), lumina_bn.ts Bengali.
# `cmake --build <dir> --target update_translations` refreshes the .ts files.
if(Qt6LinguistTools_FOUND)
    file(GLOB LUMINA_TR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.qml ${CMAKE_CURRENT_SOURCE_DIR}/ui/*/*.qml
         ${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp)
    qt_add_translations(lumina
        TS_FILES translations/lumina_en.ts translations/lumina_bn.ts
        SOURCES ${LUMINA_TR_SOURCES}
        LUPDATE_OPTIONS -no-obsolete
        RESOURCE_PREFIX /i18n)
else()
    message(WARNING "Qt6 LinguistTools not found: building without translations (UI stays English)")
endif()

if(WIN32)
    target_sources(lumina PRIVATE
        src/platform/windows/SleepInhibitor_win.h
        src/platform/windows/SleepInhibitor_win.cpp)
    # Version resource + app icon (lumina.ico = PNG-embedded, built from lumina-*.png).
    set(LUMINA_ICO ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app/lumina.ico)
    string(REPLACE "." "," LUMINA_RC_VER "${PROJECT_VERSION}.0")
    configure_file(packaging/windows/lumina.rc.in ${CMAKE_BINARY_DIR}/lumina.rc @ONLY)
    target_sources(lumina PRIVATE ${CMAKE_BINARY_DIR}/lumina.rc)
    set_target_properties(lumina PROPERTIES WIN32_EXECUTABLE TRUE) # no console window
    target_compile_definitions(lumina PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(lumina PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Quick
    Qt6::Sql
    Qt6::Test # UiTest driver (synthetic mouse/key events); inert unless env vars set
    ${LUMINA_MPV_TARGET}
)

include(GNUInstallDirs)
install(TARGETS lumina
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    # Runtime libmpv DLL (mpv-dev SDK ships it next to the import lib; MSYS2 has it in bin/).
    find_file(MPV_RUNTIME_DLL NAMES libmpv-2.dll mpv-2.dll libmpv.dll mpv.dll
              HINTS ${LIBMPV_ROOT} ENV LIBMPV_ROOT PATH_SUFFIXES . bin lib lib/x64)
    if(MPV_RUNTIME_DLL)
        install(FILES ${MPV_RUNTIME_DLL} DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        message(STATUS "libmpv runtime DLL not found; copy it next to lumina.exe manually")
    endif()
endif()

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

    add_executable(tst_urlutils
        tests/unit/tst_UrlUtils.cpp
        src/utilities/UrlUtils.cpp
    )
    target_include_directories(tst_urlutils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_urlutils PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME UrlUtils COMMAND tst_urlutils)

    add_executable(tst_settingsio
        tests/unit/tst_SettingsIO.cpp
        src/utilities/SettingsIO.cpp
    )
    target_include_directories(tst_settingsio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_settingsio PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME SettingsIO COMMAND tst_settingsio)
    add_executable(tst_audiofilters tests/unit/tst_AudioFilters.cpp)
    target_include_directories(tst_audiofilters PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_audiofilters PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME AudioFilters COMMAND tst_audiofilters)

    add_executable(tst_tracklabels
        tests/unit/tst_TrackLabels.cpp
        src/utilities/TrackLabels.cpp
    )
    target_include_directories(tst_tracklabels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_tracklabels PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME TrackLabels COMMAND tst_tracklabels)

    add_executable(tst_playlistmodel
        tests/unit/tst_PlaylistModel.cpp
        src/core/PlaylistModel.cpp
        src/core/PlaylistModel.h
        src/utilities/UrlUtils.cpp
    )
    target_include_directories(tst_playlistmodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_playlistmodel PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME PlaylistModel COMMAND tst_playlistmodel)
    add_executable(tst_shortcutmanager
        tests/unit/tst_ShortcutManager.cpp
        src/core/ShortcutManager.cpp
        src/core/ShortcutManager.h
    )
    target_include_directories(tst_shortcutmanager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_shortcutmanager PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
    add_test(NAME ShortcutManager COMMAND tst_shortcutmanager)
    add_executable(tst_singleinstance
        tests/unit/tst_SingleInstance.cpp
        app/SingleInstance.cpp
    )
    target_include_directories(tst_singleinstance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)
    target_link_libraries(tst_singleinstance PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    add_test(NAME SingleInstance COMMAND tst_singleinstance)

    add_executable(tst_translations tests/unit/tst_Translations.cpp)
    target_compile_definitions(tst_translations PRIVATE LUMINA_TS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/translations")
    target_link_libraries(tst_translations PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME Translations COMMAND tst_translations)

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

    add_executable(tst_libraryscanner
        tests/unit/tst_LibraryScanner.cpp
        src/services/LibraryScanner.cpp
        src/storage/Database.cpp
        src/storage/LibraryRepository.cpp
        src/storage/LibraryModel.cpp
        src/utilities/Logger.cpp
    )
    target_include_directories(tst_libraryscanner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_libraryscanner PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
    add_test(NAME LibraryScanner COMMAND tst_libraryscanner)

    add_executable(tst_mpris
        tests/unit/tst_Mpris.cpp
        src/platform/Mpris.cpp
    )
    target_include_directories(tst_mpris PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_mpris PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME Mpris COMMAND tst_mpris)
endif()

if(BUILD_TESTING)
    add_executable(tst_thumbnailservice
        tests/unit/tst_ThumbnailService.cpp
        src/engine/MpvThumbnailer.cpp
    )
    target_include_directories(tst_thumbnailservice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tst_thumbnailservice PRIVATE Qt6::Core Qt6::Gui Qt6::Test PkgConfig::MPV)
    add_test(NAME ThumbnailService COMMAND tst_thumbnailservice)
endif()
