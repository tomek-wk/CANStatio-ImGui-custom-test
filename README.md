# CANStatio ImGui Custom Chart Test

Standalone experiment for evaluating Dear ImGui with a custom chart implementation as the chart frontend for CANstatio LogViewer.

The repository is intentionally separate from the main CANStatio project so the experiment can freely change chart architecture, rendering, interaction rules and test strategy without affecting the production LogViewer.

## AI / project handoff

Read in this order when resuming work:

1. `AGENTS.md` — working rules, repository limits and source-of-truth hierarchy;
2. `docs/PROJECT_SPEC.md` — agreed custom-chart specification;
3. `docs/DECISIONS.md` — current design decisions;
4. `docs/PROGRESS.md` — exact implementation and verification state;
5. `docs/AUTOMATED_TESTS.md` — automated test strategy and tools;
6. relevant source files.

## Scope

This project tests the chart itself and chart-adjacent UI only. It does not include file parsing, project loading, DataCore, CSV import or other production LogViewer application features.

The target experiment covers deterministic generated datasets, shared regular time X, independent Y state per series, custom plot layout/axes/grid/transforms, custom navigation, Fit X/Y, active-series selection and highlighting, Custom Legend, Values, crosshair, A/B cursors, numbered markers and later raw-rendering performance tests.

Overview/minimap remains out of scope for the first experiment.

## Architecture target

Dear ImGui remains the GUI framework. The chart itself is application-owned.

The application owns plot rectangle and layout, data/screen transforms, axes/ticks/grid, series rendering and clipping, hit-testing, input routing, pan/zoom and chart overlays. The target architecture does not depend on an external plotting library.

## Current status

The repository was seeded from an already working chart prototype so that data models, UI concepts, interaction semantics and tests could be reused as reference material.

On 2026-09-23 the authoritative documentation was reset for the custom-chart variant.

### C1 — minimal custom plot

Implemented on `main`:

- removed the old plotting dependency from CMake;
- renamed the project/GUI target to `CANStatioImGuiCustomTest` / `canstatio_imgui_custom_test`;
- removed creation/destruction of the old plotting context;
- replaced `Chart` with an application-owned plot rectangle;
- added direct time/value -> screen-space transforms;
- render visible series directly through Dear ImGui `ImDrawList` with clipping;
- keep active-series Halo/Outline using the same screen-space geometry as the data line;
- calculate mouse time directly from the custom plot rectangle;
- keep Values and crosshair connected to the new mouse-time state;
- removed the old renderer-specific cursor overlay;
- replaced the old GUI regression set with a minimal C1 smoke suite.

C1 has **not yet been verified on the target Windows PC**. At the time of implementation the Test Agent heartbeat was stale and its documented local project registry did not yet include this repository. Historical test results from the copied baseline therefore do not count as C1 verification.

Next functional stage: **C2 — transforms, time axis, grid and app-owned X navigation**.

## Stack

- C++20
- Windows first
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- GoogleTest + CTest for functional tests
- Dear ImGui Test Engine for GUI integration tests
- GitHub Actions Windows only as a manual emergency fallback when the Test Agent/target PC is unavailable

Pinned baseline:

- Dear ImGui `v1.92.9b`
- GLFW `3.5.1`
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

Dear ImGui, GLFW and Test Engine are fetched by CMake using pinned references. GoogleTest is found as a local/system package with `find_package(GTest REQUIRED)`.

## Build and run

```text
cmake -S . -B build -G Ninja
cmake --build build
```

Windows executable:

```text
build/canstatio_imgui_custom_test.exe
```

Functional tests:

```text
ctest --test-dir build --output-on-failure
```

GUI integration tests:

```text
build/canstatio_imgui_custom_test.exe --run-tests
```

JUnit reports:

```text
build/test-results/functional-tests.xml
build/test-results/imgui-tests.xml
```

## Verification baseline

The copied pre-migration baseline had previously passed:

```text
Test Agent Windows: 21/21 functional + 19/19 GUI
```

Those results are historical only. After each custom-renderer migration stage, affected functionality must be revalidated.

## Testing policy

Normal validation uses the target Windows PC through CANStatio Test Agent:

```text
configure/build
-> functional
-> GUI
-> manual UX when relevant
```

`.github/workflows/windows-ci.yml` remains only a manual fallback when the target PC/Test Agent is unavailable. Linux GitHub Actions is not part of the project.

## Repository discipline

Normal development proceeds on `main` unless a change materially benefits from isolation.

This conversation and project work are scoped to `CANStatio-ImGui-custom-test`. Other repositories must not be modified, written to or cleaned up. The Test Agent repository may only be used as a communication path to the test environment.

After meaningful implementation or verification work, update `docs/PROGRESS.md`. Record important design decisions in `docs/DECISIONS.md` and requirement changes in `docs/PROJECT_SPEC.md`.