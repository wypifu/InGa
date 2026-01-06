cmake_minimum_required(VERSION 3.20)
project(InGa)

# --- Dependencies Detection ---

# Vulkan is mandatory
find_package(Vulkan REQUIRED)

if(UNIX AND NOT APPLE)
    # X11 is usually found via find_package
    find_package(X11 REQUIRED)
    
    # XCB and Wayland are best found via PkgConfig
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB REQUIRED xcb)
    pkg_check_modules(WAYLAND REQUIRED wayland-client)
endif()

# --- Source Collection ---
file(GLOB_RECURSE INGA_SRC 
    "src/core/*.cpp"
    "src/gfx/*.cpp"
)

# --- Target Definition ---
add_library(InGa SHARED ${INGA_SRC})

# --- Compile Definitions ---
target_compile_definitions(InGa PRIVATE INGA_BUILD_DLL)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(InGa PUBLIC INGA_DEBUG)
endif()

find_package(Vulkan REQUIRED)

# --- Include Directories ---
target_include_directories(InGa
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${Vulkan_INCLUDE_DIRS}
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Linker Setup ---
target_link_libraries(InGa PUBLIC 
  Vulkan::Vulkan
)

if(UNIX AND NOT APPLE)
    # Link native Linux windowing libraries
    target_link_libraries(InGa PRIVATE 
        ${X11_LIBRARIES}
        ${XCB_LIBRARIES}
        ${WAYLAND_LIBRARIES}
    )
    
    # Add includes for these libraries
    target_include_directories(InGa PRIVATE 
        ${X11_INCLUDE_DIR}
        ${XCB_INCLUDE_DIRS}
        ${WAYLAND_INCLUDE_DIRS}
    )
endif()

# --- Properties & Flags ---
set_target_properties(InGa PROPERTIES
    SKIP_BUILD_RPATH FALSE
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH ""
)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(NOT MSVC)
    target_compile_options(InGa PRIVATE -Wall -Wextra -fvisibility=hidden -msse4.1)
else()
    target_compile_options(InGa PRIVATE /W4)
    add_compile_options(/wd4251) # wd = Write Disable (silence warning 4251)
endif()
