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

The target experiment covers:

- deterministic generated datasets;
- shared regular time X;
- independent Y state per series;
- custom plot layout, axes, grid and coordinate transforms;
- custom X and Y navigation;
- Fit X / Fit Y;
- active-series selection and highlighting;
- custom legend;
- Values under the mouse and time crosshair;
- A/B time cursors;
- numbered draggable time markers;
- functional and GUI integration tests;
- later raw-rendering performance tests on larger datasets.

Overview/minimap remains out of scope for the first experiment.

## Architecture target

Dear ImGui remains the GUI framework. The chart itself is application-owned.

The application owns:

- plot rectangle and layout;
- data-to-screen and screen-to-data transforms;
- time and value axes;
- tick generation and formatting;
- grid rendering;
- series rendering and clipping;
- hit-testing;
- mouse input routing;
- pan and zoom;
- active-series highlight;
- crosshair, cursors and markers.

The target architecture does not depend on an external plotting library.

## Current status

The repository was seeded from an already working chart prototype so that data models, UI concepts, interaction semantics and tests could be reused as reference material.

On 2026-09-23 the authoritative documentation was reset for the custom-chart variant. The copied implementation is now considered a temporary legacy baseline: it demonstrates previously accepted behavior, but its renderer-specific architecture is not a requirement for the new implementation.

The custom chart renderer has not yet been implemented.

The next implementation step is to replace the legacy chart backend with the smallest application-owned plot surface and direct series rendering while keeping the reusable data/state models where they still make sense.

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

Dear ImGui, GLFW and Test Engine are fetched by CMake using pinned references. GoogleTest is found as a local/system package with `find_package(GTest REQUIRED)` and is not fetched by this repository.

## Verification status

The copied baseline had previously passed:

```text
Test Agent Windows: 21/21 functional + 19/19 GUI
```

Those results are retained only as a historical baseline for behavior that existed before the custom-renderer migration. They do not validate the new renderer.

After each migration stage, affected functional and GUI behavior must be revalidated on the target Windows PC through CANStatio Test Agent.

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