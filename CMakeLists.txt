cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)

project(mingw-w64-radar VERSION 3.5.0 LANGUAGES C CXX)

option(ENABLE_STANDALONE "Builds ImGui Radar logic as static library only, disables all GLFW/IMGUI initialization code. Useful for integrating this radar library into third party or private projects" OFF)

set(IMGUI_SRCDIR "${PROJECT_SOURCE_DIR}/deps/imgui")

if(NOT ENABLE_STANDALONE)
    set(GLFW_SRCDIR "${PROJECT_SOURCE_DIR}/deps/glfw")
    add_subdirectory(${GLFW_SRCDIR})

    file(GLOB IMGUI_MAIN_SRC_FILES "${IMGUI_SRCDIR}/*.cpp"
         "${glfw_SRCS}")
    add_library(imgui STATIC "${IMGUI_MAIN_SRC_FILES}"
                             "${IMGUI_SRCDIR}/backends/imgui_impl_opengl3.cpp"
                             "${IMGUI_SRCDIR}//backends/imgui_impl_glfw.cpp")
    target_include_directories(imgui PRIVATE "${GLFW_SRCDIR}/include")
    target_include_directories(imgui PUBLIC "${IMGUI_SRCDIR}" "${IMGUI_SRCDIR}/backends")
else()
    set(IMGUI_INCLUDE_DIR "${IMGUI_SRCDIR}")
endif()

add_library(radar STATIC radar.cpp)
if(ENABLE_STANDALONE)
    target_compile_definitions(radar PRIVATE -DENABLE_STANDALONE=1)
    target_include_directories(radar PRIVATE ${IMGUI_INCLUDE_DIR})
endif()
target_include_directories(radar PUBLIC ${CMAKE_SOURCE_DIR})
if(NOT ENABLE_STANDALONE)
    target_link_libraries(radar glfw imgui)
endif()

if(MINGW)
    message(STATUS "Build for Windows enabled: ${MINGW}")

    target_compile_options(radar PUBLIC -mwindows -static-libgcc -static-libstdc++ -fomit-frame-pointer -Os)
    target_compile_definitions(radar PUBLIC -DSTATIC_IMGUI=1 -DGLFW_BUILD_WIN32=1)
    target_link_options(radar PUBLIC -static-libgcc -static-libstdc++)
    target_link_libraries(radar opengl32)
else()
    target_link_libraries(radar GL)
endif()

if(NOT ENABLE_STANDALONE)
    add_executable(radar_test test.cpp)
    target_link_libraries(radar_test radar)

    add_custom_target(run-test)
    if(MINGW)
        add_custom_command(
            TARGET run-test
            POST_BUILD
            COMMAND wine ${CMAKE_BINARY_DIR}/radar_test.exe
        )
    else()
        add_custom_command(
            TARGET run-test
            POST_BUILD
            COMMAND ${CMAKE_BINARY_DIR}/radar_test
        )
    endif()
    add_dependencies(run-test radar_test)
endif()
