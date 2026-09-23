# Postęp prac

## Stan na 2026-09-24

Normalny branch rozwojowy:

```text
main
```

Repozytorium jest niezależnym eksperymentem Dear ImGui z własną implementacją wykresu. Skopiowany wcześniejszy prototyp był wyłącznie bazą migracyjną; rozwiązania techniczne poprzedniego renderera nie są częścią aktualnej architektury.

## Specyfikacja

**Status: kompletna bazowa specyfikacja pierwszego eksperymentu.**

`docs/PROJECT_SPEC.md` definiuje model danych, rendering, osie, nawigację, active/visibility, Values, cursory, markery, input priority, presety i kryterium sukcesu.

## Implementacja

### C0 — dokumentacja i wymagania

**Zakończony.**

### C1 — minimalny własny plot

**Zakończony.**

Usunięto zewnętrzną bibliotekę wykresową i utworzono app-owned plot rectangle z bezpośrednim raw-data -> screen rendering przez Dear ImGui `ImDrawList`.

### C2 — transforms, osie, grid i X navigation

**Zaimplementowany.**

- `ChartMath` jako wspólna matematyka data/screen;
- transformacje X/Y i odwrotne;
- własne ticki czasu;
- własna oś X i format względnego czasu;
- neutralny grid;
- plain LMB grab-content pan X;
- plain wheel zoom X względem czasu pod myszą;
- minimum X = `dt`;
- brak clampa viewportu do datasetu;
- Fit X;
- stała geometria plot area.

### C3 — niezależny Y i active series

**Zaimplementowany.**

- bezpośrednie mapowanie każdego `viewMin/viewMax` do plot area;
- semantic Y active visible series;
- Y ticks `1/2/5 × 10^n`;
- kolorowy semantic grid;
- Fit Y;
- `Alt + drag` grab-content pan Y;
- `Alt + wheel` zoom Y wokół środka zakresu;
- RMB screen-space hit-test segmentów;
- toggle/clear active;
- hidden-active zachowuje logiczny stan;
- Halo / Outline korzystają z tej samej geometrii co linia danych.

### C4 — Values i crosshair

**Zaimplementowany.**

- mouse time pochodzi z własnej transformacji screen -> time;
- Values zachowuje interpolację liniową;
- ukryte serie nie pojawiają się w Values;
- crosshair jest niezależnym dokładnym mouse-X i nie snapuje do próbek;
- zachowanie działa z app-owned pan/zoom X.

### C5 — kursory A/B

**Zaimplementowany.**

- dokładnie dwa time cursors A/B;
- `Shift + LMB` set A;
- `Ctrl + LMB` set B;
- plain LMB drag istniejącego kursora;
- `Ctrl + RMB` hide;
- clamp do datasetu;
- własny render, hit-test i drag w screen-space;
- po pan/zoom X kursory zachowują czas.

### C6 — markery i pełny input priority

**Zaimplementowany.**

- `MarkerModel` z numeracją `0,1,2...`;
- usunięte ID nie jest ponownie używane;
- Reset zeruje licznik;
- `Alt + LMB` tworzy marker po release, jeśli ruch pozostał poniżej progu;
- po przekroczeniu około 5 px gest staje się pan Y przy active visible series;
- bez active visible series przekroczony Alt-drag jest anulowany;
- plain LMB drag markera;
- `Ctrl + RMB` remove;
- jawna `InputPolicy` dla priorytetów Shift/Ctrl/Alt/plain;
- modifier gestures nie przechodzą awaryjnie do X/selection;
- drag zachowuje ownership do release.

### C7 — pełne presety i ścieżka performance

**Zaimplementowana funkcjonalnie.**

Dostępne presety:

- Small / Sanity — 3 × 1 000, 100 ms;
- Reference — 60 × 36 000, 100 ms;
- Overlap — 60 × 36 000;
- Mixed Scale — 60 × 36 000;
- Spikes / Noise — 60 × 36 000;
- Long Time — 10 × 108 000, 100 ms;
- Stress Raw — 60 × 360 000, 10 ms.

Renderer nadal działa raw, bez LOD/downsamplingu, zgodnie ze specyfikacją. Widoczny zakres próbek jest ograniczany do viewportu X z małym zapasem.

Pomiary wydajności i decyzja o potrzebie LOD **nie zostały jeszcze wykonane**.

## Reset i preset switching

`Test Controls` obsługuje wybór wszystkich presetów oraz pełny Reset.

Reset:

- Fit X do pełnego datasetu;
- wszystkie serie visible;
- brak active;
- Fit Y każdej serii;
- cursory A/B hidden;
- markers cleared;
- marker counter = 0.

## Testy dodane razem z pełną implementacją

Nowe testowalne komponenty:

- `ChartMath` — transforms, pan/zoom, nice/time ticks, point-to-segment;
- `InputPolicy` — priorytety modifierów i brak fallbacków;
- `MarkerModel` — ID, clamp, move, remove, reset;
- `DataGenerator` — Small specification, determinism i nazwy presetów.

Zachowane są testy `Dataset`, `Series`, `ValuesModel` i `CursorModel`, a GUI smoke suite sprawdza startup, okna, Custom Legend, highlight, plot geometry/mouse-time i Fit X.

## Weryfikacja

### Historyczny baseline

Skopiowana baza przed custom rendererem miała:

```text
21/21 functional PASS + 19/19 GUI PASS
```

Nie jest to wynik aktualnej implementacji.

### Aktualna pełna implementacja — target Windows

Zweryfikowany checkout:

```text
branch: main
commit: 5f17de5
working tree: clean
```

Rejestracja projektu w Test Agencie wykonała pełne configure + build:

```text
configure: PASS
build: PASS
```

Środowisko:

```text
Windows 10 build 19045
GCC/G++ 16.2.0
CMake 4.4.3
CTest 4.4.3
Ninja 1.13.2
GoogleTest 1.18.0
```

Functional suite:

```text
41/41 PASS
100% tests passed
exit code 0
```

GUI integration suite:

```text
6/6 PASS
Tests Result: OK
exit code 0
```

JUnit utworzony dla obu suite:

```text
build/test-results/functional-tests.xml
build/test-results/imgui-tests.xml
```

### Co pozostaje niezweryfikowane

- manualna jakość wizualna osi, grida, linii i highlightów;
- ergonomia rzeczywistych gestów X/Y, RMB, cursorów i markerów;
- performance Reference;
- performance Stress Raw;
- decyzja o potrzebie LOD/downsamplingu.

Aktualny GUI suite jest smoke/integration suite i nie obejmuje jeszcze end-to-end wszystkich gestów zaimplementowanych w C2–C6. Matematyka i polityka inputu są pokryte testami functional.

## Następny krok

Wykonać manualną walidację UX oraz pomiar Reference / Stress Raw. Następnie rozszerzyć GUI regression o najważniejsze rzeczywiste gesty, szczególnie:

```text
plain drag / wheel X
Alt drag / wheel Y
RMB selection
cursor set / drag / hide
marker create / drag / remove
modifier conflicts
```

Dopiero wyniki Reference / Stress Raw powinny zdecydować, czy potrzebny jest LOD/downsampling lub dalsza optymalizacja.
