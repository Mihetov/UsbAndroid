cmake_minimum_required(VERSION 3.21)

# Блокируем авто-деплой Android для предотвращения зацикливания макросов Qt
set(QT_ANDROID_NO_AUTOMATIC_FINALIZATION ON)

project(UsbAndroid VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Находим зависимости Qt
find_package(Qt6 REQUIRED COMPONENTS Core Quick)

# 2. Настраиваем стандартное окружение Qt6
qt_standard_project_setup(REQUIRES 6.5)

# ==========================================================
# 3. НАСТРОЙКА СБОРКИ ТОЛЬКО МОДУЛЕЙ RTU (БЕЗ TCP И LIBNET)
# ==========================================================
file(GLOB MODBUS_POSSIBLE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/[Mm]odbus")
list(GET MODBUS_POSSIBLE_DIRS 0 MODBUS_DIR)

file(GLOB_RECURSE MODBUS_SOURCES
    "${MODBUS_DIR}/src/*.cpp"
)
list(FILTER MODBUS_SOURCES EXCLUDE REGEX ".*src/TCP/.*")

message(STATUS "Итоговые файлы Modbus для сборки: ${MODBUS_SOURCES}")
# ==========================================================

# 4. Создаем исполняемый файл
qt_add_executable(appUsbAndroid MANUAL_FINALIZATION
    src/main.cpp
    src/usbscanner.cpp
    src/usbscanner.h
    src/backend.cpp
    src/backend.h
    src/registermodel.cpp
    src/registermodel.h
    ${MODBUS_SOURCES}
)

# 5. Прописываем пути к заголовочным файлам Modbus
target_include_directories(appUsbAndroid PRIVATE
    "${MODBUS_DIR}/include"
    "${MODBUS_DIR}/include/MB"
    "${MODBUS_DIR}/src"
)

# 6. Регистрируем QML-модуль
qt_add_qml_module(appUsbAndroid
    URI UsbAndroid
    VERSION 1.0
    QML_FILES
        src/Main.qml
)

# 7. Настройки для Android-пакета
set_target_properties(appUsbAndroid PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

# 8. Линкуем Qt библиотеки
target_link_libraries(appUsbAndroid
    PRIVATE
        Qt6::Core
        Qt6::Quick
)

# 9. Ручная финализация Android-деплоя
if(ANDROID)
    qt_finalize_target(appUsbAndroid)
endif()

include(GNUInstallDirs)
install(TARGETS appUsbAndroid
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)