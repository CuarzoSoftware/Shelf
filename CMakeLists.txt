cmake_minimum_required(VERSION 3.20)

project(Shelf)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

find_package(PkgConfig)
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    OpenGLWidgets
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    WidgetsAddons
)

find_package(KF6Config REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(Wayland REQUIRED)
find_package(Fontconfig REQUIRED)

pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(WAYLAND_EGL REQUIRED wayland-egl)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

add_compile_definitions(
    SK_GL
    SK_GANESH)

add_executable(cuarzo-shelf
    src/main.cpp
    src/controllers/AppController.cpp src/controllers/AppController.h src/controllers/ItemController.cpp src/controllers/ItemController.h src/core/Constants.h src/core/Object.cpp src/core/Object.h src/core/Shelf.cpp src/core/Shelf.h src/core/Utils.cpp src/core/Utils.h src/input/Seat.cpp src/input/Seat.h src/interfaces/Renderable.h src/items/Item.cpp src/items/Item.h src/protocols/wlr-foreign-toplevel-management-unstable-v1-client.h src/protocols/wlr-foreign-toplevel-management-unstable-v1.c src/protocols/wlr-foreign-toplevel-management-unstable-v1.xml src/protocols/wlr-layer-shell-unstable-v1.c src/protocols/wlr-layer-shell-unstable-v1.h src/protocols/wlr-layer-shell-unstable-v1.xml src/windows/Dock.cpp src/windows/Dock.h src/windows/Window.cpp src/windows/Window.h
    src/controllers/SeparatorController.h src/controllers/SeparatorController.cpp
    src/controllers/TrashController.h src/controllers/TrashController.cpp
    src/windows/Tooltip.h src/windows/Tooltip.cpp
    src/interfaces/PointerEventsListener.h
)

target_include_directories(cuarzo-shelf PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${WAYLAND_EGL_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    "third_party/skia/")

target_link_libraries(cuarzo-shelf PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::GuiPrivate
    Qt6::OpenGLWidgets
    KF6::CoreAddons
    KF6::I18n
    KF6::WidgetsAddons
    KF6::ConfigCore 
    KF6::ConfigGui
    LayerShellQt::Interface
    Wayland::Client
    PkgConfig::EGL
    PkgConfig::GLESv2
    ${WAYLAND_EGL_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/third_party/skia/libskia.a
)

install(TARGETS cuarzo-shelf ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
