cmake_minimum_required(VERSION 3.15)
project(CommunityModules)

enable_testing()

set(sdk_folder
    ../../Core
    )

include_directories(
    ../../Core
    )

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_UNICODE)
if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

if (MSVC)
    # Floating Point Model: Fast (/fp:fast)
    # Buffer Security Check: No (/GS-)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS-")
endif()

add_subdirectory(AGain)
