# CANStatio ImGui Custom Chart Test

Standalone experiment for evaluating Dear ImGui with a fully application-owned chart implementation as the chart frontend for CANstatio LogViewer.

The repository is intentionally separate from the main CANStatio project. The experiment can change chart architecture, rendering, interaction rules and test strategy without affecting any other repository.

## AI / project handoff

Read in this order when resuming work:

1. `AGENTS.md` — working rules and repository boundary;
2. `docs/PROJECT_SPEC.md` — authoritative custom-chart specification;
3. `docs/DECISIONS.md` — design decisions;
4. `docs/PROGRESS.md` — implementation and verification state;
5. `docs/AUTOMATED_TESTS.md` — test strategy;
6. relevant source files.

## Scope

The experiment covers the chart and chart-adjacent UI only. It does not include file parsing, CSV import, DataCore, LogViewer project loading or integration with the main CANStatio repository.

Implemented experiment scope:

- deterministic generated datasets and all planned presets;
- shared regular time X;
- independent `viewMin/viewMax` per series;
- application-owned plot layout and data/screen transforms;
- custom X and semantic-Y axes, ticks and grid;
- raw series rendering through Dear ImGui `ImDrawList`;
- pan/zoom X and active-series Y;
- Fit X / Fit Y;
- screen-space RMB series hit-test;
- active series with Halo / Outline;
- Custom Legend and visibility;
- Values with linear interpolation and crosshair;
- A/B time cursors;
- numbered draggable markers;
- explicit input-priority routing;
- exclusive fullscreen with `F11`;
- raw Reference / Stress Raw path for later performance evaluation.

Overview/minimap, irregular sampling, gaps/NaN semantics, step/digital signals and LOD/downsampling remain outside the first experiment.

## Architecture

Dear ImGui remains the GUI framework. The chart itself is application-owned.

`ChartMath` owns reusable coordinate/navigation/tick mathematics. `Chart` owns plot rendering, hit-testing and interaction routing. `CursorModel` and `MarkerModel` keep time-based overlay state. `InputPolicy` makes modifier priority explicit and independently testable.

There is no external plotting library and no artificial shared Y coordinate space. Each visible series maps directly from its own raw `viewMin..viewMax` to the common plot rectangle.

## Rendering backends

OpenGL 3.3 through GLFW/WGL is the default backend:

```text
run
run --backend=opengl
```

On Windows an alternative DirectX 11 backend is available:

```text
run --backend=dx11
```

The DX11 backend uses `IDXGIFactory2::CreateSwapChainForHwnd` with `DXGI_SWAP_EFFECT_FLIP_DISCARD`.

### Known Windows/OpenGL presentation issue

On the target Windows 10 PC with Intel HD Graphics 530 and driver `30.0.101.1692`, very short random colored flashes/rectangles can appear in a large, especially maximized, normal OpenGL window. The same class of artifact was also observed in the earlier ImPlot prototype, so it is not considered specific to the custom chart geometry.

Diagnostics established:

- disabling texture-based line AA (`AntiAliasedLinesUseTex = false`) reduces the visibility/frequency but does not eliminate the issue;
- disabling Windows MPO did not eliminate it;
- legacy DX11 presentation with `DXGI_SWAP_EFFECT_DISCARD` also flickered;
- DX11 with `DXGI_SWAP_EFFECT_FLIP_DISCARD` showed no observed flicker in the same maximized-window manual test;
- exclusive fullscreen also showed no observed flicker.

The project intentionally keeps native OpenGL as the default backend in its current form. `--backend=dx11` is retained as an alternative presentation path and a known working reference for this machine.

## Current status

The complete first custom-chart specification has been implemented on `main`.

Implemented:

- custom time and Y axes;
- neutral and active semantic grid;
- mouse-anchored multiplicative X zoom and grab-content X pan;
- active-Y pan/zoom;
- stable fixed plot geometry;
- series selection by nearest visible segment;
- cursor A/B set, drag and hide;
- marker create, drag and remove with Alt-click/Alt-drag threshold;
- exclusive modifier routing;
- reset/preset switching;
- Small, Reference, Overlap, Mixed Scale, Spikes / Noise, Long Time and Stress Raw datasets;
- functional tests for chart math, input policy, marker state and deterministic generator behavior;
- OpenGL default renderer plus optional Windows DX11 flip-model backend;
- `F11` exclusive fullscreen.

Temporary F8/F9/F10 window modes used during presentation diagnostics have been removed.

Manual UX and raw Reference / Stress Raw performance validation remain pending. The current GUI suite is still a smoke/integration suite and does not yet exercise every interaction gesture end-to-end.

## Stack

- C++20
- Windows first
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core Profile — default renderer
- DirectX 11 / DXGI `FLIP_DISCARD` — optional Windows renderer
- Dear ImGui `v1.92.9b`
- GoogleTest + CTest
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`

GLFW, Dear ImGui and Test Engine are fetched by CMake using pinned references. GoogleTest is found through `find_package(GTest REQUIRED)`.

## Build and run

```text
cmake -S . -B build -G Ninja
cmake --build build
```

Windows executable:

```text
build/canstatio_imgui_custom_test.exe
```

Default OpenGL run:

```text
run
```

Optional DirectX 11 run:

```text
run --backend=dx11
```

`run.bat` forwards command-line arguments to the executable.

Functional tests:

```text
ctest --test-dir build --output-on-failure
```

GUI integration tests:

```text
build/canstatio_imgui_custom_test.exe --run-tests
```

## Testing policy

Normal validation is performed on the target Windows PC. The Test Agent is used only as a communication path to that environment.

`.github/workflows/windows-ci.yml` remains a manually triggered Windows build + functional fallback. Linux GitHub Actions is not part of the project.

Manual validation remains required for visual quality, interaction ergonomics and raw-rendering performance.

## Repository discipline

Work in this conversation is scoped strictly to `CANStatio-ImGui-custom-test`. No other repository may be written, modified or cleaned up.

After meaningful implementation or verification work, update `docs/PROGRESS.md`. Requirement changes belong in `docs/PROJECT_SPEC.md` and durable decisions in `docs/DECISIONS.md`.
