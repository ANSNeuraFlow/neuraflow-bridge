cmake_minimum_required(VERSION 3.16)

project(NeuraFlowBridge VERSION 0.1 LANGUAGES CXX)

set(SRC_DIR "src")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Qml Quick SerialPort ShaderTools Network WebSockets LinguistTools Widgets PrintSupport)

qt_standard_project_setup(
    REQUIRES 6.8
    I18N_TRANSLATED_LANGUAGES pl
)

set(EXTRA_QML_IMPORTS
    "${CMAKE_BINARY_DIR}/src/Config"
)

set(QML_IMPORT_PATH ${EXTRA_QML_IMPORTS} CACHE STRING "Qt Creator QML Import Paths" FORCE)

set(CMAKE_AUTORCC ON)

qt_add_executable(NeuraFlowBridgeApp
    resources.qrc
    ${SRC_DIR}/main.cpp
)

set(SOURCES
    main.cpp
    utils.h
    utils.cpp
    QmlUtils.h
    QmlUtils.cpp
    TranslationManager.h
    TranslationManager.cpp
    DeepLinkHandler.h
    DeepLinkHandler.cpp
    CallbackServer.h
    CallbackServer.cpp
    TokenStore.h
    TokenStore.cpp
    AuthManager.h
    AuthManager.cpp
    DeviceManager.h
    DeviceManager.cpp
    BridgeDeviceClient.h
    BridgeDeviceClient.cpp
    StreamUploader.h
    StreamUploader.cpp
    BackendController.h
    BackendController.cpp
    SessionManager.h
    SessionManager.cpp
    DataProcessor.h
    DataProcessor.cpp
    TimeSeriesController.h
    TimeSeriesController.cpp
    QCustomPlotQuickItem.h
    QCustomPlotQuickItem.cpp
    external/qcustomplot/qcustomplot.h
    external/qcustomplot/qcustomplot.cpp
)

list(TRANSFORM SOURCES PREPEND "${SRC_DIR}/")

set(QML_FILES
    Main.qml
    DashboardPage.qml
    ClockDigit.qml
    AnimatedTimer.qml
    BrandLogo.qml
    BaseButton.qml
    BaseBusyIndicator.qml
    TimeSeriesView.qml
    ChannelBar.qml
)

list(TRANSFORM QML_FILES PREPEND "${SRC_DIR}/")

set(RESOURCES
    "icons/brand_logo.svg"
    "icons/connection.svg"
    "icons/warning.svg"
    "fonts/digital_7.mono.ttf"
    "fonts/roboto_mono.ttf"
    "fonts/space_grotesk_bold.ttf"
)

list(TRANSFORM RESOURCES PREPEND "${SRC_DIR}/")

qt_add_qml_module(NeuraFlowBridgeApp
    URI NeuraFlowBridge
    VERSION 1.0
    QML_FILES
    ${QML_FILES}
    SOURCES
    ${SOURCES}
    RESOURCES
    ${RESOURCES}
)

qt_add_translations(NeuraFlowBridgeApp
    TS_FILE_BASE qml
    TS_FILE_DIR i18n
    RESOURCE_PREFIX qt/qml/NeuraFlowBridge/i18n
)

set(SHADERS
    "animated-text.frag"
)

list(TRANSFORM SHADERS PREPEND "${SRC_DIR}/shaders/")

qt6_add_shaders(NeuraFlowBridgeApp "NeuraFlowBridgeAppShaders"
    PREFIX
    "/shaders"
    BASE
    "${SRC_DIR}/shaders"
    FILES
    ${SHADERS}
)

add_subdirectory(${SRC_DIR}/Config)

# Qt for iOS sets MACOSX_BUNDLE_GU    Qt6::QuickShapesI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(NeuraFlowBridgeApp PROPERTIES
    #    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NeuraFlowBridgeApp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(NeuraFlowBridgeApp PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/external/qcustomplot
)

target_link_libraries(NeuraFlowBridgeApp PRIVATE
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::SerialPort
    Qt6::Network
    Qt6::WebSockets
    Configplugin
)

qt_import_qml_plugins(NeuraFlowBridgeApp)

include(GNUInstallDirs)
install(TARGETS NeuraFlowBridgeApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET NeuraFlowBridgeApp
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
