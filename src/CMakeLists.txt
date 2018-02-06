file(GLOB SOURCES *.c *.h)

find_package(POPT REQUIRED)
add_executable(${TARGET_CMD_NAME} ${SOURCES})
if(ENABLE_MPI)
    add_definitions(-DENABLE_MPI)
endif()
if(ENABLE_CLUSTER)
    add_definitions(-DENABLE_CLUSTER)
endif()
if(ENABLE_PTHRADS)
    add_definitions(-DENABLE_PTHRADS)
endif()
if(DEF_N_THREADS)
    add_definitions(-DDEF_N_THREADS=${DEF_N_THREADS})
endif()
if(MAX__N_THREADS)
    add_definitions(-DMAX_N_THREADS=${MAX_N_THREADS})
endif()
target_include_directories(${TARGET_CMD_NAME}
                           PRIVATE ${CMAKE_BINARY_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_SOURCE_DIR}/lib
                                   ${POPT_INCLUDE_DIRS})
target_link_libraries(${TARGET_CMD_NAME} epsilon-lib ${POPT_LIBRARY})

