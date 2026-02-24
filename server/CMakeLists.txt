cmake_minimum_required(VERSION 3.5)
project(alesha_server_app CXX)

include(CheckIncludeFileCXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare( # download jwt-cpp from github
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.2
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
FetchContent_Declare( # download bcrypt from github
    bcrypt
    GIT_REPOSITORY https://github.com/trusch/libbcrypt.git
    GIT_TAG master
)
fetchcontent_makeavailable(jwt-cpp bcrypt) # make available for the project

configure_file(
    "config.json"
    "${CMAKE_CURRENT_BINARY_DIR}/config.json"
    COPYONLY
)

add_executable(${PROJECT_NAME} src/main.cc) # create executable

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# add_subdirectory(drogon) 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon jwt-cpp::jwt-cpp bcrypt)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED) # find drogon
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon jwt-cpp::jwt-cpp bcrypt) # link libraries

# ##############################################################################

if (CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "c++20 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 23)
    message(STATUS "use c++20")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(src SRC_SRC) # get all files from src
aux_source_directory(src/repositories REPO_SRC) # get all files from src/repositories
aux_source_directory(src/utils UTILS_SRC) # get all files from src/utils

aux_source_directory(controllers CTL_SRC) # get all files from controllers
aux_source_directory(filters FILTER_SRC) # get all files from filters
aux_source_directory(plugins PLUGIN_SRC) # get all files from plugins
aux_source_directory(models MODEL_SRC) # get all files from models

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR})
# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)

target_include_directories(${PROJECT_NAME} PRIVATE ${bcrypt_SOURCE_DIR}/include) # include bcrypt headers
target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/include) # include project headers
target_sources(${PROJECT_NAME} 
                PRIVATE
               ${SRC_SRC}
               ${REPO_SRC}
               ${UTILS_SRC}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}) # add all sources to the executable
# ##############################################################################
# uncomment the following line for dynamically loading views 
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################

# Google Test setup
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
    fetchcontent_makeavailable(jwt-cpp)
    FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp
        GIT_TAG v0.7.2
    )
    FetchContent_Declare(
        bcrypt
        GIT_REPOSITORY https://github.com/trusch/libbcrypt.git
        GIT_TAG master
    )
    # Prevent GoogleTest from overriding compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest jwt-cpp bcrypt) # make available for the project

    enable_testing()
    
    add_executable(server_tests 
        tests/test_user_repository.cpp
    )
    target_sources(server_tests PRIVATE ${REPO_SRC})
    target_include_directories(${PROJECT_NAME} PRIVATE ${bcrypt_SOURCE_DIR}/include) # include bcrypt headers
    target_include_directories(server_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include) # include project headers
    target_link_libraries(server_tests GTest::gtest_main)
    target_link_libraries(server_tests Drogon::Drogon jwt-cpp::jwt-cpp bcrypt)
    aux_source_directory(models DROGON_MODELS)
    target_sources(server_tests PRIVATE ${DROGON_MODELS})
    
    include(GoogleTest)
    gtest_discover_tests(server_tests)
endif()
