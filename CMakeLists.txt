cmake_minimum_required(VERSION 3.13)
message("Build type: " ${CMAKE_BUILD_TYPE})

include_directories(
    external
    external/SDL2/include
    external/glm
    external/physx/physx/include
    external/physx/pxshared/include
    external/stb
)
project(game)
add_executable(game src/main.cpp external/glad/glad.c)
target_compile_definitions(game PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>")
if(UNIX)
    target_link_libraries(game general SDL2 GL dl pthread stdc++fs)
    foreach(LIB_NAME
        libPhysXCooking_static_64.a
        libPhysXVehicle_static_64.a
        libPhysXExtensions_static_64.a
        libPhysX_static_64.a
        libPhysXPvdSDK_static_64.a
        libPhysXCharacterKinematic_static_64.a
        libPhysXFoundation_static_64.a
        libPhysXCommon_static_64.a)
        target_link_libraries(game debug ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/linux.clang/checked/${LIB_NAME})
        target_link_libraries(game optimized ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/linux.clang/release/${LIB_NAME})
    endforeach()
elseif(WIN32)
    target_link_libraries(game opengl32)
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -fno-exceptions")
elseif(MSVC)
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif (${flag_var} MATCHES "/MD")
    endforeach (flag_var)
endif()

add_custom_command(TARGET game POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> "${CMAKE_SOURCE_DIR}/bin/")
