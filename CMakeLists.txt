cmake_minimum_required(VERSION 3.15)

set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")

project(GMPI_Samples)

enable_testing()

set(sdk_folder
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
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

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()
