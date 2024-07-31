cmake_minimum_required(VERSION 3.10)
project(mediaengine LANGUAGES C ASM)


set(PSP_FW_VERSION 500)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -G0 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostartfiles")

include_directories(${CMAKE_SOURCE_DIR})

set(SOURCE_FILES
    me_stub.S
    main.o
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    pspkernel
)


# Define PRX properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    PSP_FW_VERSION ${PSP_FW_VERSION}
    OUTPUT_NAME "mediaengine"
)

# Add post-build steps to generate stubs
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND psp-build-exports --build-stubs ${CMAKE_SOURCE_DIR}/exports.exp
    BYPRODUCTS MediaEngine.S
    COMMENT "Generating MediaEngine.S"
)

add_custom_target(MediaEngine.S ALL
    DEPENDS ${PROJECT_NAME}
    COMMENT "Build MediaEngine.S stubs"
)