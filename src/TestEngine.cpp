#include "TestEngine.h"

#include <cstdio>
#include <filesystem>
#include <system_error>

#include <imgui.h>
#include <imgui_test_engine/imgui_te_context.h>
#include <imgui_test_engine/imgui_te_engine.h>
#include <imgui_test_engine/imgui_te_exporters.h>
#include <imgui_test_engine/imgui_te_ui.h>

void RegisterGuiTests(ImGuiTestEngine* engine, const GuiTestAccess& access);

bool TestEngine::initialize(bool runTests,
                            bool showUi,
                            const std::string& filter,
                            const std::string& resultsPath,
                            const GuiTestAccess& access) {
    runTests_ = runTests;
    showUi_ = showUi;

    if (!runTests_ && !showUi_) {
        return true;
    }

    engine_ = ImGuiTestEngine_CreateContext();
    if (engine_ == nullptr) {
        std::fprintf(stderr, "Failed to create Dear ImGui Test Engine context.\n");
        return false;
    }

    ImGuiTestEngineIO& testIo = ImGuiTestEngine_GetIO(engine_);
    testIo.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    testIo.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    testIo.ConfigLogToTTY = runTests_;
    testIo.ConfigRunSpeed = ImGuiTestRunSpeed_Fast;
    testIo.ConfigCaptureEnabled = false;
    testIo.ConfigNoThrottle = runTests_;

    if (runTests_) {
        resultsPath_ = resultsPath;
        const std::filesystem::path outputPath(resultsPath_);
        const std::filesystem::path outputDir = outputPath.parent_path();
        if (!outputDir.empty()) {
            std::error_code error;
            std::filesystem::create_directories(outputDir, error);
            if (error) {
                std::fprintf(stderr,
                             "Failed to create test results directory '%s': %s\n",
                             outputDir.string().c_str(),
                             error.message().c_str());
                return false;
            }
        }

        testIo.ExportResultsFilename = resultsPath_.c_str();
        testIo.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;
    }

    ImGuiTestEngine_Start(engine_, ImGui::GetCurrentContext());
    started_ = true;
    ImGuiTestEngine_InstallDefaultCrashHandler();

    RegisterGuiTests(engine_, access);

    if (runTests_) {
        const char* filterText = filter.empty() ? nullptr : filter.c_str();
        ImGuiTestEngine_QueueTests(engine_,
                                   ImGuiTestGroup_Tests,
                                   filterText,
                                   ImGuiTestRunFlags_RunFromCommandLine);
        testsQueued_ = true;
    }

    return true;
}

void TestEngine::stop() {
    if (engine_ != nullptr && started_) {
        ImGuiTestEngine_Stop(engine_);
        started_ = false;
    }
}

void TestEngine::destroy() {
    if (engine_ != nullptr) {
        ImGuiTestEngine_DestroyContext(engine_);
        engine_ = nullptr;
    }
}

void TestEngine::drawUi() {
    if (engine_ != nullptr && showUi_) {
        ImGuiTestEngine_ShowTestEngineWindows(engine_, nullptr);
    }
}

void TestEngine::preSwap() {
    if (engine_ != nullptr && started_) {
        ImGuiTestEngine_PreSwap(engine_);
    }
}

void TestEngine::postSwap() {
    if (engine_ != nullptr && started_) {
        ImGuiTestEngine_PostSwap(engine_);
    }
}

bool TestEngine::testsComplete() const {
    return runTests_ && testsQueued_ && engine_ != nullptr && ImGuiTestEngine_IsTestQueueEmpty(engine_);
}

bool TestEngine::shouldExitAfterTests() const {
    return testsComplete();
}

int TestEngine::resultCode() const {
    if (!runTests_ || engine_ == nullptr) {
        return 0;
    }

    ImGuiTestEngineResultSummary summary;
    ImGuiTestEngine_GetResultSummary(engine_, &summary);

    if (summary.CountTested == 0) {
        return 2;
    }
    if (summary.CountInQueue != 0 || summary.CountSuccess != summary.CountTested) {
        return 1;
    }
    return 0;
}

void TestEngine::printResultSummary() const {
    if (engine_ != nullptr) {
        ImGuiTestEngine_PrintResultSummary(engine_);
    }
}
