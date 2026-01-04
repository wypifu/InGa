cmake_minimum_required(VERSION 3.12)
project("InGa")

if(TARGET InGa)
    return()
endif()

# 1. On définit les types de sortie pour qu'ils respectent ta structure
# CMAKE_CURRENT_BINARY_DIR sera dicté par le projet appelant (Inga_Demo)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# 2. Collecte des fichiers (Seulement le CORE pour l'instant)
set(INGA_SRC
    src/core/allocator.cpp
)

# 3. La cible (Target)
add_library(InGa SHARED ${INGA_SRC})

set_target_properties(InGa PROPERTIES
  CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN OFF
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${PROJECT_NAME} PUBLIC 
    $<$<CONFIG:Debug>:INGA_DEBUG>
  )
endif()

# 4. Gestion des INCLUDES (Le point CRUCIAL pour Neovim)
# On définit ce qui est interne et ce qui est exporté
target_include_directories(InGa 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 5. Flags et Macros
target_compile_definitions(InGa PRIVATE INGA_BUILD_DLL)

if(NOT MSVC)
    target_compile_options(InGa PRIVATE -Wall -Wextra -fvisibility=hidden -msse4.1)
    # Debug vs Release
    target_compile_options(InGa PRIVATE $<$<CONFIG:Debug>:-ggdb -O0>)
    target_compile_options(InGa PRIVATE $<$<CONFIG:Release>:-O3>)
else()
    target_compile_options(InGa PRIVATE /W4 /msse4.1)
endif()

if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
