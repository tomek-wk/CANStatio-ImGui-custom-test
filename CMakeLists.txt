cmake_minimum_required(VERSION 3.25)

project(CANStatioImPlotTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.5.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9b
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v1.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

# Pinned to the last Test Engine line before the 1.93 compatibility changes.
# Keep this commit coupled to the pinned Dear ImGui version and update them together.
FetchContent_Declare(
  imgui_test_engine_source
  GIT_REPOSITORY https://github.com/ocornut/imgui_test_engine.git
  GIT_TAG 508a8fc8dacac2f346d353fed31b9bc90ed29adc
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(glfw imgui implot imgui_test_engine_source)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

add_library(imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(imgui PUBLIC
  "${imgui_SOURCE_DIR}"
  "${imgui_SOURCE_DIR}/backends"
)

target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_TEST_ENGINE)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_library(implot STATIC
  "${implot_SOURCE_DIR}/implot.cpp"
  "${implot_SOURCE_DIR}/implot_items.cpp"
)

target_include_directories(implot PUBLIC "${implot_SOURCE_DIR}")
target_link_libraries(implot PUBLIC imgui)

add_library(imgui_test_engine STATIC
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_capture_tool.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_context.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_coroutine.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_engine.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_exporters.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_perftool.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_ui.cpp"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine/imgui_te_utils.cpp"
)

target_include_directories(imgui_test_engine PUBLIC
  "${imgui_test_engine_source_SOURCE_DIR}"
  "${imgui_test_engine_source_SOURCE_DIR}/imgui_test_engine"
)

target_compile_definitions(imgui_test_engine PUBLIC IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL=1)
target_link_libraries(imgui_test_engine PUBLIC imgui Threads::Threads)

add_executable(canstatio_implot_test
  src/main.cpp
  src/App.cpp
  src/Chart.cpp
  src/CursorModel.cpp
  src/CursorOverlay.cpp
  src/CustomLegend.cpp
  src/DataGenerator.cpp
  src/TestControls.cpp
  src/TestEngine.cpp
  src/ValuesModel.cpp
  src/ValuesWindow.cpp
  tests/CursorTests.cpp
  tests/GuiTests.cpp
  tests/NavigationTests.cpp
  tests/SelectionTests.cpp
  tests/ValuesTests.cpp
)

target_include_directories(canstatio_implot_test PRIVATE src tests)
target_link_libraries(canstatio_implot_test PRIVATE implot imgui_test_engine imgui glfw OpenGL::GL)

add_executable(canstatio_functional_tests
  src/CursorModel.cpp
  src/ValuesModel.cpp
  tests/functional/CursorModelTests.cpp
  tests/functional/DatasetTests.cpp
  tests/functional/ValuesModelTests.cpp
)
target_include_directories(canstatio_functional_tests PRIVATE src)
target_link_libraries(canstatio_functional_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(canstatio_functional_tests)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(canstatio_implot_test PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(canstatio_functional_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
