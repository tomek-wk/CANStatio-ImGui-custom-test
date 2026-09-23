# Automatyczne testy

## Cel

Automatyzacja ma wykrywać regresje funkcjonalne eksperymentu Dear ImGui / ImPlot bez zastępowania ręcznej oceny wyglądu, ergonomii i UX.

Strategia ma trzy warstwy:

1. **functional C++** — logika, matematyka, stan i edge-case'y bez GUI;
2. **GUI integration** — rzeczywiste interakcje Dear ImGui / ImPlot przez Dear ImGui Test Engine;
3. **manual** — wygląd, czytelność, ergonomia, subiektywna płynność i UX.

Ten dokument jest nadrzędnym źródłem prawdy w zakresie automatycznych testów, ich narzędzi i uruchamiania.

## Status integracji

Eksperyment automatycznego testowania prowadzony na `feature/imgui-test-engine` został zakończony i zintegrowany do `main` przez PR #1 2026-09-23.

Normalny rozwój projektu odbywa się teraz na `main`. Historyczna merge policy D-043 dotyczyła tego zakończonego brancha i została spełniona jawną zgodą użytkownika przed merge.

## Zasady wyboru narzędzi

- nie tworzymy osobnej biblioteki `core` tylko dla testów;
- nie przenosimy FlaUI / UIA3 / xUnit ze starego wxWidgets;
- nie dodajemy nowej zależności, jeśli istniejące narzędzia rozwiązują problem;
- najpierw testujemy istniejący produkcyjny interfejs;
- małą czystą funkcję lub model wydzielamy tylko wtedy, gdy zapobiega to duplikowaniu algorytmu albo kruchemu testowi GUI i ma sens produkcyjny.

`ValuesModel` i `CursorModel` są przykładami małych produkcyjnych modeli współdzielonych przez aplikację i testy.

# Narzędzia

## Functional

Docelowy Windows PC:

```text
CMake 4.4.3
CTest 4.4.3
GoogleTest 1.18.0
GCC/G++ 16.2.0
Ninja 1.13.2
```

Target:

```text
canstatio_functional_tests
```

JUnit:

```text
build/test-results/functional-tests.xml
```

GoogleTest jest znajdowany przez `find_package(GTest REQUIRED)`; repo go nie pobiera.

## GUI

Framework: **Dear ImGui Test Engine**.

Bazowe wersje:

- Dear ImGui `v1.92.9b`;
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`.

Tryby lokalne:

```text
canstatio_implot_test.exe --run-tests
canstatio_implot_test.exe --run-tests <filter>
canstatio_implot_test.exe --show-test-ui
```

JUnit GUI:

```text
build/test-results/imgui-tests.xml
```

Nie dodajemy OS-level GUI automation bez konkretnej luki, której Dear ImGui Test Engine nie potrafi wiarygodnie sprawdzić.

# CANStatio Test Agent — podstawowa ścieżka

CANStatio Test Agent `0.7` jest podstawowym zdalnym środowiskiem testowym docelowego Windows PC.

Normalny przepływ:

```text
configure/build
-> functional
-> GUI
-> manual UX, jeśli zmiana dotyczy interakcji lub wyglądu
```

Agent udostępnia bounded actions, w tym `test` z `suite=functional` i `suite=gui`; nie udostępnia ogólnego remote shell.

# GitHub Actions — tylko awaryjny Windows fallback

GitHub Actions nie jest normalną ścieżką testową i nie jest dodatkowym checkpointem.

Używamy go **wyłącznie wtedy, gdy docelowy Windows PC lub Test Agent jest niedostępny**.

Workflow:

```text
.github/workflows/windows-ci.yml
```

Jest celowo `workflow_dispatch` only — bez triggerów `push` i `pull_request`.

Zakres:

```text
checkout
-> MSYS2 UCRT64
-> configure
-> pełny build obu executable
-> functional CTest
-> JUnit artifact
```

Artifact:

```text
windows-functional-test-results
```

Standardowy GitHub-hosted Windows runner nie uruchamia GUI suite. Eksperymentalny start GUI zakończył się błędem GLFW `65542` z powodu braku użytecznego WGL/OpenGL. Nie dokładamy ANGLE/OSMesa tylko po to, aby wymusić GUI na runnerze.

## Linux GitHub Actions

Linux GitHub Actions był jednorazowym eksperymentem przenośności. Workflow został usunięty i Linux CI **nie jest częścią bieżącej strategii testowej**. Nie należy go przywracać bez nowej jawnej decyzji użytkownika.

# Stan zweryfikowany 2026-09-23

## Functional

```text
Test Agent Windows:    21/21 PASS
GitHub Windows:        21/21 PASS — historycznie zweryfikowany fallback
```

Pokrycie obejmuje `Dataset`, `Series::fitViewToData`, `ValuesModel` i `CursorModel`.

## GUI integration

```text
Test Agent Windows:    19/19 PASS
GitHub Windows:        niewykonywane — brak użytecznego WGL/OpenGL
```

Zestaw obejmuje startup, okna, legendę, active/hidden-active, Halo/Outline mode, Fit X/Y, X navigation, `Alt` Y navigation, Values/crosshair, RMB hit-test, kursory A/B oraz stabilność geometrii plot area przy zmianach active Y.

Testy kursora:

```text
cursors/set_a_and_b
cursors/plain_drag_and_ctrl_rmb_remove
cursors/alt_drag_over_cursor_pans_y
```

Trzeci test nie oznacza specjalnej ścieżki implementacyjnej dla kursora. Chroni ogólną zasadę: `Alt + drag` jest pan Y; kursor przy `Alt` nie reaguje.

Test layoutu:

```text
canstatio/stable_plot_area_active_y_axis
```

Sprawdza niezmienne `plotPos`/`plotSize` dla braku active, aktywacji serii, zmiany na serię o innej skali Y i ponownego wyłączenia active.

## Plan functional F0–F8

- **F0 środowisko + runner — zakończony**;
- **F1 Dataset / Series — zakończony dla obecnego modelu**;
- **F2 Values / interpolacja — zakończony**;
- **F3 mapowanie Y / Fit Y — częściowo pokryte; dalszy refaktor tylko przy realnej potrzebie**;
- **F4 pan/zoom Y math — warunkowo później**;
- **F5 RMB hit-test math — warunkowo później**;
- **F6 time ticks / formatting — przyszły kandydat**;
- **F7 kursory A/B — zakończony dla aktualnej implementacji**;
- **F8 markery/input priority — następny etap; testy razem z implementacją**.

## Czego automaty nie zastępują

- jakości wizualnej Halo / Outline i ich joinów;
- praktycznej szerokości/czytelności semantic Y gutter;
- czytelności Values i crosshair;
- kolorów, etykiet i łatwości chwytania kursorów A/B;
- subiektywnej ergonomii i płynności;
- manualnego sprawdzenia zachowania na rzeczywistym docelowym sprzęcie.

Ostatni cleanup Y-gutter i Halo/Outline został ręcznie zaakceptowany przez użytkownika po przejściu automatycznych testów.
