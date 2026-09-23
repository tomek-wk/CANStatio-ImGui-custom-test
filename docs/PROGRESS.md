# Postęp prac

## Stan na 2026-09-23

Normalny branch rozwojowy:

```text
main
```

Repozytorium zostało utworzone jako niezależny wariant eksperymentalny dla Dear ImGui z własną implementacją wykresu.

Początkowo skopiowano do niego działającą bazę wcześniejszego prototypu, aby zachować sprawdzone modele danych, zachowania użytkowe i testy jako materiał referencyjny.

Skopiowany renderer i jego rozwiązania techniczne są traktowane jako legacy baseline, a nie jako architektura docelowa.

## C0 — dokumentacja i wymagania

**Status: zakończony.**

- zebrano aktualną i planowaną funkcjonalność;
- usunięto z dokumentacji założenia zależne od poprzedniego renderera;
- zdefiniowano app-owned plot, transforms, input i overlays;
- zresetowano rejestr decyzji pod wariant custom chart;
- zapisano nadrzędną granicę repozytorium.

## C1 — minimalny własny plot i build

**Status implementacji: zakończony.**

**Status weryfikacji na target Windows: oczekuje.**

Zaimplementowano:

- usunięcie starej biblioteki wykresowej z `CMakeLists.txt`;
- zmianę projektu na `CANStatioImGuiCustomTest`;
- zmianę executable na `canstatio_imgui_custom_test`;
- usunięcie tworzenia i niszczenia starego kontekstu wykresowego;
- własny `ChartHost` i app-owned plot rectangle;
- bezpośrednie mapowanie czasu na X w pikselach;
- bezpośrednie mapowanie wartości każdej serii z jej `viewMin/viewMax` na Y w pikselach;
- clipping renderowania do plot area;
- rysowanie widocznych serii przez `ImDrawList::AddPolyline`;
- active series rysowaną na końcu;
- Halo/Outline korzystające z tej samej geometrii screen-space co cienka linia danych;
- mouse time liczony z własnego plot rectangle;
- crosshair oparty o własną geometrię;
- Values nadal korzystające z `ChartMouseState`, już zasilanego przez custom plot;
- usunięcie renderer-specific `CursorOverlay`;
- usunięcie starych GUI testów navigation/selection/Values/cursors zależnych od poprzedniej architektury;
- nowy minimalny GUI suite C1 obejmujący startup, okna, Custom Legend, highlight mode, plot geometry/mouse time i Fit X;
- aktualizację `run.bat` i `update.bat` do nowej nazwy executable.

Nie zostały jeszcze przeniesione:

- oś czasu X i jej ticki;
- neutralny grid;
- pan/zoom X;
- semantic Y i kolorowy grid;
- pan/zoom Y;
- RMB selection;
- własny cursor overlay A/B;
- markery;
- pełny input priority.

### Weryfikacja C1

Próba użycia Test Agenta została zatrzymana przed kolejkovaniem komend:

- ostatni dostępny heartbeat agenta był nieaktualny względem czasu pracy;
- dokumentowana lokalna konfiguracja agenta zawierała tylko poprzedni projekt, nie `CANStatio-ImGui-custom-test`.

Dlatego nie deklarujemy configure/build ani testów jako wykonanych.

## Docelowa architektura

Dear ImGui pozostaje frameworkiem GUI.

Własna warstwa wykresu ma przejąć:

- plot area i layout;
- data/screen transforms;
- render serii;
- clipping;
- osie, ticki, etykiety i grid;
- pan/zoom X;
- pan/zoom Y aktywnej serii;
- hit-test serii;
- input routing;
- active highlight;
- crosshair;
- cursory A/B;
- markery.

Nie używamy wspólnej sztucznej przestrzeni Y. Każda seria jest mapowana bezpośrednio ze swojego `viewMin..viewMax` do pikseli plot area.

## Funkcjonalność do zachowania

Z baseline pozostają wymaganiami użytkowymi, jeśli później nie zostaną jawnie zmienione:

- regularny wspólny X;
- niezależny `viewMin/viewMax` każdej serii;
- Fit X i Fit Y;
- active series;
- visibility serii;
- Custom Legend;
- semantic Y aktywnej serii;
- neutralny i kolorowy grid;
- plain drag / wheel dla X;
- `Alt + drag` / `Alt + wheel` dla aktywnego Y;
- RMB hit-test serii;
- Values z interpolacją;
- crosshair;
- Halo / Outline;
- kursory A/B;
- markery i input priority.

## Historyczna weryfikacja baseline

Skopiowana baza miała wcześniej potwierdzony wynik:

```text
Test Agent Windows: 21/21 functional PASS + 19/19 GUI PASS
```

Ten wynik nie jest wynikiem custom renderera.

## Co zachowano bez przepisywania od zera

Na etapie C1 pozostawiono:

- `Dataset` i `Series`;
- `DataGenerator`;
- `ValuesModel`;
- `CursorModel` jako semantyczny state na przyszły etap C5;
- deterministic colors;
- `CustomLegend`;
- `ValuesWindow`;
- funkcjonalne testy modeli.

## Kolejne etapy

### C2 — transforms, osie, grid i X navigation

Następny etap.

Cel:

- wydzielić wspólny, testowalny system data/screen;
- własna oś czasu;
- X tick generation;
- neutralny grid;
- pan X;
- zoom X;
- Fit X;
- stabilny plot layout.

### C3 — niezależny Y i active series

Cel:

- semantic Y aktywnej serii;
- kolorowy grid;
- Fit Y;
- `Alt + drag` / `Alt + wheel`;
- RMB hit-test;
- pełna integracja Custom Legend;
- manualna ocena Halo / Outline.

### C4 — Values i crosshair

Cel:

- rozszerzyć i ponownie zweryfikować obecny C1 mouse-time path;
- Values/crosshair przy pan/zoom X.

### C5 — kursory A/B

Cel:

- wykorzystać `CursorModel`, jeśli nadal pasuje;
- własny render, hit-test i drag;
- Shift/Ctrl set;
- plain drag;
- Ctrl+RMB hide;
- clamp do datasetu.

### C6 — markery i pełny input priority

Cel:

- numerowane markery;
- Alt-click vs Alt-drag threshold;
- plain drag markera;
- Ctrl+RMB remove;
- kolizje z cursorami;
- pełne testy priorytetów gestów.

### C7 — pełne presety i wydajność

Cel:

- Reference;
- Overlap;
- Mixed Scale;
- Spikes / Noise;
- Long Time;
- Stress Raw;
- pomiary raw renderingu;
- decyzja, czy LOD jest faktycznie potrzebny.

## Automatyzacja

Projekt ma trzy poziomy weryfikacji:

1. functional C++ — GoogleTest + CTest;
2. GUI integration — Dear ImGui Test Engine;
3. manual — wygląd, ergonomia i płynność.

Podstawową ścieżką pozostaje docelowy Windows PC przez CANStatio Test Agent.

## Najbliższy krok

Najpierw wykonać zaległą weryfikację C1, gdy `CANStatio-ImGui-custom-test` będzie dostępny w Test Agencie. Następnie rozpocząć **C2 — transforms, time axis, grid i app-owned X navigation**.