cmake_minimum_required(VERSION 3.12)
project(TP15_Sincronizacion CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(EXTRA_LIBS ws2_32)
else()
    set(EXTRA_LIBS pthread)
endif()

add_executable(cristian src/ejercicio_c_cristian/main.cpp)
target_link_libraries(cristian PRIVATE ${EXTRA_LIBS})
set_target_properties(cristian PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
