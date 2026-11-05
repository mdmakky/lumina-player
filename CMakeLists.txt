cmake_minimum_required(VERSION 3.28)

project(Lumina VERSION 0.2.0 LANGUAGES CXX DESCRIPTION "Lumina Player — premium cross-platform media player")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Quick Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

qt_standard_project_setup(REQUIRES 6.4)

# ---------------------------------------------------------------------------
# Main application target
# ---------------------------------------------------------------------------
qt_add_executable(lumina
    app/main.cpp
    app/ApplicationContext.h
    app/ApplicationContext.cpp
    src/utilities/Logger.h
    src/utilities/Logger.cpp
    src/utilities/TimeUtils.h
    src/utilities/TimeUtils.cpp
    src/core/PlayerController.h
    src/core/PlayerController.cpp
    src/core/PlaylistModel.h
    src/core/PlaylistModel.cpp
    src/engine/MpvEngine.h
    src/engine/MpvEngine.cpp
    src/engine/MpvVideoItem.h
    src/engine/MpvVideoItem.cpp
)

target_compile_definitions(lumina PRIVATE
    LUMINA_VERSION="${PROJECT_VERSION}"
)

set_source_files_properties(ui/Theme/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Resource prefix :/qt/qml/ (Qt >= 6.5); main.cpp loads qrc:/qt/qml/Lumina/Player/...
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_qml_module(lumina
    URI Lumina.Player
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/Theme/Theme.qml
        ui/Components/IconButton.qml
        ui/Components/TimeSlider.qml
    RESOURCES
        resources/icons/play.svg
        resources/icons/pause.svg
        resources/icons/stop.svg
        resources/icons/volume-high.svg
        resources/icons/volume-mute.svg
        resources/icons/fullscreen.svg
        resources/icons/fullscreen-exit.svg
        resources/icons/folder-open.svg
)

target_include_directories(lumina PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(lumina PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    PkgConfig::MPV
)

include(GNUInstallDirs)
install(TARGETS lumina
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
endif()
