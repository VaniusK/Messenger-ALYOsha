cmake_minimum_required(VERSION 3.16)
project(messenger_client)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network WebSockets)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_NO_DEBUG_OUTPUT QT_NO_INFO_OUTPUT QT_NO_WARNING_OUTPUT)
endif()

qt_add_executable(client WIN32
    src/core/main.cpp
    src/core/AuthManager.cpp
    include/AuthManager.hpp
    src/core/StateManager.cpp
    include/StateManager.hpp
    src/core/ChatManager.cpp
    include/ChatManager.hpp
    src/core/ConnectionManager.cpp
    include/ConnectionManager.hpp
)

qt_add_qml_module(client
    URI messenger_client_uri
    VERSION 1.0
    QML_FILES
        src/ui/main.qml
        src/ui/sign_in.qml
        src/ui/sign_up.qml
        src/ui/chat.qml
        src/ui/Sidebar.qml
        src/ui/ChatArea.qml
)
target_include_directories(client PRIVATE include)
target_link_libraries(client PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::WebSockets)

# Tests (optional, Qt Test or GTest)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    
    add_executable(client_tests tests/test_placeholder.cpp)
    target_include_directories(client_tests PRIVATE include)
    target_link_libraries(client_tests GTest::gtest_main)
    
    include(GoogleTest)
    gtest_discover_tests(client_tests)
endif()