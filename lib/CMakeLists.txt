file(GLOB SOURCES *.c *.h)
file(GLOB MSVC_SRC RELATIVE msvc *.c)
if(MSVC)
    set(SOURCES ${SOURCES} ${MSVC_SRC})
endif()
add_library(epsilon-lib ${SOURCES})
set_target_properties(epsilon-lib PROPERTIES
			OUTPUT_NAME "${TARGET_LIB_NAME}"
            PREFIX "lib"
            SOVERSION ${EPSILON_VERSION_MAJOR}.${EPSILON_VERSION_MINOR}
            VERSION ${EPSILON_VERSION})
target_include_directories(epsilon-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_SOURCE_DIR}/src)
