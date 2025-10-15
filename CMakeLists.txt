cmake_minimum_required(VERSION 3.20)
project(FabricBinarySearch)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_GUI "Build with GUI support" ON)

find_package(ZLIB REQUIRED)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

set(CORE_SOURCES
    src/core/ModManager.cpp
    src/core/BinarySearchEngine.cpp
)

set(UTILS_SOURCES
    src/utils/ModInfo.cpp
    src/utils/JarReader.cpp
    src/utils/CrashLogParser.cpp
    src/utils/MinecraftLauncher.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
    src/utils/ProgressState.cpp
)

set(SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
)

if(BUILD_GUI)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.4
    )
    FetchContent_MakeAvailable(imgui)

    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    list(APPEND SOURCES ${IMGUI_SOURCES} src/gui/GuiApp.cpp)

    add_compile_definitions(BUILD_GUI)
endif()

add_executable(FabricBinarySearch ${SOURCES})

target_include_directories(FabricBinarySearch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
)

target_link_libraries(FabricBinarySearch PRIVATE
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
)

if(BUILD_GUI)
    target_include_directories(FabricBinarySearch PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(FabricBinarySearch PRIVATE glfw)

    if(APPLE)
        target_link_libraries(FabricBinarySearch PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(FabricBinarySearch PRIVATE OpenGL::GL)
    elseif(WIN32)
        target_link_libraries(FabricBinarySearch PRIVATE opengl32)
    endif()
endif()

# Installation targets
install(TARGETS FabricBinarySearch
    RUNTIME DESTINATION bin
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "FabricBinarySearch")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "FabricBinarySearch")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fabric Mod Binary Search Tool")
set(CPACK_PACKAGE_EXECUTABLES "FabricBinarySearch" "Fabric Binary Search")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FabricBinarySearch")
endif()

include(CPack)