cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)

project(mingw-w64-radar VERSION 3.5.0 LANGUAGES C CXX)

option(ENABLE_STANDALONE "Builds ImGui Radar logic as static library only, disables all GLFW/IMGUI initialization code. Useful for integrating this radar library into third party or private projects" OFF)
option(BUILD_DPP_KMEM_EXAMPLE "Builds example which requires build and installed mingw-w64-dpp and mingw-w64-kmem" OFF)
if(BUILD_DPP_KMEM_EXAMPLE)
    if(NOT CMAKE_GENERATOR MATCHES "Makefiles")
        message(FATAL_ERROR "Only CMAKE_GENERATOR Makefiles support if building DPP/KMEM example")
    endif()

    set(DPP_ROOT "" CACHE PATH "Path to mingw-w64-dpp repository root")
    set(KMEM_ROOT "" CACHE PATH "Path to mingw-w64-kmem repository root")

    if(NOT EXISTS "${DPP_ROOT}/Makefile.inc")
        message(FATAL_ERROR "DPP_ROOT ${DPP_ROOT}/Makefile.inc does not exist!")
    endif()
    if(NOT EXISTS "${KMEM_ROOT}/Makefile.inc")
        message(FATAL_ERROR "KMEM_ROOT ${KMEM_ROOT}/Makefile.inc does not exist!")
    endif()

    message(STATUS "DPP_ROOT: ${DPP_ROOT}")
    message(STATUS "KMEM_ROOT: ${KMEM_ROOT}")
endif()

set(GLFW_INSTALL OFF CACHE BOOL "Disable installation of GLFW" FORCE)
set(IMGUI_SRCDIR "${PROJECT_SOURCE_DIR}/deps/imgui")
set(IMGUI_DEFS -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS=1
               -DIMGUI_DISABLE_FILE_FUNCTIONS=1
               -DIMGUI_DISABLE_DEMO_WINDOWS=1
               -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
               -DIMGUI_DISABLE_DEBUG_TOOLS=1)
add_definitions(${IMGUI_DEFS})

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
    target_compile_options(radar PUBLIC -mwindows -static-libgcc -static-libstdc++ -fomit-frame-pointer -ffunction-sections -fdata-sections -fvisibility=hidden -Os)
    target_link_options(radar PUBLIC -static-libgcc -static-libstdc++ -s)
    target_link_libraries(radar opengl32)
else()
    target_compile_options(radar PUBLIC -static-libgcc -static-libstdc++ -fomit-frame-pointer -ffunction-sections -fdata-sections -fvisibility=hidden -Os)
    target_link_options(radar PUBLIC -static-libgcc -static-libstdc++ -s -flto)
    target_link_libraries(radar GL)
endif()

if(NOT ENABLE_STANDALONE)
    add_executable(radar_test test.cpp)
    target_include_directories(radar_test PRIVATE
                               "${IMGUI_SRCDIR}" "${IMGUI_SRCDIR}/backends")
    target_link_libraries(radar_test radar)

    add_custom_target(run-test)
    add_custom_target(run-test-ext)
    if(MINGW)
        add_custom_command(
            TARGET run-test
            POST_BUILD
            COMMAND wine ${CMAKE_BINARY_DIR}/radar_test.exe
        )
        add_custom_command(
            TARGET run-test-ext
            POST_BUILD
            COMMAND wine ${CMAKE_BINARY_DIR}/radar_test.exe extended-tests
        )
    else()
        add_custom_command(
            TARGET run-test
            POST_BUILD
            COMMAND ${CMAKE_BINARY_DIR}/radar_test
        )
        add_custom_command(
            TARGET run-test-ext
            POST_BUILD
            COMMAND ${CMAKE_BINARY_DIR}/radar_test extended-tests
        )
    endif()
    add_dependencies(run-test radar_test)
    add_dependencies(run-test-ext radar_test)
endif()

if(BUILD_DPP_KMEM_EXAMPLE)
    include(ExternalProject)

    message(STATUS "Make command: ${CMAKE_MAKE_PROGRAM}")

    ExternalProject_Add(radar-example
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/examples
        BINARY_DIR ${CMAKE_BINARY_DIR}/radar-example-build
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E env --
        ${CMAKE_MAKE_PROGRAM}
            -C${CMAKE_SOURCE_DIR}/examples
            -j${CMAKE_BUILD_PARALLEL_LEVEL}
            DPP_ROOT=${DPP_ROOT}
            KMEM_ROOT=${KMEM_ROOT}
            DEBUG=
            Q=
            BUILD_DIR=${CMAKE_BINARY_DIR}/radar-example-build
            RADAR_BUILD_DIR=${CMAKE_BINARY_DIR}
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env --
        ${CMAKE_MAKE_PROGRAM}
            -C${CMAKE_SOURCE_DIR}/examples
            -j${CMAKE_BUILD_PARALLEL_LEVEL}
            DPP_ROOT=${DPP_ROOT}
            KMEM_ROOT=${KMEM_ROOT}
            DEBUG=
            Q=
            BUILD_DIR=${CMAKE_BINARY_DIR}/radar-example-build
            DESTDIR=${CMAKE_BINARY_DIR}/radar-example-install
            RADAR_BUILD_DIR=${CMAKE_BINARY_DIR} install
        BUILD_ALWAYS TRUE
        BUILD_IN_SOURCE FALSE
    )

    add_dependencies(radar-example radar)

    install(
        FILES
        ${CMAKE_BINARY_DIR}/radar_test.exe
        ${CMAKE_BINARY_DIR}/radar-example-install/radar.exe
        ${CMAKE_BINARY_DIR}/radar-example-install/fake-game.exe
        ${CMAKE_BINARY_DIR}/radar-example-install/radar-driver.bat
        ${CMAKE_BINARY_DIR}/radar-example-install/radar-driver.sys
        ${CMAKE_BINARY_DIR}/radar-example-install/radar-driver-sign-driver-on-windows.bat
        ${CMAKE_BINARY_DIR}/radar-example-install/create-codesign-ca-on-windows.bat
        ${CMAKE_BINARY_DIR}/radar-example-install/codesign-ca-cert.crt
        DESTINATION /
    )
endif()
