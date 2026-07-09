cmake_minimum_required(VERSION 3.21)

project(QtQmlQrAndroid VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick Multimedia Concurrent)

option(QR_APP_FETCH_ZXING "Fetch zxing-cpp with FetchContent" OFF)

if (NOT TARGET ZXing::ZXing)
    find_package(ZXing CONFIG QUIET)
endif()

if (NOT TARGET ZXing::ZXing AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/zxing-cpp/CMakeLists.txt")
    set(ZXING_READERS ON CACHE BOOL "" FORCE)
    set(ZXING_WRITERS OFF CACHE STRING "" FORCE)
    set(ZXING_C_API OFF CACHE BOOL "" FORCE)
    set(ZXING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZXING_EXAMPLES_QT OFF CACHE BOOL "" FORCE)
    set(ZXING_BLACKBOX_TESTS OFF CACHE BOOL "" FORCE)
    set(ZXING_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(ZXING_TEST_DOTNET OFF CACHE BOOL "" FORCE)
    set(ZXING_TEST_GO OFF CACHE BOOL "" FORCE)
    set(ZXING_TEST_INSTALL OFF CACHE BOOL "" FORCE)
    set(ZXING_PYTHON_MODULE OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/zxing-cpp)
endif()

if (NOT TARGET ZXing::ZXing AND QR_APP_FETCH_ZXING)
    include(FetchContent)
    FetchContent_Declare(
        zxing-cpp
        GIT_REPOSITORY https://github.com/zxing-cpp/zxing-cpp.git
        GIT_TAG v2.3.0
    )
    FetchContent_MakeAvailable(zxing-cpp)
endif()

if (NOT TARGET ZXing::ZXing)
    message(FATAL_ERROR
        "ZXing::ZXing was not found. Install zxing-cpp, add it as third_party/zxing-cpp, "
        "or configure with -DQR_APP_FETCH_ZXING=ON."
    )
endif()

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(appQtQmlQrAndroid
    src/main.cpp
    src/QrGenerator.cpp
    src/QrGenerator.h
    src/QrScanner.cpp
    src/QrScanner.h
    third_party/nayuki-qr/qrcodegen.cpp
    third_party/nayuki-qr/qrcodegen.hpp
)

qt_add_qml_module(appQtQmlQrAndroid
    URI QrApp
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(appQtQmlQrAndroid
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Multimedia
        Qt6::Concurrent
        ZXing::ZXing
)

target_include_directories(appQtQmlQrAndroid PRIVATE src third_party/nayuki-qr)

set_target_properties(appQtQmlQrAndroid PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if (ANDROID)
    set_target_properties(appQtQmlQrAndroid PROPERTIES
        QT_ANDROID_PACKAGE_NAME "org.codex.qtqmlqrandroid"
        QT_ANDROID_APP_NAME "Qt QR Scanner"
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android/package"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 36
        QT_ANDROID_COMPILE_SDK_VERSION 36
        QT_ANDROID_SDK_BUILD_TOOLS_REVISION 37.0.0
    )
endif()

install(TARGETS appQtQmlQrAndroid
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
