#include "App.h"

#include "Chart.h"
#include "CustomLegend.h"
#include "DataGenerator.h"
#include "TestControls.h"
#include "ValuesWindow.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace {
constexpr const char* kGlslVersion = "#version 330";
constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 800;

void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

const char* glString(GLenum name) {
    const GLubyte* value = glGetString(name);
    return value != nullptr ? reinterpret_cast<const char*>(value) : "<unavailable>";
}

void logOpenGlInfo() {
    std::fprintf(stderr, "OpenGL vendor: %s\n", glString(GL_VENDOR));
    std::fprintf(stderr, "OpenGL renderer: %s\n", glString(GL_RENDERER));
    std::fprintf(stderr, "OpenGL version: %s\n", glString(GL_VERSION));
}
} // namespace

App::App(AppOptions options)
    : options_(std::move(options)) {}

int App::run() {
    if (!initialize()) {
        shutdown();
        return 1;
    }

    int exitCode = 0;
    while (!glfwWindowShouldClose(window_)) {
        frame();
        if (testEngine_.shouldExitAfterTests()) {
            testEngine_.printResultSummary();
            exitCode = testEngine_.resultCode();
            break;
        }
    }

    if (options_.runTests && !testEngine_.testsComplete()) {
        exitCode = 3;
    }

    shutdown();
    return exitCode;
}

bool App::initialize() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(kInitialWidth,
                               kInitialHeight,
                               "CANStatio ImGui Custom Chart Test",
                               nullptr,
                               nullptr);
    if (window_ == nullptr) {
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(options_.runTests ? 0 : 1);
    logOpenGlInfo();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // The texture-based AA path made the transient Windows/OpenGL artifacts
    // substantially more visible. Keep line AA enabled, but use geometric AA.
    ImGui::GetStyle().AntiAliasedLinesUseTex = false;

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        return false;
    }

    dataset_ = DataGenerator::make(dataPreset_);
    xMin_ = 0.0;
    xMax_ = dataset_.endTimeSeconds();
    if (!(xMax_ > xMin_)) {
        xMax_ = xMin_ + std::max(dataset_.dtSeconds, 1.0e-6);
    }

    const GuiTestAccess testAccess{
        .dataset = &dataset_,
        .activeSeriesIndex = &activeSeriesIndex_,
        .activeHighlightMode = &activeHighlightMode_,
        .xMin = &xMin_,
        .xMax = &xMax_,
        .showCustomLegend = &showCustomLegend_,
        .showValues = &showValues_,
        .showCrosshair = &showCrosshair_,
        .chartMouseState = &chartMouseState_,
        .chartLayoutState = &chartLayoutState_,
        .cursorModel = &cursorModel_,
    };

    if (!testEngine_.initialize(options_.runTests,
                                options_.showTestUi,
                                options_.testFilter,
                                options_.testResultsPath,
                                testAccess)) {
        return false;
    }

    return true;
}

void App::shutdown() {
    testEngine_.stop();

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    testEngine_.destroy();

    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

void App::saveWindowedGeometry() {
    if (glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) == GLFW_TRUE) {
        glfwRestoreWindow(window_);
    }
    glfwGetWindowPos(window_, &windowedX_, &windowedY_);
    glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
}

void App::restoreWindowedGeometry() {
    glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_TRUE);
    glfwSetWindowMonitor(window_,
                         nullptr,
                         windowedX_,
                         windowedY_,
                         std::max(windowedWidth_, 640),
                         std::max(windowedHeight_, 480),
                         GLFW_DONT_CARE);
}

void App::handleWindowModeToggles() {
    const bool fullscreenKeyDown = glfwGetKey(window_, GLFW_KEY_F11) == GLFW_PRESS;
    if (fullscreenKeyDown && !fullscreenToggleKeyDown_) {
        toggleFullscreen();
    }
    fullscreenToggleKeyDown_ = fullscreenKeyDown;

    const bool borderlessKeyDown = glfwGetKey(window_, GLFW_KEY_F10) == GLFW_PRESS;
    if (borderlessKeyDown && !borderlessToggleKeyDown_) {
        toggleBorderless();
    }
    borderlessToggleKeyDown_ = borderlessKeyDown;
}

void App::toggleFullscreen() {
    if (!fullscreen_) {
        if (borderless_) {
            restoreWindowedGeometry();
            borderless_ = false;
        } else {
            saveWindowedGeometry();
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
        if (monitor == nullptr || mode == nullptr) {
            return;
        }

        glfwSetWindowMonitor(window_,
                             monitor,
                             0,
                             0,
                             mode->width,
                             mode->height,
                             mode->refreshRate);
        fullscreen_ = true;
        std::fprintf(stderr,
                     "Exclusive fullscreen: ON (%dx%d @ %d Hz)\n",
                     mode->width,
                     mode->height,
                     mode->refreshRate);
        return;
    }

    restoreWindowedGeometry();
    fullscreen_ = false;
    std::fprintf(stderr,
                 "Exclusive fullscreen: OFF (%dx%d at %d,%d)\n",
                 windowedWidth_,
                 windowedHeight_,
                 windowedX_,
                 windowedY_);
}

void App::toggleBorderless() {
    if (fullscreen_) {
        std::fprintf(stderr, "Borderless toggle ignored while exclusive fullscreen is active.\n");
        return;
    }

    if (!borderless_) {
        saveWindowedGeometry();

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
        if (monitor == nullptr || mode == nullptr) {
            return;
        }

        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowPos(window_, monitorX, monitorY);
        glfwSetWindowSize(window_, mode->width, mode->height);
        borderless_ = true;
        std::fprintf(stderr,
                     "Borderless fullscreen: ON (%dx%d at %d,%d)\n",
                     mode->width,
                     mode->height,
                     monitorX,
                     monitorY);
        return;
    }

    restoreWindowedGeometry();
    borderless_ = false;
    std::fprintf(stderr,
                 "Borderless fullscreen: OFF (%dx%d at %d,%d)\n",
                 windowedWidth_,
                 windowedHeight_,
                 windowedX_,
                 windowedY_);
}

void App::frame() {
    glfwPollEvents();
    handleWindowModeToggles();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    Chart::draw(dataset_,
                activeSeriesIndex_,
                activeHighlightMode_,
                xMin_,
                xMax_,
                showCrosshair_,
                chartMouseState_,
                chartLayoutState_,
                cursorModel_,
                markerModel_,
                chartInteractionState_);

    const bool reset = TestControls::draw(dataset_,
                                          dataPreset_,
                                          activeSeriesIndex_,
                                          activeHighlightMode_,
                                          xMin_,
                                          xMax_,
                                          cursorModel_,
                                          markerModel_,
                                          showCustomLegend_,
                                          showValues_,
                                          showCrosshair_);
    if (reset) {
        chartInteractionState_.reset();
        chartMouseState_ = {};
    }

    CustomLegend::draw(dataset_, activeSeriesIndex_, showCustomLegend_);
    ValuesWindow::draw(dataset_, activeSeriesIndex_, chartMouseState_, showValues_);
    testEngine_.drawUi();

    ImGui::Render();

    int displayWidth = 0;
    int displayHeight = 0;
    glfwGetFramebufferSize(window_, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(0.08F, 0.08F, 0.09F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    testEngine_.preSwap();
    glfwSwapBuffers(window_);
    testEngine_.postSwap();
}
