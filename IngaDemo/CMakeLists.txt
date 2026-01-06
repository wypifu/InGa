cmake_minimum_required(VERSION 3.20)
project(IngaDemo)

add_executable(IngaDemo
  src/main.cpp
)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Linkage au moteur
target_link_libraries(IngaDemo PRIVATE InGa)

if(UNIX)
# Puisque libInGa.so sera juste à côté de l'exécutable dans build/bin/
    set_target_properties(IngaDemo PROPERTIES
      INSTALL_RPATH "$ORIGIN/"
       BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()


