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

Pełna implementacja ma znacznie większy zakres niż aktualny GUI smoke suite. Po uruchomieniu środowiska testowego należy rozszerzyć GUI regression przede wszystkim o rzeczywiste gesty X/Y, RMB selection, cursory i markery. Functional tests już chronią matematykę oraz jawny routing modifierów.

## Weryfikacja obecnego kodu

Aktualny kod pełnego custom chart **nie został jeszcze uruchomiony w środowisku testowym**.

Nie raportujemy PASS/FAIL dla obecnego commita.

Zgodnie z bieżącą instrukcją użytkownika CANStatio Test Agent nie jest używany, ponieważ obecnie nie działa. Nie wolno traktować historycznych wyników skopiowanego prototypu jako walidacji aktualnego renderera.

## Historyczny baseline

Przed migracją skopiowany prototyp miał:

```text
Test Agent Windows: 21/21 functional PASS
Test Agent Windows: 19/19 GUI PASS
```

To wyłącznie punkt odniesienia historycznego.

## Docelowa sekwencja walidacji

Gdy środowisko będzie dostępne:

```text
configure/build
-> ctest functional
-> Dear ImGui Test Engine GUI
-> manual UX
-> Reference performance
-> Stress Raw performance
```

Manualnie należy sprawdzić przede wszystkim:

- czy osie i grid są czytelne;
- czy Halo / Outline nie pogarszają odczytu danych;
- czy pan/zoom mają właściwy kierunek i szybkość;
- czy RMB selection jest wygodne przy overlap;
- czy cursory i markery łatwo złapać;
- czy Alt-click / Alt-drag threshold jest ergonomiczny;
- czy Reference jest praktycznie płynny;
- gdzie Stress Raw ujawnia potrzebę LOD.

## GitHub Actions fallback

`.github/workflows/windows-ci.yml` jest ręcznym `workflow_dispatch` fallbackiem dla Windows build + functional tests. Nie jest automatycznym CI dla push/PR i nie zastępuje GUI/manual testów na docelowym PC.

Linux GitHub Actions nie jest częścią projektu.
