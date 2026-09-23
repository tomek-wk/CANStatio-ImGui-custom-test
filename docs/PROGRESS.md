# Postęp prac

## Stan na 2026-09-23

Normalny branch rozwojowy:

```text
main
```

Repozytorium zostało utworzone jako niezależny wariant eksperymentalny dla Dear ImGui z własną implementacją wykresu.

Początkowo skopiowano do niego działającą bazę wcześniejszego prototypu, aby zachować sprawdzone modele danych, zachowania użytkowe i testy jako materiał referencyjny.

Na tym etapie skopiowany renderer i jego rozwiązania techniczne są traktowane jako **legacy baseline**, a nie jako architektura docelowa.

## Reset dokumentacji — zakończony

Dokumentacja została przepisana pod wariant własnego wykresu:

- `README.md` opisuje nowy cel repozytorium;
- `AGENTS.md` definiuje pracę wyłącznie w tym repozytorium oraz nową odpowiedzialność renderera;
- `docs/PROJECT_SPEC.md` opisuje funkcjonalność bez zależności od zewnętrznego plot widgetu;
- `docs/DECISIONS.md` został zresetowany do decyzji właściwych dla custom chart;
- `docs/AUTOMATED_TESTS.md` opisuje plan ponownej walidacji po migracji;
- ten plik rozdziela legacy baseline od faktycznie przeniesionej funkcjonalności.

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

Nie zakładamy wspólnej sztucznej przestrzeni Y. Każda seria ma być mapowana bezpośrednio ze swojego `viewMin..viewMax` do pikseli plot area.

## Funkcjonalność do zachowania

Skopiowany baseline dostarcza zachowania, które mają zostać odtworzone w custom rendererze, jeśli nie zostaną później jawnie zmienione:

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
- docelowe markery i input priority.

Nie zachowujemy automatycznie rozwiązań technicznych, które były potrzebne tylko przez poprzedni renderer.

## Historyczna weryfikacja baseline

Skopiowana baza miała wcześniej potwierdzony wynik:

```text
Test Agent Windows: 21/21 functional PASS + 19/19 GUI PASS
```

Ten wynik jest punktem odniesienia dla zachowania sprzed migracji. Nie jest wynikiem custom renderera.

Po rozpoczęciu migracji każdy dotknięty obszar musi zostać ponownie zweryfikowany.

## Co można zachować bez przepisywania od zera

Najbardziej prawdopodobne elementy do ponownego użycia:

- `Dataset` i `Series`;
- `DataGenerator`;
- `ValuesModel`;
- `CursorModel` jako semantyczny state;
- deterministic colors;
- część formatowania czasu i ticków;
- część matematyki RMB hit-test po odseparowaniu od starego systemu współrzędnych;
- część testów funkcyjnych modeli.

Każdy z tych elementów należy ocenić przed pozostawieniem. Sam fakt istnienia w baseline nie wystarcza.

## Plan migracji

### C0 — dokumentacja i wymagania

**Status: zakończony.**

- zebrano aktualną i planowaną funkcjonalność;
- usunięto z dokumentacji założenia zależne od poprzedniego renderera;
- zdefiniowano app-owned plot, transforms, input i overlays;
- ustalono nowe źródła prawdy.

### C1 — minimalny własny plot i build

**Status: następny etap.**

Cel:

- usunąć zależność starego renderera z konfiguracji projektu;
- zmienić nazwę projektu/targetu/executable na wariant custom;
- uruchomić aplikację z Dear ImGui + GLFW + OpenGL bez starego backendu wykresu;
- utworzyć własny plot rectangle;
- narysować co najmniej jedną serię przez własną geometrię screen-space i clipping.

Na końcu: build + minimalny GUI smoke test.

### C2 — transforms, osie, grid i X navigation

Cel:

- wspólny system data/screen;
- własna oś czasu;
- X tick generation;
- neutralny grid;
- pan X;
- zoom X;
- Fit X;
- stabilny plot layout.

### C3 — niezależny Y i active series

Cel:

- bezpośrednie mapowanie każdej serii z jej własnego `viewMin/viewMax`;
- semantic Y aktywnej serii;
- kolorowy grid;
- Fit Y;
- `Alt + drag` / `Alt + wheel`;
- RMB hit-test;
- Custom Legend;
- Halo / Outline.

### C4 — Values i crosshair

Cel:

- wspólny mouse time z własnego plot transform;
- Values z istniejącą interpolacją;
- crosshair niezależny od Values.

### C5 — kursory A/B

Cel:

- zachować `CursorModel` jeśli nadal pasuje;
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

Projekt nadal ma trzy poziomy weryfikacji:

1. functional C++ — GoogleTest + CTest;
2. GUI integration — Dear ImGui Test Engine;
3. manual — wygląd, ergonomia i płynność.

Podstawową ścieżką jest docelowy Windows PC przez CANStatio Test Agent.

GitHub-hosted Windows pozostaje wyłącznie ręcznym fallbackiem, gdy target PC/Test Agent jest niedostępny.

## Nadal niezaimplementowane w custom rendererze

Na moment resetu dokumentacji **cała warstwa custom chart jest jeszcze do wykonania**.

Skopiowany baseline może nadal uruchamiać się i przechodzić swoje stare testy, ale nie należy tego raportować jako postępu własnego renderera.

## Najbliższy krok

Rozpocząć **C1 — minimalny własny plot i build** na `main`.

Pierwszy checkpoint powinien być celowo mały: aplikacja bez starego backendu wykresu, własny prostokąt plot area, podstawowy clipping i jedna lub kilka linii danych rysowanych w screen-space. Dopiero po takim działającym fundamencie należy przenosić osie i interakcje.