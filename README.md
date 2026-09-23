# CANStatio ImPlot Test

Standalone experiment for evaluating Dear ImGui + ImPlot as the chart frontend for CANstatio LogViewer.

The repository is intentionally separate from the main CANStatio project so the experiment can freely change its structure, dependencies, interaction model and rendering approach without affecting the production LogViewer.

## AI / project handoff

Read in this order when resuming work:

1. `AGENTS.md` — working rules, limits and source-of-truth hierarchy;
2. `docs/PROJECT_SPEC.md` — agreed experiment specification;
3. `docs/DECISIONS.md` — important decisions and replacements;
4. `docs/PROGRESS.md` — exact implementation/verification state and next step;
5. `docs/AUTOMATED_TESTS.md` — automated test strategy and tools;
6. relevant source files.

## Scope

This project tests the chart itself and chart-adjacent UI only. It does not include file parsing, project loading, DataCore, CSV import or other production LogViewer application features.

The experiment covers deterministic generated datasets, shared regular time X, independent Y state per series, active-series semantic Y/grid, X and Y navigation, Fit X/Y, series selection, native/custom legends, Values, crosshair, Halo/Outline active highlighting, A/B time cursors and planned numbered draggable markers.

Overview/minimap remains out of scope for the first experiment.

## Current status

The automated-testing experiment was completed on `feature/imgui-test-engine` and integrated into `main` through PR #1 on 2026-09-23. Normal application development now continues linearly on `main`; separate branches are reserved for changes large enough to justify isolation, such as major architecture, renderer, data-model or tooling experiments.

Stages 1–5 and Stage 6A A/B cursors are implemented. Current cursor input:

- `Shift + LMB` sets or moves A;
- `Ctrl + LMB` sets or moves B;
- plain LMB drag moves an existing cursor;
- `Ctrl + RMB` hides the cursor under the mouse;
- `Alt + drag` is the normal active-Y pan gesture and has no cursor-specific behavior;
- cursor time is clamped to the dataset and remains time-based across X navigation.

Two older visual/layout issues were also resolved before Stage 6B:

- semantic Y uses a permanently reserved left gutter, so activating/deactivating or switching active series no longer changes plot-area geometry;
- Halo/Outline is rendered as one screen-space `ImDrawList::AddPolyline` overlay instead of a second thick ImPlot line, improving joins at bends while keeping the 1.5 px data line unchanged.

Current verified regression on the target Windows PC:

```text
Test Agent Windows: 21/21 functional + 19/19 GUI
```

The Y-gutter and Halo/Outline changes were also accepted in manual visual testing.

The next functional work is Stage 6B — numbered markers plus the remaining input-priority rules, especially `Alt-click` versus `Alt-drag`.

## Stack

- C++20
- Windows first
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile
- Dear ImGui
- ImPlot
- GoogleTest + CTest for functional tests
- Dear ImGui Test Engine for GUI integration tests
- GitHub Actions Windows only as a manual emergency fallback when the Test Agent/target PC is unavailable

Pinned baseline:

- Dear ImGui `v1.92.9b`
- ImPlot `v1.0`
- GLFW `3.5.1`
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

Dear ImGui, ImPlot, GLFW and Test Engine are fetched by CMake using pinned references. GoogleTest is found as a local/system package with `find_package(GTest REQUIRED)` and is not fetched by this repository.

## Repository layout

```text
.github/
  workflows/
    windows-ci.yml
AGENTS.md
CMakeLists.txt
README.md
run.bat
update.bat
docs/
  PROJECT_SPEC.md
  DECISIONS.md
  PROGRESS.md
  AUTOMATED_TESTS.md
src/
  main.cpp
  App.*
  Chart.*
  CursorModel.*
  CursorOverlay.*
  CustomLegend.*
  DataGenerator.*
  Dataset.h
  SeriesStyle.h
  TestControls.*
  TestEngine.*
  ValuesModel.*
  ValuesWindow.*
tests/
  CursorTests.cpp
  GuiTests.cpp
  NavigationTests.cpp
  SelectionTests.cpp
  ValuesTests.cpp
  functional/
    CursorModelTests.cpp
    DatasetTests.cpp
    ValuesModelTests.cpp
```

## Build and run

```text
cmake -S . -B build -G Ninja
cmake --build build
```

Windows executable:

```text
build/canstatio_implot_test.exe
```

Functional tests:

```text
ctest --test-dir build --output-on-failure
```

GUI integration tests on the target Windows PC:

```text
build/canstatio_implot_test.exe --run-tests
```

JUnit reports:

```text
build/test-results/functional-tests.xml
build/test-results/imgui-tests.xml
```

## Testing policy

Normal validation uses the target Windows PC through CANStatio Test Agent:

```text
configure/build
-> functional
-> GUI
-> manual UX when relevant
```

`.github/workflows/windows-ci.yml` is retained only as a manual fallback when the target PC/Test Agent is unavailable. It is intentionally `workflow_dispatch` only. Linux GitHub Actions is not part of the project.

Hosted Windows GUI execution is not used because the standard runner did not provide a usable WGL/OpenGL context during the experiment.

## CANStatio Test Agent

The separate repository `tomek-wk/CANStatio-Test-Agent` controls the Windows development machine through bounded named capabilities. Current verified agent line is 0.7. It supports project operations such as `checkout`, `pull`, `configure`, `build`, `update`, `run`, `status`, `stop` and `test`; it does not expose a general remote shell.

## Project discipline

- normal development proceeds on `main` unless a change materially benefits from an isolated branch;
- use public ImPlot/ImGui APIs unless an explicit decision changes that;
- do not add parser/DataCore/main-CANstatio integration without a separate decision;
- do not refactor production code merely to mimic the test architecture of another project;
- use the target Windows/Test Agent path whenever it is available;
- use GitHub-hosted Windows only when the Test Agent/target PC is unavailable;
- update `docs/PROGRESS.md` after meaningful implementation or verification work;
- record important design decisions in `docs/DECISIONS.md` and, when they alter requirements, `docs/PROJECT_SPEC.md`.
