cmake_minimum_required(VERSION 3.15)
project(OpenMind VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten for WebAssembly")

    add_executable(openmind
        src/bridge/openmind_bridge.cpp
        src/engine/openmind_engine.cpp
        src/engine/VoxelOctree.cpp
        src/engine/PhysicsEngine.cpp
        src/llm/JSONValidator.cpp
    )

    target_include_directories(openmind PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/llm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/agents
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge
    )

    set(EMSCRIPTEN_FLAGS
        "-s WASM=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s INITIAL_MEMORY=67108864"
        "-s MAXIMUM_MEMORY=268435456"
        "-s MODULARIZE=1"
        "-s EXPORT_NAME='OpenMindModule'"
        "-s ENVIRONMENT='web'"
        "-s FILESYSTEM=0"
        "-s SINGLE_FILE=0"
        "-s MALLOC=emmalloc"
        "-O3"
    )

    string(REPLACE ";" " " EMSCRIPTEN_FLAGS_STR "${EMSCRIPTEN_FLAGS}")
    set_target_properties(openmind PROPERTIES LINK_FLAGS "${EMSCRIPTEN_FLAGS_STR}")

    target_compile_options(openmind PRIVATE
        "-O3"
    )

else()
    message(STATUS "Building native C++")

    add_library(OpenMindCore STATIC
        src/engine/openmind_engine.cpp
        src/engine/VoxelOctree.cpp
        src/engine/PhysicsEngine.cpp
    )

    target_include_directories(OpenMindCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/llm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/agents
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge
    )

    add_executable(OpenMind src/main.cpp)
    target_link_libraries(OpenMind PRIVATE OpenMindCore)

    add_executable(OpenMindPhysics src/main_physics.cpp)
    target_link_libraries(OpenMindPhysics PRIVATE OpenMindCore)

    if(MSVC)
        target_compile_options(OpenMindCore PUBLIC /W4)
    else()
        target_compile_options(OpenMindCore PUBLIC -Wall -Wextra -pedantic)
    endif()

    enable_testing()
    add_test(NAME VoxelEngineTest COMMAND OpenMind)
    add_test(NAME PhysicsEngineTest COMMAND OpenMindPhysics)
endif()
