# Automatyczne testy

## Cel

Automatyzacja chroni semantykę własnego wykresu bez zastępowania ręcznej oceny wyglądu, ergonomii i płynności.

Warstwy:

1. **functional C++** — logika, matematyka, transformacje, stan i input policy bez GUI;
2. **GUI integration** — rzeczywiste widgety Dear ImGui i podstawowy stan integracyjny;
3. **manual** — wygląd, ergonomia, chwytanie narzędzi i performance.

## Narzędzia

Functional:

```text
GoogleTest + CTest
canstatio_functional_tests
```

GUI integration:

```text
Dear ImGui Test Engine
canstatio_imgui_custom_test.exe --run-tests
```

GoogleTest jest znajdowany przez `find_package(GTest REQUIRED)`. Dear ImGui Test Engine pozostaje przypięty do commita `508a8fc8dacac2f346d353fed31b9bc90ed29adc`.

## Aktualny functional suite

Zachowane testy:

- `Dataset::endTimeSeconds()`;
- `Series::fitViewToData()`;
- `ValuesModel` — interpolacja, visibility, granice i hover state;
- `CursorModel` — niezależne A/B, clamp i hide.

Dodane dla pełnej implementacji custom chart:

### `ChartMathTests.cpp`

- data -> screen -> data round-trip;
- kierunek grab-content pan X;
- zoom X z nieruchomym anchor point pod myszą;
- minimalny span X;
- grab-content pan Y;
- zoom Y z zachowaniem środka;
- family `1/2/5 × 10^n`;
- generacja ticków czasu;
- point-to-segment distance używana przez hit-test.

### `InputPolicyTests.cpp`

- priorytet Shift/Ctrl/Alt dla LMB;
- plain LMB: cursor/marker drag przed pan X;
- `Ctrl + RMB` usuwa/hide tylko overlay i nie przechodzi do selection;
- plain RMB uruchamia selection;
- wheel: plain X, Alt Y, brak fallbacku dla nieobsługiwanych modifierów.

### `MarkerModelTests.cpp`

- sekwencyjne ID;
- clamp czasu;
- brak ponownego używania usuniętego ID;
- move;
- Reset i powrót licznika do 0.

### `DataGeneratorTests.cpp`

- parametry Small / Sanity;
- deterministyczność Small;
- stabilne nazwy wszystkich presetów.

## GUI integration suite

Aktualny smoke suite:

```text
custom_chart/startup_windows
custom_chart/window_toggles
custom_chart/custom_legend_active_series
custom_chart/active_highlight_mode
custom_chart/plot_geometry_and_mouse_time
custom_chart/fit_x
```

Suite jest celowo oparty na zachowaniu użytkowym i publicznym stanie testowym, a nie na strukturze renderera.

Pełna implementacja ma znacznie większy zakres niż aktualny GUI smoke suite. Należy rozszerzyć GUI regression przede wszystkim o rzeczywiste gesty X/Y, RMB selection, cursory i markery. Functional tests już chronią matematykę oraz jawny routing modifierów.

## Weryfikacja obecnego kodu

Target Windows PC zweryfikował oczyszczony kod po usunięciu tymczasowych trybów diagnostycznych F8/F9/F10.

Zweryfikowany commit kodu:

```text
a244fc0
```

Configure/build:

```text
configure: PASS
build: PASS
```

Functional CTest:

```text
41/41 PASS
100% tests passed
exit code 0
```

GUI Dear ImGui Test Engine:

```text
6/6 PASS
Tests Result: OK
exit code 0
```

Raporty JUnit zostały utworzone:

```text
build/test-results/functional-tests.xml
build/test-results/imgui-tests.xml
```

GUI suite był uruchomiony na domyślnym backendzie OpenGL i zarejestrował:

```text
OpenGL vendor: Intel
OpenGL renderer: Intel(R) HD Graphics 530
OpenGL version: 3.3.0 - Build 30.0.101.1692
```

Późniejsze commity `README.md`, `docs/DECISIONS.md`, `docs/PROGRESS.md`, `AGENTS.md` i tego pliku są zmianami dokumentacyjnymi dotyczącymi handoffu i nie zmieniają zweryfikowanego kodu wykonywalnego.

## Historyczny baseline

Przed migracją skopiowany prototyp miał:

```text
Test Agent Windows: 21/21 functional PASS
Test Agent Windows: 19/19 GUI PASS
```

To wyłącznie punkt odniesienia historycznego i nie należy mieszać go z aktualnym wynikiem 41/41 + 6/6.

## Co nadal wymaga rozszerzenia automatyzacji

Aktualny GUI suite jest smoke/integration suite. Nie obejmuje jeszcze end-to-end:

- plain drag / wheel X;
- Alt drag / wheel Y;
- RMB series selection;
- cursor set / drag / hide;
- marker create / drag / remove;
- konfliktów modifierów na rzeczywistym plot area.

Te elementy mają już testowalną matematykę/state/input policy po stronie functional, ale powinny dostać także najważniejsze testy GUI regression.

## Manualna walidacja

Manualnie należy sprawdzić przede wszystkim:

- czy osie i grid są czytelne;
- czy Halo / Outline nie pogarszają odczytu danych;
- czy pan/zoom mają właściwy kierunek i szybkość;
- czy RMB selection jest wygodne przy overlap;
- czy cursory i markery łatwo złapać;
- czy Alt-click / Alt-drag threshold jest ergonomiczny;
- czy Reference jest praktycznie płynny;
- gdzie Stress Raw ujawnia potrzebę LOD.

Znany problem prezentacji Windows/OpenGL i alternatywny backend DX11 są opisane w `README.md`, `docs/DECISIONS.md` i `docs/PROGRESS.md`. Nie są traktowane jako automatyczny PASS/FAIL testów GUI.

## Docelowa dalsza sekwencja walidacji

Po obecnym PASS pozostaje:

```text
manual UX
-> rozszerzenie GUI regression
-> Reference performance
-> Stress Raw performance
```

## GitHub Actions fallback

`.github/workflows/windows-ci.yml` jest ręcznym `workflow_dispatch` fallbackiem dla Windows build + functional tests. Nie jest automatycznym CI dla push/PR i nie zastępuje GUI/manual testów na docelowym PC.

Linux GitHub Actions nie jest częścią projektu.