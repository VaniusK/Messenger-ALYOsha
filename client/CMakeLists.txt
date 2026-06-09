cmake_minimum_required(VERSION 3.16)
project(messenger_client)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Sql Network WebSockets Test Widgets Multimedia Svg Concurrent)
find_package(sodium CONFIG QUIET)
if(NOT sodium_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED libsodium)

    if(NOT TARGET sodium::libsodium)
        add_library(sodium::libsodium INTERFACE IMPORTED)
        target_include_directories(sodium::libsodium INTERFACE ${SODIUM_INCLUDE_DIRS})
        target_link_libraries(sodium::libsodium INTERFACE ${SODIUM_LIBRARIES})
        target_link_directories(sodium::libsodium INTERFACE ${SODIUM_LIBRARY_DIRS})
    endif()
endif()

#if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
#    add_compile_definitions(QT_NO_DEBUG_OUTPUT QT_NO_INFO_OUTPUT QT_NO_WARNING_OUTPUT)
#endif()

qt_add_executable(client #WIN32
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
    src/core/MediaCacheManager.cpp
    include/MediaCacheManager.hpp
    src/core/LocalChatStorage.cpp
    include/LocalChatStorage.hpp
    src/core/CryptoManager.cpp
    include/CryptoManager.hpp
    src/core/SecretChatManager.cpp
    include/SecretChatManager.hpp
    src/core/SecretDatabaseManager.cpp
    include/SecretDatabaseManager.hpp
    ../common/include/enums/WebsocketsMessagesTypes.h
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
        src/ui/CreateGroupPopup.qml
        src/ui/SettingsPopup.qml
        src/ui/AppTheme.qml
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
        assets/icons/check_single.svg
        assets/icons/check_double.svg
        assets/icons/camera.svg
        assets/icons/gear.svg
        assets/icons/person_plus.svg
        assets/icons/admin_shield.svg
        assets/icons/delete_user.svg
        assets/icons/owner_shield.svg
        assets/icons/create_group.svg
        assets/icons/change_color.svg
        assets/icons/lock.svg
        assets/icons/person_lock.svg
)
target_include_directories(client PRIVATE include)
target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/enums
)
target_link_libraries(client PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Sql Qt6::WebSockets Qt6::Widgets Qt6::Multimedia Qt6::Svg Qt6::Concurrent sodium::libsodium)

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
    src/core/CryptoManager.cpp
    include/CryptoManager.hpp
    src/core/SecretChatManager.cpp
    include/SecretChatManager.hpp
    src/core/SecretDatabaseManager.cpp
    include/SecretDatabaseManager.hpp
    ../common/include/enums/WebsocketsMessagesTypes.h
    )

    target_include_directories(client_tests PRIVATE include)
    target_include_directories(client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/enums
    )
    target_link_libraries(client_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    Qt6::Test
    sodium::libsodium
    )

    find_library(SODIUM_LIB sodium REQUIRED)
    target_link_libraries(client PRIVATE ${SODIUM_LIB})
    
    include(GoogleTest)
    gtest_discover_tests(client_tests)
endif()