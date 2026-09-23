#include "App.h"

#include <cstdio>
#include <filesystem>
#include <string_view>
#include <utility>

namespace {
void printUsage(const char* executable) {
    std::printf("Usage: %s [--run-tests [filter]] [--show-test-ui]\n", executable);
}
} // namespace

int main(int argc, char** argv) {
    AppOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--run-tests") {
            options.runTests = true;
            if (i + 1 < argc) {
                const std::string_view next(argv[i + 1]);
                if (!next.starts_with("--")) {
                    options.testFilter = argv[++i];
                }
            }
        } else if (argument == "--show-test-ui") {
            options.showTestUi = true;
        } else if (argument == "--help" || argument == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            printUsage(argv[0]);
            return 2;
        }
    }

    const std::filesystem::path executablePath = std::filesystem::absolute(argv[0]);
    options.testResultsPath =
        (executablePath.parent_path() / "test-results" / "imgui-tests.xml").string();

    App app(std::move(options));
    return app.run();
}
