cmake_minimum_required(VERSION 3.20)
project(InGa_Workspace)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VULKAN_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-SDK")

# Les headers sont communs
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_ROOT}/include" CACHE PATH "")

if(WIN32)
    set(VULKAN_OS_DIR "Windows")
    set(VULKAN_LIB_NAME "vulkan-1.lib")
else()
    set(VULKAN_OS_DIR "Linux")
    set(VULKAN_LIB_NAME "libvulkan.so")
endif()

# On pointe vers les dossiers spécifiques
set(Vulkan_LIBRARY "${VULKAN_SDK_ROOT}/lib/${VULKAN_OS_DIR}/${VULKAN_LIB_NAME}" CACHE PATH "")
set(VULKAN_BIN_DIR "${VULKAN_SDK_ROOT}/bin/${VULKAN_OS_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- AJOUT DES PROJETS ---
add_subdirectory(InGa)
add_subdirectory(IngaDemo)
