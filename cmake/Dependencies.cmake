include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(glm URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(imgui_source URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(glfw glm imgui_source)
add_library(imgui STATIC
    ${imgui_source_SOURCE_DIR}/imgui.cpp
    ${imgui_source_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_source_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_source_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_source_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_source_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_source_SOURCE_DIR} ${imgui_source_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)
