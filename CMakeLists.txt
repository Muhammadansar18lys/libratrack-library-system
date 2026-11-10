cmake_minimum_required(VERSION 3.24)
project(LibraTrack VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBRATRACK_BUILD_API "Build the Crow/MySQL REST API" ON)
if(LIBRATRACK_BUILD_API)
    find_package(Crow CONFIG REQUIRED)
    find_package(mysql-concpp REQUIRED)
    add_executable(libratrack-api
        backend/src/main.cpp
        backend/src/database.cpp
        backend/src/library_service.cpp
        backend/src/date_utils.cpp
    )
    target_include_directories(libratrack-api PRIVATE backend/include)
    target_link_libraries(libratrack-api PRIVATE Crow::Crow mysql::concpp-jdbc)
    if(MSVC)
        target_compile_options(libratrack-api PRIVATE /W4 /permissive-)
    else()
        target_compile_options(libratrack-api PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(libratrack-unit-tests tests/test_core.cpp backend/src/date_utils.cpp)
    target_include_directories(libratrack-unit-tests PRIVATE backend/include)
    target_compile_features(libratrack-unit-tests PRIVATE cxx_std_20)
    add_test(NAME libratrack-core COMMAND libratrack-unit-tests)
endif()
