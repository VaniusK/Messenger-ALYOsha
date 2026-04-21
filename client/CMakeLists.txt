cmake_minimum_required(VERSION 3.16)
project(messenger_client)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network WebSockets Test Widgets Multimedia Svg)

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
    src/core/MediaManager.cpp
    include/MediaManager.hpp
    src/core/VoiceManager.cpp
    include/VoiceManager.hpp
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
        src/ui/MediaPickerPopup.qml
        src/ui/MediaPreviewDialog.qml
    RESOURCES
        assets/icons/clip.svg
        assets/icons/clip_active.svg
        assets/icons/photo.svg
        assets/icons/document.svg
        assets/icons/bookmark.svg
        assets/icons/send.svg
        assets/icons/send_active.svg
        assets/icons/mic.svg
        assets/icons/play.svg
        assets/icons/pause.svg
        assets/icons/play_blue.svg
        assets/icons/pause_blue.svg
        assets/icons/volume.svg
)
target_include_directories(client PRIVATE include)
target_link_libraries(client PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::WebSockets Qt6::Widgets Qt6::Multimedia Qt6::Svg)

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
    
    add_executable(client_tests 
    src/core/StateManager.cpp
    include/StateManager.hpp
    src/core/AuthManager.cpp
    include/AuthManager.hpp
    src/core/ConnectionManager.cpp
    include/ConnectionManager.hpp
    src/core/ChatManager.cpp
    include/ChatManager.hpp
    tests/test_state_manager.cpp
    tests/mocks/FakeNetworkReply.hpp
    tests/test_auth_manager.cpp
    tests/test_chat_manager.cpp
    )

    target_include_directories(client_tests PRIVATE include)
    target_link_libraries(client_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    Qt6::Test
    )
    
    include(GoogleTest)
    gtest_discover_tests(client_tests)
endif()