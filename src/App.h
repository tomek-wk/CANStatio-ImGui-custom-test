#pragma once

#include "Chart.h"
#include "CursorModel.h"
#include "DataGenerator.h"
#include "Dataset.h"
#include "MarkerModel.h"
#include "TestEngine.h"

#include <cstddef>
#include <limits>
#include <string>

struct GLFWwindow;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct IDXGISwapChain;

enum class RenderBackend {
    OpenGL,
    DirectX11,
};

struct AppOptions {
    bool runTests = false;
    bool showTestUi = false;
    std::string testFilter;
    std::string testResultsPath;
    RenderBackend backend = RenderBackend::OpenGL;
};

class App {
public:
    explicit App(AppOptions options = {});
    int run();

private:
    bool initialize();
    void shutdown();
    void frame();
    bool initializeRenderer();
    void shutdownRenderer();
    bool initializeDirectX11();
    void shutdownDirectX11();
    bool createDirectX11RenderTarget();
    void destroyDirectX11RenderTarget();
    void resizeDirectX11IfNeeded();
    void handleWindowModeToggles();
    void toggleFullscreen();
    void saveWindowedGeometry();
    void restoreWindowedGeometry();

    AppOptions options_;
    GLFWwindow* window_ = nullptr;
    ID3D11Device* d3dDevice_ = nullptr;
    ID3D11DeviceContext* d3dContext_ = nullptr;
    IDXGISwapChain* dxgiSwapChain_ = nullptr;
    ID3D11RenderTargetView* d3dRenderTarget_ = nullptr;
    int d3dWidth_ = 0;
    int d3dHeight_ = 0;
    bool fullscreen_ = false;
    bool fullscreenToggleKeyDown_ = false;
    int windowedX_ = 100;
    int windowedY_ = 100;
    int windowedWidth_ = 1280;
    int windowedHeight_ = 800;
    Dataset dataset_;
    DataPreset dataPreset_ = DataPreset::Small;
    std::size_t activeSeriesIndex_ = std::numeric_limits<std::size_t>::max();
    ActiveHighlightMode activeHighlightMode_ = ActiveHighlightMode::Halo;
    double xMin_ = 0.0;
    double xMax_ = 0.0;
    bool showCustomLegend_ = true;
    bool showValues_ = true;
    bool showCrosshair_ = true;
    ChartMouseState chartMouseState_;
    ChartLayoutState chartLayoutState_;
    ChartInteractionState chartInteractionState_;
    CursorModel cursorModel_;
    MarkerModel markerModel_;
    TestEngine testEngine_;
};
