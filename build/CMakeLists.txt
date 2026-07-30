cmake_minimum_required(VERSION 3.15)
project(OpenMind VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten for WebAssembly")

    add_executable(openmind
        openmind_bridge.cpp
        openmind_engine.cpp
        VoxelOctree.cpp
        PhysicsEngine.cpp
    )

    target_include_directories(openmind PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    set(EMSCRIPTEN_FLAGS
        "-s WASM=1"
        "-s EXPORTED_FUNCTIONS=['_initWorld','_setBlock','_setBlockSimple','_removeBlock','_getBlockData','_getWorldStats','_setTimeScale','_setGravity','_setAmbientTemperature','_setWind','_rewindTime','_saveSnapshot','_getPendingFragments','_addAgent','_setAgentPosition','_getAgentData','_getAgentCount','_generateFromPrompt','_teleportCamera','_setOverlay','_exportCSV','_exportGLTF','_getLastExportCSV','_getLastExportGLTF','_saveWorld','_cleanup','_tickPhysics','_tickPhysicsDelta','_setWeather','_getWeatherType','_getWeather','_setTimeOfDay','_getTimeOfDay','_getSunPosition','_getSunlightIntensity','_setCycleDuration','_malloc','_free']"
        "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s INITIAL_MEMORY=16777216"
        "-s MAXIMUM_MEMORY=268435456"
        "-s MODULARIZE=1"
        "-s EXPORT_NAME='OpenMindModule'"
        "-s ENVIRONMENT='web'"
        "-s FILESYSTEM=0"
        "-s SINGLE_FILE=0"
        "-s MALLOC=emmalloc"
        "-O2"
    )

    string(REPLACE ";" " " EMSCRIPTEN_FLAGS_STR "${EMSCRIPTEN_FLAGS}")
    set_target_properties(openmind PROPERTIES LINK_FLAGS "${EMSCRIPTEN_FLAGS_STR}")

    target_compile_options(openmind PRIVATE
        "-O2"
        "-sEXPORTED_FUNCTIONS=['_initWorld','_setBlock','_setBlockSimple','_removeBlock','_getBlockData','_getWorldStats','_setTimeScale','_setGravity','_setAmbientTemperature','_setWind','_rewindTime','_saveSnapshot','_getPendingFragments','_addAgent','_setAgentPosition','_getAgentData','_getAgentCount','_generateFromPrompt','_teleportCamera','_setOverlay','_exportCSV','_exportGLTF','_getLastExportCSV','_getLastExportGLTF','_saveWorld','_cleanup','_tickPhysics','_tickPhysicsDelta','_setWeather','_getWeatherType','_getWeather','_setTimeOfDay','_getTimeOfDay','_getSunPosition','_getSunlightIntensity','_setCycleDuration','_malloc','_free']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
    )

else()
    message(STATUS "Building native C++")

    add_library(OpenMindCore STATIC
        openmind_engine.cpp
        VoxelOctree.cpp
        PhysicsEngine.cpp
    )

    target_include_directories(OpenMindCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(OpenMind main.cpp)
    target_link_libraries(OpenMind PRIVATE OpenMindCore)

    add_executable(OpenMindPhysics main_physics.cpp)
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
