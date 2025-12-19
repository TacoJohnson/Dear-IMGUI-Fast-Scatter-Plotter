cmake_minimum_required(VERSION 3.14)
project(ImGuiExample)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to download GLFW
include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# Configure GLFW options (disable unnecessary features)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Dear ImGui sources
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create executable (WIN32 flag removes console window on Windows)
add_executable(imgui_example
    main.cpp
    ${IMGUI_SOURCES}
)

# LIDAR Point Cloud Viewer executable
add_executable(lidar_viewer
    lidar_example.cpp
    point_cloud_renderer.cpp
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(imgui_example PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
)

target_include_directories(lidar_viewer PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(imgui_example
    glfw
    ${OPENGL_LIBRARIES}
)

target_link_libraries(lidar_viewer
    glfw
    ${OPENGL_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(imgui_example opengl32 glu32)
    target_link_libraries(lidar_viewer opengl32 glu32)
elseif(APPLE)
    target_link_libraries(imgui_example "-framework OpenGL")
endif()
