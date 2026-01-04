cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project("IngaDemo")

# 1. CONFIGURATION GLOBALE
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    set(PLATFORM "Windows")
else()
    set(PLATFORM "Linux")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CONFIG ${CMAKE_BUILD_TYPE})

# 2. DEFINITION DES CHEMINS (On prépare tout avant de créer les cibles)
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(INGA_PATH "${ROOT_DIR}/src/libraries/InGa")

# On définit les dossiers de sortie (ABSOLUS pour éviter l'erreur RELATIVE_PATH)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PLATFORM}/${PROJECT_NAME}/${CONFIG}")
set(INGA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${PLATFORM}/InGa/${CONFIG}")

# 3. AJOUT DU MOTEUR (On le fait AVANT l'exécutable pour que la cible 'InGa' existe)
add_subdirectory(${INGA_PATH} ${INGA_OUTPUT_DIR})

# 4. CRÉATION DE L'EXÉCUTABLE
set(SRC src/main.cpp)
add_executable(${PROJECT_NAME} ${SRC})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${PROJECT_NAME} PUBLIC 
    $<$<CONFIG:Debug>:INGA_DEBUG>
  )
endif()


# 5. RPATH (Lien dynamique relatif entre l'exe et le .so)
if(UNIX)
    # On force CMake à utiliser les chemins absolus pour calculer le relatif
    get_filename_component(ABS_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" ABSOLUTE)
    get_filename_component(ABS_LIB_DIR "${INGA_OUTPUT_DIR}" ABSOLUTE)
    
    file(RELATIVE_PATH REL_PATH_TO_INGA "${ABS_BIN_DIR}" "${ABS_LIB_DIR}")
    
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        LINK_FLAGS "-Wl,-rpath,'$ORIGIN/${REL_PATH_TO_INGA}'"
    )
endif()

# 6. FLAGS DE COMPILATION
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /O2 /msse4.1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wshadow -msse4.1)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-ggdb -O0 -pg>)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# 7. LINKAGE
target_link_libraries(${PROJECT_NAME} PRIVATE InGa)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32 shell32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
