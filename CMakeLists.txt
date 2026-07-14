cmake_minimum_required(VERSION 3.21)

project(UsbAndroid VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appUsbAndroid
    src/main.cpp
    src/usbscanner.cpp
    src/usbscanner.h
)

qt_add_qml_module(appUsbAndroid
    URI UsbAndroid
    VERSION 1.0
    QML_FILES
        src/Main.qml
)

# set_target_properties(appUsbAndroid PROPERTIES
#     QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
# )

target_link_libraries(appUsbAndroid
    PRIVATE Qt6::Core Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appUsbAndroid
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
