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

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <backends/imgui_impl_dx11.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#endif

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
    std::fprintf(stderr, "Rendering backend: OpenGL\n");
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

    if (options_.backend == RenderBackend::DirectX11) {
#ifndef _WIN32
        std::fprintf(stderr, "DirectX 11 backend is only available on Windows.\n");
        return false;
#else
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
    } else {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    window_ = glfwCreateWindow(kInitialWidth,
                               kInitialHeight,
                               "CANStatio ImGui Custom Chart Test",
                               nullptr,
                               nullptr);
    if (window_ == nullptr) {
        return false;
    }

    if (options_.backend == RenderBackend::OpenGL) {
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(options_.runTests ? 0 : 1);
        logOpenGlInfo();
    } else if (!initializeDirectX11()) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // The texture-based line-AA path made the transient OpenGL artifacts more visible.
    // Keep geometric line AA for both renderers so the chart geometry stays identical.
    ImGui::GetStyle().AntiAliasedLinesUseTex = false;

    if (!initializeRenderer()) {
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

bool App::initializeRenderer() {
    if (options_.backend == RenderBackend::DirectX11) {
#ifdef _WIN32
        if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
            return false;
        }
        if (!ImGui_ImplDX11_Init(d3dDevice_, d3dContext_)) {
            ImGui_ImplGlfw_Shutdown();
            return false;
        }
        return true;
#else
        return false;
#endif
    }

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        return false;
    }
    return true;
}

void App::shutdownRenderer() {
    if (options_.backend == RenderBackend::DirectX11) {
#ifdef _WIN32
        ImGui_ImplDX11_Shutdown();
#endif
    } else {
        ImGui_ImplOpenGL3_Shutdown();
    }
    ImGui_ImplGlfw_Shutdown();
}

bool App::initializeDirectX11() {
#ifndef _WIN32
    return false;
#else
    HWND hwnd = glfwGetWin32Window(window_);
    if (hwnd == nullptr) {
        return false;
    }

    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL selectedFeatureLevel = D3D_FEATURE_LEVEL_10_0;

    HRESULT result = D3D11CreateDevice(nullptr,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       nullptr,
                                       0,
                                       featureLevels,
                                       2,
                                       D3D11_SDK_VERSION,
                                       &d3dDevice_,
                                       &selectedFeatureLevel,
                                       &d3dContext_);
    if (FAILED(result)) {
        std::fprintf(stderr, "D3D11CreateDevice failed: 0x%08lX\n",
                     static_cast<unsigned long>(result));
        return false;
    }

    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* dxgiAdapter = nullptr;
    IDXGIFactory2* dxgiFactory = nullptr;
    IDXGISwapChain1* swapChain1 = nullptr;

    result = d3dDevice_->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (SUCCEEDED(result)) {
        result = dxgiDevice->GetAdapter(&dxgiAdapter);
    }
    if (SUCCEEDED(result)) {
        result = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    }

    if (SUCCEEDED(result)) {
        DXGI_SWAP_CHAIN_DESC1 swapDesc{};
        swapDesc.Width = 0;
        swapDesc.Height = 0;
        swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapDesc.Stereo = FALSE;
        swapDesc.SampleDesc.Count = 1;
        swapDesc.SampleDesc.Quality = 0;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount = 2;
        swapDesc.Scaling = DXGI_SCALING_STRETCH;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapDesc.Flags = 0;

        result = dxgiFactory->CreateSwapChainForHwnd(d3dDevice_,
                                                      hwnd,
                                                      &swapDesc,
                                                      nullptr,
                                                      nullptr,
                                                      &swapChain1);
        if (SUCCEEDED(result)) {
            dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
            dxgiSwapChain_ = swapChain1;
            swapChain1 = nullptr;
        }
    }

    if (swapChain1 != nullptr) {
        swapChain1->Release();
    }
    if (dxgiFactory != nullptr) {
        dxgiFactory->Release();
    }
    if (dxgiAdapter != nullptr) {
        dxgiAdapter->Release();
    }
    if (dxgiDevice != nullptr) {
        dxgiDevice->Release();
    }

    if (FAILED(result) || dxgiSwapChain_ == nullptr) {
        std::fprintf(stderr, "CreateSwapChainForHwnd (FLIP_DISCARD) failed: 0x%08lX\n",
                     static_cast<unsigned long>(result));
        shutdownDirectX11();
        return false;
    }

    if (!createDirectX11RenderTarget()) {
        shutdownDirectX11();
        return false;
    }

    glfwGetFramebufferSize(window_, &d3dWidth_, &d3dHeight_);
    std::fprintf(stderr,
                 "Rendering backend: DirectX 11 / DXGI FLIP_DISCARD (feature level 0x%04X)\n",
                 static_cast<unsigned int>(selectedFeatureLevel));
    return true;
#endif
}

void App::shutdownDirectX11() {
#ifdef _WIN32
    if (dxgiSwapChain_ != nullptr) {
        dxgiSwapChain_->SetFullscreenState(FALSE, nullptr);
    }
    destroyDirectX11RenderTarget();
    if (dxgiSwapChain_ != nullptr) {
        dxgiSwapChain_->Release();
        dxgiSwapChain_ = nullptr;
    }
    if (d3dContext_ != nullptr) {
        d3dContext_->Release();
        d3dContext_ = nullptr;
    }
    if (d3dDevice_ != nullptr) {
        d3dDevice_->Release();
        d3dDevice_ = nullptr;
    }
#endif
}

bool App::createDirectX11RenderTarget() {
#ifndef _WIN32
    return false;
#else
    if (dxgiSwapChain_ == nullptr || d3dDevice_ == nullptr) {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    const HRESULT getBufferResult = dxgiSwapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(getBufferResult) || backBuffer == nullptr) {
        return false;
    }

    const HRESULT viewResult =
        d3dDevice_->CreateRenderTargetView(backBuffer, nullptr, &d3dRenderTarget_);
    backBuffer->Release();
    return SUCCEEDED(viewResult);
#endif
}

void App::destroyDirectX11RenderTarget() {
#ifdef _WIN32
    if (d3dRenderTarget_ != nullptr) {
        d3dRenderTarget_->Release();
        d3dRenderTarget_ = nullptr;
    }
#endif
}

void App::resizeDirectX11IfNeeded() {
#ifdef _WIN32
    if (options_.backend != RenderBackend::DirectX11 || dxgiSwapChain_ == nullptr) {
        return;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width <= 0 || height <= 0 || (width == d3dWidth_ && height == d3dHeight_)) {
        return;
    }

    destroyDirectX11RenderTarget();
    const HRESULT resizeResult =
        dxgiSwapChain_->ResizeBuffers(0,
                                      static_cast<UINT>(width),
                                      static_cast<UINT>(height),
                                      DXGI_FORMAT_UNKNOWN,
                                      0);
    if (FAILED(resizeResult)) {
        std::fprintf(stderr, "DXGI ResizeBuffers failed: 0x%08lX\n",
                     static_cast<unsigned long>(resizeResult));
        return;
    }

    if (createDirectX11RenderTarget()) {
        d3dWidth_ = width;
        d3dHeight_ = height;
    }
#endif
}

void App::shutdown() {
    testEngine_.stop();

    if (ImGui::GetCurrentContext() != nullptr) {
        shutdownRenderer();
        ImGui::DestroyContext();
    }

    testEngine_.destroy();

    if (options_.backend == RenderBackend::DirectX11) {
        shutdownDirectX11();
    }

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
}

void App::toggleFullscreen() {
    if (!fullscreen_) {
        saveWindowedGeometry();

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
#ifdef _WIN32
        if (options_.backend == RenderBackend::DirectX11 && dxgiSwapChain_ != nullptr) {
            const HRESULT result = dxgiSwapChain_->SetFullscreenState(TRUE, nullptr);
            if (FAILED(result)) {
                std::fprintf(stderr, "DXGI exclusive fullscreen failed: 0x%08lX\n",
                             static_cast<unsigned long>(result));
            }
        }
#endif
        fullscreen_ = true;
        std::fprintf(stderr,
                     "Exclusive fullscreen: ON (%dx%d @ %d Hz)\n",
                     mode->width,
                     mode->height,
                     mode->refreshRate);
        return;
    }

#ifdef _WIN32
    if (options_.backend == RenderBackend::DirectX11 && dxgiSwapChain_ != nullptr) {
        dxgiSwapChain_->SetFullscreenState(FALSE, nullptr);
    }
#endif
    restoreWindowedGeometry();
    fullscreen_ = false;
    std::fprintf(stderr,
                 "Exclusive fullscreen: OFF (%dx%d at %d,%d)\n",
                 windowedWidth_,
                 windowedHeight_,
                 windowedX_,
                 windowedY_);
}

void App::frame() {
    glfwPollEvents();
    handleWindowModeToggles();
    resizeDirectX11IfNeeded();

    if (options_.backend == RenderBackend::DirectX11) {
#ifdef _WIN32
        ImGui_ImplDX11_NewFrame();
#endif
    } else {
        ImGui_ImplOpenGL3_NewFrame();
    }
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

    if (options_.backend == RenderBackend::DirectX11) {
#ifdef _WIN32
        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window_, &displayWidth, &displayHeight);
        if (displayWidth > 0 && displayHeight > 0 && d3dRenderTarget_ != nullptr) {
            constexpr float clearColor[4] = {0.08F, 0.08F, 0.09F, 1.0F};
            d3dContext_->OMSetRenderTargets(1, &d3dRenderTarget_, nullptr);
            d3dContext_->ClearRenderTargetView(d3dRenderTarget_, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            testEngine_.preSwap();
            dxgiSwapChain_->Present(options_.runTests ? 0 : 1, 0);
            testEngine_.postSwap();
        }
#endif
        return;
    }

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
