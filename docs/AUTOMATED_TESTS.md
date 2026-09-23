# Automatyczne testy

## Cel

Automatyzacja ma wykrywać regresje funkcjonalne eksperymentu Dear ImGui z własną implementacją wykresu bez zastępowania ręcznej oceny wyglądu, ergonomii i UX.

Strategia ma trzy warstwy:

1. **functional C++** — logika, matematyka, transformacje, stan i edge-case'y bez GUI;
2. **GUI integration** — rzeczywiste interakcje Dear ImGui przez Dear ImGui Test Engine;
3. **manual** — wygląd, czytelność, ergonomia, subiektywna płynność i UX.

Ten dokument jest nadrzędnym źródłem prawdy w zakresie automatycznych testów, ich narzędzi i uruchamiania.

## Zasady wyboru narzędzi

- nie tworzymy osobnej biblioteki `core` tylko dla testów;
- nie przenosimy FlaUI / UIA3 / xUnit ze starego wxWidgets;
- nie dodajemy nowej zależności, jeśli istniejące narzędzia rozwiązują problem;
- małą czystą funkcję lub model wydzielamy tylko wtedy, gdy zapobiega to duplikowaniu algorytmu albo kruchemu testowi GUI i ma sens produkcyjny;
- matematyka data/screen, hit-test i pan/zoom powinna być testowana funkcyjnie tam, gdzie można ją oddzielić od renderowania.

## Narzędzia

### Functional

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

GoogleTest jest znajdowany przez `find_package(GTest REQUIRED)`.

### GUI

Framework: **Dear ImGui Test Engine**.

Bazowe wersje:

- Dear ImGui `v1.92.9b`;
- Dear ImGui Test Engine commit `508a8fc8dacac2f346d353fed31b9bc90ed29adc`.

Aktualny target GUI od C1:

```text
canstatio_imgui_custom_test
```

Uruchomienie suite:

```text
build/canstatio_imgui_custom_test.exe --run-tests
```

Nie dodajemy OS-level GUI automation bez konkretnej luki, której Dear ImGui Test Engine nie potrafi wiarygodnie sprawdzić.

## CANStatio Test Agent — podstawowa ścieżka

CANStatio Test Agent jest podstawowym zdalnym środowiskiem testowym docelowego Windows PC.

Normalny przepływ:

```text
configure/build
-> functional
-> GUI
-> manual UX, jeśli zmiana dotyczy interakcji lub wyglądu
```

Repozytorium Test Agenta nie jest modyfikowane przez ten projekt. Może być używane tylko jako kanał komunikacji z docelowym środowiskiem.

## GitHub Actions — tylko awaryjny Windows fallback

GitHub Actions nie jest normalną ścieżką testową i nie jest dodatkowym obowiązkowym checkpointem.

Używamy go wyłącznie wtedy, gdy docelowy Windows PC lub Test Agent jest niedostępny.

Workflow:

```text
.github/workflows/windows-ci.yml
```

Jest celowo `workflow_dispatch` only — bez triggerów `push` i `pull_request`.

Standardowy GitHub-hosted Windows runner nie jest podstawowym środowiskiem GUI. Linux GitHub Actions nie jest częścią bieżącej strategii testowej.

## Historyczny baseline przed migracją

Skopiowana baza projektu miała potwierdzony wynik:

```text
Test Agent Windows:    21/21 functional PASS
Test Agent Windows:    19/19 GUI PASS
GitHub Windows:        21/21 functional PASS — historycznie zweryfikowany fallback
```

Te wyniki nie są wynikiem własnego renderera.

## C1 — aktualny stan testów

Implementacja C1 zawiera nowy minimalny GUI suite:

```text
custom_chart/startup_windows
custom_chart/window_toggles
custom_chart/custom_legend_active_series
custom_chart/active_highlight_mode
custom_chart/plot_geometry_and_mouse_time
custom_chart/fit_x
```

Stare GUI testy zależne od poprzedniej architektury zostały usunięte zamiast sztucznie utrzymywać ich założenia.

C1 nie został jeszcze uruchomiony na target Windows. Powód jest operacyjny, nie testowy: ostatni dostępny heartbeat Test Agenta był nieaktualny, a jego dokumentowana lokalna konfiguracja nie zawierała jeszcze `CANStatio-ImGui-custom-test` jako zarejestrowanego projektu.

Nie raportujemy więc żadnego PASS/FAIL dla C1.

## Functional — elementy zachowane z baseline

Najbardziej prawdopodobne do bezpośredniego zachowania:

- `Dataset::endTimeSeconds()`;
- `Series::fitViewToData()`;
- `ValuesModel` — exact sample, interpolacja, hidden/no-hover/outside dataset i edge-case'y;
- `CursorModel` — start hidden, niezależne A/B, clamp i hide.

Ich wcześniejsze wyniki pozostają historyczne do czasu ponownego uruchomienia po C1.

## Plan testów functional

### F0 — środowisko + runner

Do ponownego potwierdzenia po C1.

### F1 — Dataset / Series

Baseline istnieje; uruchomić ponownie bez zmiany semantyki.

### F2 — transformacja X

Do dodania w C2:

- time -> screen X;
- screen X -> time;
- granice plot rect;
- zoomed/panned ranges;
- wartości poza viewportem.

### F3 — transformacja Y

Do dodania w C2/C3:

- raw value -> screen Y dla niezależnego `viewMin/viewMax`;
- screen Y -> raw value;
- różne skale serii;
- edge cases zakresu;
- wartości poza viewportem.

### F4 — X pan / zoom / Fit X math

Do dodania w C2.

### F5 — Y pan / zoom / Fit Y math

Do dodania lub rozszerzenia w C3.

### F6 — ticki i formatowanie czasu/Y

Do dodania w C2/C3.

### F7 — RMB hit-test math

Do dodania w C3.

### F8 — Values

Model baseline istnieje; ponowna walidacja po podpięciu pod pełny custom X navigation w C4.

### F9 — kursory A/B

State model baseline istnieje. Geometria i interakcje do zbudowania w C5.

### F10 — markery i input priority

Do dodania razem z C6.

### F11 — performance helpers

Dopiero po pełnych presetach, jeśli pojawią się testowalne elementy związane z widocznym zakresem indeksów lub przygotowaniem geometrii.

## Plan GUI integration

Docelowy zestaw ma pokrywać:

- startup i okna pomocnicze;
- stabilny plot area;
- Custom Legend visibility i active;
- Fit X / Fit Y;
- plain drag / wheel X;
- Alt drag / wheel Y;
- RMB selection;
- Values i crosshair;
- Halo / Outline mode;
- cursory A/B;
- markery;
- input priority i konflikty modifierów.

Testy GUI powinny sprawdzać semantykę i state, a nie utrwalać niepotrzebnie wewnętrzną strukturę renderera.

## Weryfikacja etapami migracji

### C1

Do wykonania, gdy repozytorium będzie dostępne w Test Agencie:

- configure/build;
- pełny retained functional suite;
- 6-testowy C1 GUI smoke suite;
- manualny start aplikacji i kontrola, czy raw series są widoczne.

### C2

- functional transforms X;
- tick/time formatting;
- GUI X pan/zoom/Fit X;
- manualna ocena osi i gridu.

### C3

- functional Y math i hit-test;
- GUI active/visibility/Fit Y/Y navigation/RMB;
- manual Halo/Outline i semantic Y.

### C4

- functional Values;
- GUI Values/crosshair.

### C5

- functional cursor state + geometry helpers;
- GUI set/drag/remove + modifier conflicts;
- manual chwytanie kursora i czytelność etykiet.

### C6

- functional marker state/input threshold;
- pełny GUI input-priority suite;
- manual ergonomia markerów.

### C7

- full regression;
- manual performance run na Reference i Stress Raw;
- zapis wyników i ewentualna decyzja o LOD.

## Czego automaty nie zastępują

- jakości wizualnej linii i highlightów;
- czytelności osi i etykiet;
- praktycznej szerokości obszaru osi Y;
- czytelności Values i crosshair;
- kolorów, etykiet i łatwości chwytania cursorów/markerów;
- subiektywnej ergonomii i płynności;
- manualnego sprawdzenia zachowania na rzeczywistym docelowym sprzęcie.