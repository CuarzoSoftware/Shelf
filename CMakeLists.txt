cmake_minimum_required(VERSION 3.20)

project(Shelf)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
    Gui
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    WidgetsAddons
)

find_package(KF6Config REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(Wayland REQUIRED)

add_executable(cuarzo-shelf
    src/main.cpp
    src/Dock.h src/Dock.cpp
    src/DockContainer.h src/DockContainer.cpp
    src/Global.h src/Global.cpp
    src/Shelf.h src/Shelf.cpp
    src/ItemModel.h src/ItemModel.cpp
    src/Application.h src/Application.cpp
    src/protocols/wlr-foreign-toplevel-management-unstable-v1.c src/protocols/wlr-foreign-toplevel-management-unstable-v1-client.h
)

target_include_directories(cuarzo-shelf PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(cuarzo-shelf PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::GuiPrivate
    KF6::CoreAddons
    KF6::I18n
    KF6::WidgetsAddons
    KF6::ConfigCore 
    KF6::ConfigGui
    LayerShellQt::Interface
    Wayland::Client
)

install(TARGETS cuarzo-shelf ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
